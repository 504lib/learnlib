#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef NO_LOG_ASSERT
#define LOG_ASSERT(expr) ((void)0)
#else
#include "../Log/Log.h"
#endif

#ifndef STATIC_QUEUE_ENTER_CRITICAL
#define STATIC_QUEUE_ENTER_CRITICAL() ((void)0)
#endif

#ifndef STATIC_QUEUE_EXIT_CRITICAL
#define STATIC_QUEUE_EXIT_CRITICAL() ((void)0)
#endif

#define STATIC_QUEUE_MAX_CAPACITY 64

#define DECLARE_STATIC_QUEUE(NAME,TYPE,CAPACITY)\
    typedef char NAME##_buffer_check[(CAPACITY) > 0 ? 1 : -1];\
    typedef char NAME##_buffer_max_check[(CAPACITY) <= STATIC_QUEUE_MAX_CAPACITY ? 1 : -1];\
    typedef struct {\
        bool initialized;\
        TYPE buffer[(CAPACITY) + 1];\
        volatile size_t head;\
        volatile size_t tail;\
    } NAME##_t;\
    static inline size_t NAME##_RAW_CAPACITY(void)\
    {\
        return (CAPACITY) + 1U;\
    }\
    static inline bool NAME##_IS_INIT(const NAME##_t* q)\
    {\
        if (q->initialized)\
        {\
            return true;\
        }\
        return false;\
    }\
    static inline void NAME##_INIT(NAME##_t* q)\
    {\
        LOG_ASSERT(q != NULL);\
        q->head = 0U;\
        q->tail = 0U;\
        q->initialized = true;\
    }\
    static inline bool NAME##_IS_EMPTY(const NAME##_t* q)\
    {\
        bool is_empty;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        STATIC_QUEUE_ENTER_CRITICAL();\
        is_empty = (q->head == q->tail);\
        STATIC_QUEUE_EXIT_CRITICAL();\
        return is_empty;\
    }\
    static inline bool NAME##_IS_FULL(const NAME##_t* q)\
    {\
        bool is_full;\
        size_t head;\
        size_t tail;\
        size_t next_tail;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        STATIC_QUEUE_ENTER_CRITICAL();\
        head = q->head;\
        tail = q->tail;\
        STATIC_QUEUE_EXIT_CRITICAL();\
        next_tail = (tail + 1U) % NAME##_RAW_CAPACITY();\
        is_full = (next_tail == head);\
        return is_full;\
    }\
    static inline size_t NAME##_SIZE(const NAME##_t* q)\
    {\
        size_t head;\
        size_t tail;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        STATIC_QUEUE_ENTER_CRITICAL();\
        head = q->head;\
        tail = q->tail;\
        STATIC_QUEUE_EXIT_CRITICAL();\
        if (tail >= head)\
        {\
            return tail - head;\
        }\
        return NAME##_RAW_CAPACITY() - head + tail;\
    }\
    static inline size_t NAME##_CAPACITY(const NAME##_t* q)\
    {\
        (void)q;\
        return CAPACITY;\
    }\
    static inline bool NAME##_PUSH(NAME##_t* q,TYPE item)\
    {\
        bool pushed;\
        size_t next_tail;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        STATIC_QUEUE_ENTER_CRITICAL();\
        next_tail = (q->tail + 1U) % NAME##_RAW_CAPACITY();\
        if (next_tail == q->head)\
        {\
            pushed = false;\
        }\
        else\
        {\
            q->buffer[q->tail] = item;\
            q->tail = next_tail;\
            pushed = true;\
        }\
        STATIC_QUEUE_EXIT_CRITICAL();\
        return pushed;\
    }\
    static inline bool NAME##_POP(NAME##_t* q,TYPE* item)\
    {\
        bool popped;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        STATIC_QUEUE_ENTER_CRITICAL();\
        if (q->head == q->tail)\
        {\
            popped = false;\
        }\
        else\
        {\
            *item = q->buffer[q->head];\
            q->head = (q->head + 1U) % NAME##_RAW_CAPACITY();\
            popped = true;\
        }\
        STATIC_QUEUE_EXIT_CRITICAL();\
        return popped;\
    }\
    static inline bool NAME##_PEEK(const NAME##_t* q,TYPE* item)\
    {\
        bool peeked;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        STATIC_QUEUE_ENTER_CRITICAL();\
        if (q->head == q->tail)\
        {\
            peeked = false;\
        }\
        else\
        {\
            *item = q->buffer[q->head];\
            peeked = true;\
        }\
        STATIC_QUEUE_EXIT_CRITICAL();\
        return peeked;\
    }\
    static inline bool NAME##_BACK(const NAME##_t* q,TYPE* item)\
    {\
        bool backed;\
        size_t back_index;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        STATIC_QUEUE_ENTER_CRITICAL();\
        if (q->head == q->tail)\
        {\
            backed = false;\
        }\
        else\
        {\
            back_index = (q->tail == 0U) ? (NAME##_RAW_CAPACITY() - 1U) : (q->tail - 1U);\
            *item = q->buffer[back_index];\
            backed = true;\
        }\
        STATIC_QUEUE_EXIT_CRITICAL();\
        return backed;\
    }
