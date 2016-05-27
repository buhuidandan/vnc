#include "TunnelClient.h"
#include "TunnelServer.h"

bool TunnelServer::tunConnect(TunnelClient *client)
{
    if (nullptr == client)
    {
        return false;
    }
    m_client = client;

    return true;
}

bool TunnelServer::tunDisconnect(TunnelClient *client)
{
    m_client = nullptr;

    return true;
}

