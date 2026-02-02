#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <Log.h>

#define STATIC_QUEUE_MAX_CAPACITY 64

#define DECLARE_STATIC_QUEUE(NAME,TYPE,CAPACITY)\
    typedef char NAME##_buffer_check[(CAPACITY) > 0 ? 1 : -1];\
    typedef char NAME##_buffer_max_check[(CAPACITY) <= STATIC_QUEUE_MAX_CAPACITY ? 1 : -1];\
    typedef struct {\
        bool initialized;\
        TYPE buffer[CAPACITY];\
        size_t head;\
        size_t tail;\
        size_t size;\
    } NAME##_t;\
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
        q->head = 0;\
        q->tail = 0;\
        q->size = 0;\
        q->initialized = true;\
    }\
    static inline bool NAME##_IS_EMPTY(const NAME##_t* q)\
    {\
        return q->size == 0;\
    }\
    static inline bool NAME##_IS_FULL(const NAME##_t* q)\
    {\
        return q->size == CAPACITY;\
    }\
    static inline size_t NAME##_SIZE(const NAME##_t* q)\
    {\
        return q->size;\
    }\
    static inline size_t NAME##_CAPACITY(const NAME##_t* q)\
    {\
        (void)q;\
        return CAPACITY;\
    }\
    static inline bool NAME##_PUSH(NAME##_t* q,TYPE item)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        if (NAME##_IS_FULL(q))\
        {\
            return false;\
        }\
        q->buffer[q->tail] = item;\
        q->tail = (q->tail + 1) % CAPACITY;\
        q->size++;\
        return true;\
    }\
    static inline bool NAME##_POP(NAME##_t* q,TYPE* item)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        if (NAME##_IS_EMPTY(q))\
        {\
            return false;\
        }\
        *item = q->buffer[q->head];\
        q->head = (q->head + 1) % CAPACITY;\
        q->size--;\
        return true;\
    }\
    static inline bool NAME##_PEEK(const NAME##_t* q,TYPE* item)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        if (NAME##_IS_EMPTY(q))\
        {\
            return false;\
        }\
        *item = q->buffer[q->head];\
        return true;\
    }\
    static inline bool NAME##_BACK(const NAME##_t* q,TYPE* item)\
    {\
        LOG_ASSERT(q != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(q));\
        LOG_ASSERT(item != NULL);\
        if (NAME##_IS_EMPTY(q))\
        {\
            return false;\
        }\
        size_t back_index = (q->tail == 0) ? (CAPACITY - 1) : (q->tail - 1);\
        *item = q->buffer[back_index];\
        return true;\
    }\
