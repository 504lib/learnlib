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
        LOG_ASSERT(!NAME##_IS_INIT(q));\
        q->head = 0U;\
        q->tail = 0U;\
        q->initialized = true;\
    }\
    static inline bool NAME##_IS_EMPTY(const NAME##_t* q)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        return q->head == q->tail;\
    }\
    static inline bool NAME##_IS_FULL(const NAME##_t* q)\
    {\
        size_t next_tail;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        next_tail = (q->tail + 1U) % NAME##_RAW_CAPACITY();\
        return next_tail == q->head;\
    }\
    static inline size_t NAME##_SIZE(const NAME##_t* q)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        if (q->tail >= q->head)\
        {\
            return q->tail - q->head;\
        }\
        return NAME##_RAW_CAPACITY() - q->head + q->tail;\
    }\
    static inline size_t NAME##_CAPACITY(const NAME##_t* q)\
    {\
        (void)q;\
        return CAPACITY;\
    }\
    static inline bool NAME##_PUSH(NAME##_t* q,TYPE item)\
    {\
        size_t next_tail;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        next_tail = (q->tail + 1U) % NAME##_RAW_CAPACITY();\
        if (next_tail == q->head)\
        {\
            return false;\
        }\
        q->buffer[q->tail] = item;\
        q->tail = next_tail;\
        return true;\
    }\
    static inline bool NAME##_POP(NAME##_t* q,TYPE* item)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        if (q->head == q->tail)\
        {\
            return false;\
        }\
        *item = q->buffer[q->head];\
        q->head = (q->head + 1U) % NAME##_RAW_CAPACITY();\
        return true;\
    }\
    static inline bool NAME##_PEEK(const NAME##_t* q,TYPE* item)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        if (q->head == q->tail)\
        {\
            return false;\
        }\
        *item = q->buffer[q->head];\
        return true;\
    }\
    static inline bool NAME##_BACK(const NAME##_t* q,TYPE* item)\
    {\
        size_t back_index;\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        if (q->head == q->tail)\
        {\
            return false;\
        }\
        back_index = (q->tail == 0U) ? (NAME##_RAW_CAPACITY() - 1U) : (q->tail - 1U);\
        *item = q->buffer[back_index];\
        return true;\
    }
