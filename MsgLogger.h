#ifndef _MSG_LOGGER_H_
#define _MSG_LOGGER_H_

class MsgLogger
{
public:
    enum MSG_LEVEL
    {
        MSG_EMERG = 0,
        MSG_ERR,
        MSG_WARNING,
        MSG_INFO,
        MSG_DBG
    };

    MsgLogger() {}
    ~MsgLogger() {}
    bool write(MSG_LEVEL level, const char *msg, ...);
};

#endif

