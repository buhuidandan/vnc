#ifndef _TUNNEL_H_
#define _TUNNEL_H_

#include "ProtoBufPtr.h"

class Tunnel
{
public:
    enum TunnelType
    {
        TUN_MAC = 0,
        TUN_IP
    };
    static Tunnel *allocTun(TunnelType type);
    ~Tunnel();
    void read(ProtoBufPtr pBuf);
    void write(ProtoBufPtr pBuf);
    TunnelType getType() const { return m_type; }

private:
    Tunnel(int fd, TunnelType type): m_fd(fd), m_type(type) {}
    Tunnel(const Tunnel &);
    operator=(const Tunnel &);

    int m_fd;
    TunnelType m_type;
};

#endif // _TUNNEL_H_

