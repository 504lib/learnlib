#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef NO_LOG_ASSERT
#define LOG_ASSERT(expr) ((void)0)
#else
#include "../../Log/Log.h"
#endif

#ifndef STATIC_STACK_ENTER_CRITICAL
#define STATIC_STACK_ENTER_CRITICAL() ((void)0)
#endif

#ifndef STATIC_STACK_EXIT_CRITICAL
#define STATIC_STACK_EXIT_CRITICAL() ((void)0)
#endif

#define STATIC_STACK_MAX_CAPACITY 64

#define DECLARE_STATIC_STACK(NAME,TYPE,CAPACITY)\
    typedef char NAME##_buffer_check[(CAPACITY) > 0 ? 1 : -1];\
    typedef char NAME##_buffer_max_check[(CAPACITY) <= STATIC_STACK_MAX_CAPACITY ? 1 : -1];\
    typedef struct {\
        bool initialized;\
        TYPE buffer[(CAPACITY)];\
        volatile size_t top;\
    } NAME##_t;\
    static inline bool NAME##_IS_INIT(const NAME##_t* stack)\
    {\
        if (stack->initialized)\
        {\
            return true;\
        }\
        return false;\
    }\
    static inline void NAME##_INIT(NAME##_t* stack)\
    {\
        LOG_ASSERT(stack != NULL);\
        stack->top = 0U;\
        stack->initialized = true;\
    }\
    static inline bool NAME##_IS_EMPTY(const NAME##_t* stack)\
    {\
        bool is_empty;\
        LOG_ASSERT(stack != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(stack));\
        STATIC_STACK_ENTER_CRITICAL();\
        is_empty = (stack->top == 0U);\
        STATIC_STACK_EXIT_CRITICAL();\
        return is_empty;\
    }\
    static inline bool NAME##_IS_FULL(const NAME##_t* stack)\
    {\
        bool is_full;\
        LOG_ASSERT(stack != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(stack));\
        STATIC_STACK_ENTER_CRITICAL();\
        is_full = (stack->top == (CAPACITY));\
        STATIC_STACK_EXIT_CRITICAL();\
        return is_full;\
    }\
    static inline size_t NAME##_SIZE(const NAME##_t* stack)\
    {\
        size_t size;\
        LOG_ASSERT(stack != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(stack));\
        STATIC_STACK_ENTER_CRITICAL();\
        size = stack->top;\
        STATIC_STACK_EXIT_CRITICAL();\
        return size;\
    }\
    static inline size_t NAME##_CAPACITY(const NAME##_t* stack)\
    {\
        (void)stack;\
        return (CAPACITY);\
    }\
    static inline void NAME##_CLEAR(NAME##_t* stack)\
    {\
        LOG_ASSERT(stack != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(stack));\
        STATIC_STACK_ENTER_CRITICAL();\
        stack->top = 0U;\
        STATIC_STACK_EXIT_CRITICAL();\
    }\
    static inline bool NAME##_PUSH(NAME##_t* stack,TYPE item)\
    {\
        bool pushed;\
        LOG_ASSERT(stack != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(stack));\
        STATIC_STACK_ENTER_CRITICAL();\
        if (stack->top >= (CAPACITY))\
        {\
            pushed = false;\
        }\
        else\
        {\
            stack->buffer[stack->top] = item;\
            stack->top += 1U;\
            pushed = true;\
        }\
        STATIC_STACK_EXIT_CRITICAL();\
        return pushed;\
    }\
    static inline bool NAME##_POP(NAME##_t* stack,TYPE* item)\
    {\
        bool popped;\
        LOG_ASSERT(stack != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(stack));\
        LOG_ASSERT(item != NULL);\
        STATIC_STACK_ENTER_CRITICAL();\
        if (stack->top == 0U)\
        {\
            popped = false;\
        }\
        else\
        {\
            stack->top -= 1U;\
            *item = stack->buffer[stack->top];\
            popped = true;\
        }\
        STATIC_STACK_EXIT_CRITICAL();\
        return popped;\
    }\
    static inline bool NAME##_TOP(const NAME##_t* stack,TYPE* item)\
    {\
        bool peeked;\
        LOG_ASSERT(stack != NULL);\
        LOG_ASSERT(NAME##_IS_INIT(stack));\
        LOG_ASSERT(item != NULL);\
        STATIC_STACK_ENTER_CRITICAL();\
        if (stack->top == 0U)\
        {\
            peeked = false;\
        }\
        else\
        {\
            *item = stack->buffer[stack->top - 1U];\
            peeked = true;\
        }\
        STATIC_STACK_EXIT_CRITICAL();\
        return peeked;\
    }\
    static inline bool NAME##_PEEK(const NAME##_t* stack,TYPE* item)\
    {\
        return NAME##_TOP(stack, item);\
    }