#ifndef _TUNNEL_SERVER_H_
#define _TUNNEL_SERVER_H_

class TunnelClient;

class TunnelServer
{
public:
    TunnelServer() {}
    ~TunnelServer() {}
    bool tunConnect(TunnelClient *client);
    bool tunDisconnect(TunnelClient *client);

private:
    TunnelClient *m_client = nullptr;
};

#endif

