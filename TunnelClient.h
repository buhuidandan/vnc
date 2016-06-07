#ifndef _TUNNEL_CLIENT_H_
#define _TUNNEL_CLIENT_H_

extern "C"
{
#include <pthread.h>
}
#include <string>
#include <vector>
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
    std::vector<uint8_t> getTunMAC() const { return m_tunMAC; }

private:
    static void *workerThread(void *arg);

    TunnelClient(const TunnelClient &tun);
    TunnelClient &operator=(const TunnelClient &rhs);
    TunnelClient(const char *tunName);
    int allocTun(const char *tunName);
    bool init();
    void tunHandleIO(unsigned events);
    void sockHandleIO(unsigned events);
    void tunRxIP(ProtoBufPtr pBuf);
    void tunRxARP(ProtoBufPtr pBuf);
    bool sendToTun(uint16_t type, ProtoBufPtr pBuf);

    std::string m_name;
    TUN_STATE m_state = TUN_UNINITED;
    int m_epollHandle = -1;
    int m_tunHandle = -1;
    int m_sockHandle = -1;
    std::vector<uint8_t> m_tunMAC;
    pthread_t m_workerID;
};

#endif

