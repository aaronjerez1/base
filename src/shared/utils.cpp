#include "utils.h"
#include <cstdarg>
//
//
//
//
///**
// * Safer snprintf for Linux
// * - prints up to limit-1 characters
// * - output string is always null terminated even if limit reached
// * - return value is the number of characters actually printed
// */
//int my_snprintf(char* buffer, size_t limit, const char* format, ...)
//{
//    if (limit == 0)
//        return 0;
//
//    va_list arg_ptr;
//    va_start(arg_ptr, format);
//    int ret = vsnprintf(buffer, limit, format, arg_ptr);
//    va_end(arg_ptr);
//
//    // vsnprintf on Linux returns the number of characters that would have
//    // been written if the buffer was large enough (not including null-terminator)
//    if (ret < 0)
//    {
//        // Error occurred
//        buffer[0] = '\0';
//        return 0;
//    }
//    else if (ret >= static_cast<int>(limit))
//    {
//        // Output was truncated
//        buffer[limit - 1] = '\0';
//        return limit - 1;
//    }
//
//    return ret;
//}
//
//
//
/**
 * Format a string with printf-style formatting
 * Dynamically allocates memory if needed to accommodate the formatted string
 * Returns a std::string with the formatted content
 */
std::string strprintf(const char* format, ...)
{
    // Initial buffer on the stack
    char buffer[1024];
    std::string result;

    va_list args;
    va_start(args, format);

    // Try with the stack buffer first
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);

    if (ret >= 0)
    {
        if (ret < static_cast<int>(sizeof(buffer)))
        {
            // It fit in our buffer
            result = std::string(buffer, ret);
        }
        else
        {
            // We need a bigger buffer
            result.resize(ret);
            va_end(args);
            va_start(args, format);
            vsnprintf(&result[0], ret + 1, format, args);
        }
    }

    va_end(args);
    return result;
}

bool error(const char* format, ...)
{
    char buffer[50000];
    int limit = sizeof(buffer);
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = vsnprintf(buffer, limit, format, arg_ptr);
    va_end(arg_ptr);
    if (ret < 0 || ret >= limit)
    {
        ret = limit - 1;
        buffer[limit - 1] = 0;
    }
    printf("ERROR: %s\n", buffer);
    return false;
}