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
    static MsgLogger *getInstance() { return nullptr; }
    MsgLogger() {}
    ~MsgLogger() {}
    bool write();
};

#endif

