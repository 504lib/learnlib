#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "protocol.h"

#define TEST_HEADER1 0xAAu
#define TEST_HEADER2 0x55u
#define TEST_TAIL1   0x0Du
#define TEST_TAIL2   0x0Au
#define TEST_ACK_TYPE 0xFFu
#define TEST_FRAME_OVERHEAD 8u
#define TEST_MAX_PAYLOAD (UART_PROTOCOL_FRAME_BUFFER_LEN - TEST_FRAME_OVERHEAD)

DECLARE_STATIC_QUEUE(TestByteQueue, uint8_t, 32)

typedef struct
{
    uint8_t buffer[UART_PROTOCOL_FRAME_BUFFER_LEN * 4u];
    size_t len;
    size_t call_count;
} transmit_spy_t;

typedef struct
{
    uint8_t frame_type;
    uint8_t payload[UART_PROTOCOL_FRAME_BUFFER_LEN];
    uint16_t len;
    size_t call_count;
} receive_spy_t;

typedef struct
{
    uint8_t frame_type;
    size_t call_count;
} timeout_spy_t;

typedef struct
{
    UART_protocol_t protocol;
    TestByteQueue_t queue;
    transmit_spy_t tx_spy;
    receive_spy_t rx_spy;
    timeout_spy_t timeout_spy;
} protocol_fixture_t;

static protocol_fixture_t* g_active_fixture = NULL;
static uint32_t g_fake_tick = 0;

static bool queue_push(void* queue_instance, const uint8_t data)
{
    return TestByteQueue_PUSH((TestByteQueue_t*)queue_instance, data);
}

static bool queue_pop(void* queue_instance, uint8_t* data)
{
    return TestByteQueue_POP((TestByteQueue_t*)queue_instance, data);
}

static uint16_t test_checksum(const uint8_t* data, size_t len)
{
    uint16_t sum = 0;
    for (size_t index = 0; index < len; ++index)
    {
        sum = (uint16_t)(sum + data[index]);
    }
    return sum;
}

static size_t build_frame(uint8_t* output, uint8_t type, const uint8_t* payload, uint8_t len)
{
    const uint16_t checksum = (uint16_t)(type + len + test_checksum(payload, len));

    output[0] = TEST_HEADER1;
    output[1] = TEST_HEADER2;
    output[2] = type;
    output[3] = len;
    if (len > 0u)
    {
        memcpy(&output[4], payload, len);
    }
    output[4 + len] = (uint8_t)((checksum >> 8) & 0xFFu);
    output[5 + len] = (uint8_t)(checksum & 0xFFu);
    output[6 + len] = TEST_TAIL1;
    output[7 + len] = TEST_TAIL2;
    return (size_t)(TEST_FRAME_OVERHEAD + len);
}

static bool test_transmit(const uint8_t* data, uint16_t len)
{
    assert(g_active_fixture != NULL);
    assert(data != NULL || len == 0u);
    assert(g_active_fixture->tx_spy.len + len <= sizeof(g_active_fixture->tx_spy.buffer));

    memcpy(&g_active_fixture->tx_spy.buffer[g_active_fixture->tx_spy.len], data, len);
    g_active_fixture->tx_spy.len += len;
    g_active_fixture->tx_spy.call_count += 1u;
    return true;
}

static void test_receive_handler(uint8_t frame_type, const uint8_t* frame_data, uint16_t frame_len)
{
    assert(g_active_fixture != NULL);
    g_active_fixture->rx_spy.frame_type = frame_type;
    g_active_fixture->rx_spy.len = frame_len;
    g_active_fixture->rx_spy.call_count += 1u;
    if (frame_len > 0u)
    {
        memcpy(g_active_fixture->rx_spy.payload, frame_data, frame_len);
    }
}

static void test_timeout_handler(uint8_t current_frame_type)
{
    assert(g_active_fixture != NULL);
    g_active_fixture->timeout_spy.frame_type = current_frame_type;
    g_active_fixture->timeout_spy.call_count += 1u;
}

static uint32_t test_get_tick(void)
{
    return g_fake_tick;
}

static void reset_fixture(protocol_fixture_t* fixture, bool enable_ack)
{
    Uart_Protocol_FunctionsParameters required = {
        .Head_Tial_Frame_struct = {
            .Headerframe1 = TEST_HEADER1,
            .Headerframe2 = TEST_HEADER2,
            .Tailframe1 = TEST_TAIL1,
            .Tailframe2 = TEST_TAIL2,
        },
        .transmit_function = test_transmit,
        .frame_received_handler = test_receive_handler,
        .queue_ops = {
            .Queue_instance = &fixture->queue,
            .Queue_pushback = queue_push,
            .Queue_popfront = queue_pop,
        },
    };
    Uart_Protocol_OptionalFunctionsParameters optional = {
        .timeout_handler = enable_ack ? test_timeout_handler : NULL,
        .GetTick = enable_ack ? test_get_tick : NULL,
    };

    memset(fixture, 0, sizeof(*fixture));
    TestByteQueue_INIT(&fixture->queue);
    g_active_fixture = fixture;
    g_fake_tick = 0u;
    assert(Uart_Protocol_Init(&fixture->protocol, required, optional));
}

static void feed_frame(protocol_fixture_t* fixture, const uint8_t* frame, size_t len)
{
    assert(Uart_Protocol_ProcessReceivedDataBuffer(&fixture->protocol, (uint8_t*)frame, len));
    for (size_t index = 0; index < len; ++index)
    {
        (void)Uart_Protocol_Loop(&fixture->protocol);
    }
}

static void test_receive_zero_length_frame_without_ack(void)
{
    protocol_fixture_t fixture;
    uint8_t frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    const size_t frame_len = build_frame(frame, 0x10u, NULL, 0u);

    reset_fixture(&fixture, false);
    feed_frame(&fixture, frame, frame_len);

    assert(fixture.rx_spy.call_count == 1u);
    assert(fixture.rx_spy.frame_type == 0x10u);
    assert(fixture.rx_spy.len == 0u);
    assert(fixture.tx_spy.call_count == 0u);
}

static void test_receive_frame_with_bad_checksum_is_ignored(void)
{
    protocol_fixture_t fixture;
    uint8_t frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    const uint8_t payload[] = {0x01u, 0x02u, 0x03u};
    const size_t frame_len = build_frame(frame, 0x22u, payload, (uint8_t)sizeof(payload));

    reset_fixture(&fixture, false);
    frame[4 + sizeof(payload)] ^= 0x01u;
    feed_frame(&fixture, frame, frame_len);

    assert(fixture.rx_spy.call_count == 0u);
    assert(fixture.tx_spy.call_count == 0u);
}

static void test_receive_oversized_length_recovers_on_next_valid_frame(void)
{
    protocol_fixture_t fixture;
    uint8_t invalid_frame[] = {
        TEST_HEADER1, TEST_HEADER2, 0x33u, (uint8_t)(TEST_MAX_PAYLOAD + 1u),
        0x00u, 0x00u, TEST_TAIL1, TEST_TAIL2,
    };
    uint8_t valid_frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    const uint8_t payload[] = {0xABu};
    const size_t valid_len = build_frame(valid_frame, 0x44u, payload, (uint8_t)sizeof(payload));

    reset_fixture(&fixture, false);
    feed_frame(&fixture, invalid_frame, sizeof(invalid_frame));
    feed_frame(&fixture, valid_frame, valid_len);

    assert(fixture.rx_spy.call_count == 1u);
    assert(fixture.rx_spy.frame_type == 0x44u);
    assert(fixture.rx_spy.len == sizeof(payload));
    assert(fixture.rx_spy.payload[0] == 0xABu);
}

static void test_transmit_rejects_payload_over_max_length(void)
{
    protocol_fixture_t fixture;
    uint8_t payload[TEST_MAX_PAYLOAD + 1u];

    memset(payload, 0x5Au, sizeof(payload));
    reset_fixture(&fixture, false);

    assert(!Uart_Protocol_Transmit_Frame(&fixture.protocol, payload, 0x55u, (uint8_t)sizeof(payload)));
    assert(fixture.tx_spy.call_count == 0u);
}

static void test_transmit_max_payload_succeeds(void)
{
    protocol_fixture_t fixture;
    uint8_t payload[TEST_MAX_PAYLOAD];
    uint8_t expected_frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    size_t expected_len;

    for (size_t index = 0; index < sizeof(payload); ++index)
    {
        payload[index] = (uint8_t)(0x10u + index);
    }

    expected_len = build_frame(expected_frame, 0x66u, payload, (uint8_t)sizeof(payload));

    reset_fixture(&fixture, false);

    assert(Uart_Protocol_Transmit_Frame(&fixture.protocol, payload, 0x66u, (uint8_t)sizeof(payload)));
    assert(fixture.tx_spy.call_count == 1u);
    assert(fixture.tx_spy.len == expected_len);
    assert(memcmp(fixture.tx_spy.buffer, expected_frame, expected_len) == 0);
}

static void test_ack_enabled_receive_sends_ack_and_delivers_payload(void)
{
    protocol_fixture_t fixture;
    uint8_t frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    uint8_t ack_frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    const uint8_t payload[] = {0xC1u, 0xC2u};
    const size_t frame_len = build_frame(frame, 0x77u, payload, (uint8_t)sizeof(payload));
    const size_t ack_len = build_frame(ack_frame, TEST_ACK_TYPE, NULL, 0u);

    reset_fixture(&fixture, true);
    feed_frame(&fixture, frame, frame_len);

    assert(fixture.rx_spy.call_count == 1u);
    assert(fixture.tx_spy.call_count == 1u);
    assert(fixture.tx_spy.len == ack_len);
    assert(memcmp(fixture.tx_spy.buffer, ack_frame, ack_len) == 0);
}

static void test_ack_timeout_retries_then_stops_after_ack(void)
{
    protocol_fixture_t fixture;
    uint8_t payload[] = {0x12u, 0x34u};
    uint8_t expected_frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    uint8_t ack_frame[UART_PROTOCOL_FRAME_BUFFER_LEN];
    const size_t expected_len = build_frame(expected_frame, 0x88u, payload, (uint8_t)sizeof(payload));
    const size_t ack_len = build_frame(ack_frame, TEST_ACK_TYPE, NULL, 0u);

    reset_fixture(&fixture, true);
    assert(Uart_Protocol_Transmit_Frame(&fixture.protocol, payload, 0x88u, (uint8_t)sizeof(payload)));
    assert(fixture.tx_spy.call_count == 1u);
    assert(fixture.tx_spy.len == expected_len);
    assert(memcmp(fixture.tx_spy.buffer, expected_frame, expected_len) == 0);

    g_fake_tick = fixture.protocol.tickBased_timeout.timeout_threshold + 1u;
    assert(Uart_Protocol_Loop(&fixture.protocol));
    assert(fixture.tx_spy.call_count == 2u);
    assert(fixture.tx_spy.len == expected_len * 2u);
    assert(memcmp(&fixture.tx_spy.buffer[expected_len], expected_frame, expected_len) == 0);

    feed_frame(&fixture, ack_frame, ack_len);
    const size_t len_after_ack = fixture.tx_spy.len;

    g_fake_tick += fixture.protocol.tickBased_timeout.timeout_threshold + 1u;
    assert(Uart_Protocol_Loop(&fixture.protocol));
    assert(fixture.tx_spy.len == len_after_ack);
    assert(fixture.timeout_spy.call_count == 0u);
}

static void test_ack_timeout_hits_max_retry_and_calls_timeout_handler(void)
{
    protocol_fixture_t fixture;
    uint8_t payload[] = {0x99u};

    reset_fixture(&fixture, true);
    assert(Uart_Protocol_Transmit_Frame(&fixture.protocol, payload, 0x99u, (uint8_t)sizeof(payload)));

    for (size_t retry = 0; retry < fixture.protocol.tickBased_timeout.max_try_times; ++retry)
    {
        g_fake_tick += fixture.protocol.tickBased_timeout.timeout_threshold + 1u;
        assert(Uart_Protocol_Loop(&fixture.protocol));
    }

    g_fake_tick += fixture.protocol.tickBased_timeout.timeout_threshold + 1u;
    assert(Uart_Protocol_Loop(&fixture.protocol));
    assert(fixture.timeout_spy.call_count == 1u);
    assert(fixture.timeout_spy.frame_type == 0u);
}

int main(void)
{
    test_receive_zero_length_frame_without_ack();
    test_receive_frame_with_bad_checksum_is_ignored();
    test_receive_oversized_length_recovers_on_next_valid_frame();
    test_transmit_rejects_payload_over_max_length();
    test_transmit_max_payload_succeeds();
    test_ack_enabled_receive_sends_ack_and_delivers_payload();
    test_ack_timeout_retries_then_stops_after_ack();
    test_ack_timeout_hits_max_retry_and_calls_timeout_handler();

    puts("protocol_test: all tests passed");
    return 0;
}