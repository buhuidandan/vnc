#ifndef _TUNNEL_CLIENT_H_
#define _TUNNEL_CLIENT_H_

extern "C"
{
#include <pthread.h>
}
#include <string>
#include "ProtoBufPtr.h"

class TunnelClient
{
public:
    enum TUN_STATE
    {
        TUN_UNINITED = 0x1,
        TUN_INITED = 0x2,
        TUN_RUNNING = 0x4,
        TUN_STOPPED = 0x8
    };

    static TunnelClient *createTunClient(const char *tunName);

    ~TunnelClient();
    bool start();
    bool stop();
    TUN_STATE getState() const { return m_state; }
    std::size_t tunWrite(ProtoBufPtr pBuf);

private:
    static void *workerThread(void *arg);

    TunnelClient(const TunnelClient &tun);
    TunnelClient &operator=(const TunnelClient &rhs);
    TunnelClient(const char *tunName);
    int allocTun(const char *tunName);
    bool init();
    void tunHandleIO(unsigned events);
    void sockHandleIO(unsigned events);

    std::string m_name;
    TUN_STATE m_state = TUN_UNINITED;
    int m_epollHandle = -1;
    int m_tunHandle = -1;
    int m_sockHandle = -1;
    pthread_t m_workerID;
};

#endif

