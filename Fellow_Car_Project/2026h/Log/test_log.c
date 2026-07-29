#include "Log.h"
#include <string.h>
#include <stdio.h>

static char g_buffer[1024];
static size_t g_written;

static void capture_tx(const char* buffer, size_t buffer_size)
{
    size_t copy = buffer_size < sizeof(g_buffer) - g_written ? buffer_size : sizeof(g_buffer) - g_written;
    memcpy(g_buffer + g_written, buffer, copy);
    g_written += copy;
    g_buffer[g_written] = '\0';
}

static void reset_capture(void)
{
    memset(g_buffer, 0, sizeof(g_buffer));
    g_written = 0;
}

static int test_basic_output(void)
{
    reset_capture();
    LOG_INFO("hello");
    if (strstr(g_buffer, "[INFO]") == NULL)  return 1;
    if (strstr(g_buffer, "hello") == NULL)   return 2;
    return 0;
}

static int test_level_filter(void)
{
    reset_capture();
    LOG_Set_Level(LOG_LEVEL_WARN);
    LOG_DEBUG("debug msg");
    LOG_INFO("info msg");
    if (g_written != 0) return 1;

    LOG_WARN("warn msg");
    if (strstr(g_buffer, "[WARN]") == NULL) return 2;
    return 0;
}

static int test_raw_output(void)
{
    reset_capture();
    LOG_Set_Level(LOG_LEVEL_DEBUG);
    LOG_RAW("raw data");
    if (strstr(g_buffer, "raw data") == NULL) return 1;
    if (strstr(g_buffer, "[") != NULL)       return 2;
    return 0;
}

static int test_file_line(void)
{
    reset_capture();
    LOG_Set_Level(LOG_LEVEL_DEBUG);
    LOG_DEBUG("check file/line");
    if (strstr(g_buffer, "test_log.c") == NULL) return 1;
    return 0;
}

static int test_all_levels(void)
{
    reset_capture();
    LOG_Set_Level(LOG_LEVEL_DEBUG);
    LOG_DEBUG("d");
    LOG_INFO("i");
    LOG_WARN("w");
    LOG_ERROR("e");
    LOG_FATAL("f");
    if (!strstr(g_buffer, "[DEBUG]")) return 1;
    if (!strstr(g_buffer, "[INFO]"))  return 2;
    if (!strstr(g_buffer, "[WARN]"))  return 3;
    if (!strstr(g_buffer, "[ERROR]")) return 4;
    if (!strstr(g_buffer, "[FATAL]")) return 5;
    return 0;
}

static int test_truncation(void)
{
    reset_capture();
    LOG_Set_Level(LOG_LEVEL_DEBUG);
    LOG_INFO("1234567890123456789012345678901234567890123456789012345678901234567890");
    if (g_written == 0) return 1;
    return 0;
}

static int test_null_transmit(void)
{
    LOG_Set_Transmit(NULL);
    LOG_RAW("should not crash");
    return 0;
}

int main(void)
{
    LOG_Set_Transmit(capture_tx);
    LOG_Set_Level(LOG_LEVEL_DEBUG);

    int failures = 0;
    int (*tests[])(void) = {
        test_basic_output,
        test_level_filter,
        test_raw_output,
        test_file_line,
        test_all_levels,
        test_truncation,
        test_null_transmit,
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        int result = tests[i]();
        printf("%s test_%zu\n", result == 0 ? "PASS" : "FAIL", i + 1);
        if (result != 0) failures++;
    }

    return failures;
}
