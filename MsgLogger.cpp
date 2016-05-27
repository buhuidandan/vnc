#include <cstdarg>
#include <cstdio>
#include "MsgLogger.h"

bool MsgLogger::write(MSG_LEVEL level, const char *msg, ...)
{
    std::va_list args;

    (void)level;
    va_start(args, msg);
    std::vprintf(msg, args);
    va_end(args);

    return true;
}

