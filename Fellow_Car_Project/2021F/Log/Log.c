#include "Log.h"

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