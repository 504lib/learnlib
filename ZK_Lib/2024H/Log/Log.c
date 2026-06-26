#include "Log.h"
#include <stdarg.h>
#include "../static_queue/static_queue.h"
#include <stdint.h>


static LOG_LEVEL_T LOG_Level = LOG_LEVEL_INFO;
static void (*transmit_interface)(const char* buffer, size_t buffer_size) = NULL;

#if LOG_USE_QUEUE == 1
typedef struct LOG_BUFFER
{
    char buffer[LOG_BUFFER_SIZE];
    LOG_LEVEL_T level;
}LOG_BUFFER_T;

DECLARE_STATIC_QUEUE(LOG_BUFFER_Queue,LOG_BUFFER_T,LOG_BUFFER_SIZE)

LOG_BUFFER_Queue_t LOG_BUFFER_Queue;

#endif

StaticBufferError check_pool(const char* buffer, size_t buffer_size,size_t required_size)
{
    #if __cplusplus
    if (buffer == nullptr)
    {
        return StaticBufferError::INVALID_POINTER;
    }
    if (buffer_size < required_size)
    {
        return StaticBufferError::NOT_ENOUGH_MEMORY;
    }
    return StaticBufferError::OK;
    #else
    if (buffer == NULL)
    {
        return INVALID_POINTER;
    }
    if (buffer_size < required_size)
    {
        return NOT_ENOUGH_MEMORY;
    }
    return OK;
    #endif
}


void LOG_Set_Level(LOG_LEVEL_T level)
{
    LOG_Level = level;
}

void LOG_Init(void (*tx)(const char* buffer, size_t buffer_size))
{

    #if LOG_USE_QUEUE == 1
    LOG_BUFFER_Queue_INIT(&LOG_BUFFER_Queue);
    #endif
    transmit_interface = tx;
    LOG_Level = LOG_LEVEL_INFO;
}
#if LOG_USE_QUEUE == 1
void LOG_Process(void)
{
    LOG_BUFFER_T buffer;
    if (!LOG_BUFFER_Queue_POP(&LOG_BUFFER_Queue,&buffer))
    {
        return;
    }
    if (transmit_interface != NULL)
    {
        transmit_interface(buffer.buffer,strnlen(buffer.buffer,LOG_BUFFER_SIZE));
    }
}
#endif


LOG_LEVEL_T LOG_Get_Level(void)
{
    return LOG_Level;
}

void LOG_Set_Transmit(void (*tx)(const char* buffer, size_t buffer_size))
{
    transmit_interface = tx;
}

int LOG_Snprintf(char* buffer, size_t buffer_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, buffer_size, fmt, args);
    va_end(args);
    return len;
}

void LOG_RAW(const char* fmt,...)
{
    #if LOG_USE_QUEUE == 1
    LOG_BUFFER_T buffer_structure;
    char* buffer = buffer_structure.buffer;
    #else
    char buffer[LOG_BUFFER_SIZE];
    #endif
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, LOG_BUFFER_SIZE, fmt, args);
    va_end(args);
    #if LOG_USE_QUEUE == 1
    buffer_structure.level = LOG_LEVEL_RAW;
    (void)LOG_BUFFER_Queue_PUSH(&LOG_BUFFER_Queue,buffer_structure);
    #else
    if (transmit_interface != NULL)
    {
        transmit_interface(buffer, strnlen(buffer, LOG_BUFFER_SIZE));
    }
    #endif // LOG_USE_QUEUE == 1
}

void _LOG_DEBUG(const char* fmt,...)
{
    if (LOG_LEVEL_DEBUG < LOG_Level)
    {
        return;
    }
    #if LOG_USE_QUEUE == 1
    LOG_BUFFER_T buffer_structure;
    char* buffer = buffer_structure.buffer;
    #else
    char buffer[LOG_BUFFER_SIZE];
    #endif
    int len = LOG_Snprintf(buffer, LOG_BUFFER_SIZE, "[DEBUG]");
    if (len < 0)
    {
        len = 0;
    }
    if (len >= LOG_BUFFER_SIZE)
    {
        len = LOG_BUFFER_SIZE - 1;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + len, LOG_BUFFER_SIZE - len, fmt, args);
    va_end(args);
    LOG_RAW(buffer);
}

void _LOG_INFO(const char* fmt,...)
{
    if (LOG_LEVEL_INFO < LOG_Level)
    {
        return;
    }
    #if LOG_USE_QUEUE == 1
    LOG_BUFFER_T buffer_structure;
    char* buffer = buffer_structure.buffer;
    #else
    char buffer[LOG_BUFFER_SIZE];
    #endif
    int len = LOG_Snprintf(buffer, LOG_BUFFER_SIZE, "[INFO]");
    if (len < 0)
    {
        len = 0;
    }
    if (len >= LOG_BUFFER_SIZE)
    {
        len = LOG_BUFFER_SIZE - 1;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + len, LOG_BUFFER_SIZE - len, fmt, args);
    va_end(args);
    LOG_RAW(buffer);
}

void _LOG_WARN(const char* fmt,...)
{
    if (LOG_LEVEL_WARN < LOG_Level)
    {
        return;
    }
    #if LOG_USE_QUEUE == 1
    LOG_BUFFER_T buffer_structure;
    char* buffer = buffer_structure.buffer;
    #else
    char buffer[LOG_BUFFER_SIZE];
    #endif
    int len = LOG_Snprintf(buffer, LOG_BUFFER_SIZE, "[WARN]");
    if (len < 0)
    {
        len = 0;
    }
    if (len >= LOG_BUFFER_SIZE)
    {
        len = LOG_BUFFER_SIZE - 1;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + len, LOG_BUFFER_SIZE - len, fmt, args);
    va_end(args);
    LOG_RAW(buffer);
}

void _LOG_FATAL(const char* fmt,...)
{
    if (LOG_LEVEL_FATAL < LOG_Level)
    {
        return;
    }
    #if LOG_USE_QUEUE == 1
    LOG_BUFFER_T buffer_structure;
    char* buffer = buffer_structure.buffer;
    #else
    char buffer[LOG_BUFFER_SIZE];
    #endif
    int len = LOG_Snprintf(buffer, LOG_BUFFER_SIZE, "[FATAL]");
    if (len < 0)
    {
        len = 0;
    }
    if (len >= LOG_BUFFER_SIZE)
    {
        len = LOG_BUFFER_SIZE - 1;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + len, LOG_BUFFER_SIZE - len, fmt, args);
    va_end(args);
    LOG_RAW(buffer);
}

void _LOG_ERROR(const char* fmt,...)
{
    if (LOG_LEVEL_ERROR < LOG_Level)
    {
        return;
    }
    #if LOG_USE_QUEUE == 1
    LOG_BUFFER_T buffer_structure;
    char* buffer = buffer_structure.buffer;
    #else
    char buffer[LOG_BUFFER_SIZE];
    #endif
    int len = LOG_Snprintf(buffer, LOG_BUFFER_SIZE, "[ERROR]");
    if (len < 0)
    {
        len = 0;
    }
    if (len >= LOG_BUFFER_SIZE)
    {
        len = LOG_BUFFER_SIZE - 1;
    }
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + len, LOG_BUFFER_SIZE - len, fmt, args);
    va_end(args);
    LOG_RAW(buffer);
}

