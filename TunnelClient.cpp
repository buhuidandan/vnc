extern "C"
{
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <linux/if.h>
#include <linux/if_tun.h>
}
#include <cstring>
#include <cerrno>
#include <iostream>
#include "MsgLogger.h"
#include "TunnelClient.h"
#include "VNCUtil.h"

// Ethnert packet 
const uint16_t TUN_ETH_TYPE_OFFSET = 12;
const uint16_t TUN_ETH_HDR_LEN = 14;
const uint16_t TUN_ETH_MIN_PAYLOAD = 46;
const uint16_t TUN_ETH_MAX_PAYLOAD = 1500;

// ARP packet
const uint16_t TUN_HTPYE_OFFSET = 0;
const uint16_t TUN_PTYPE_OFFSET = 2;
const uint16_t TUN_HLEN_OFFSET = 4;
const uint16_t TUN_PLEN_OFFSET = 5;
const uint16_t TUN_OPER_OFFSET = 6;
const uint16_t TUN_SHA_OFFSET = 8;
const uint16_t TUN_SPA_OFFSET = 14;
const uint16_t TUN_THA_OFFSET = 18;
const uint16_t TUN_TPA_OFFSET = 24;

const uint16_t TUN_HTYPE_ETH = 0x0001;
const uint16_t TUN_PTYPE_IPV4 = 0x0800;
const uint16_t TUN_PLEN_IPV4 = 4;
const uint16_t TUN_HLEN_MAC = 6;
const uint16_t TUN_ARP_REQ = 0x0001;
const uint16_t TUN_ARP_REPLY = 0x0002;
const uint16_t TUN_ARP_SIZE = 28;
const uint16_t TUN_IP_PAYLOAD_MAX = 576;

const uint16_t DHCP_SRC_PORT    = 68;
const uint16_t DHCP_DST_PORT    = 67;

const uint16_t TUN_RX_HDR_SZ = 100;
const uint16_t TUN_RX_TRL_SZ = 100;

extern MsgLogger logger;
uint8_t g_tunReadBuf[TUN_ETH_MAX_PAYLOAD+TUN_ETH_HDR_LEN];
// 10.0.2.10, just used for test. Normally, should read from configuration file.
const char g_tunIP[TUN_PLEN_IPV4] = {0x0a, 0x00, 0x02, 0x0a};

TunnelClient *TunnelClient::createTunClient(const char *tunName)
{
    TunnelClient *client = nullptr;

    if (nullptr == tunName)
    {
        // log
        return nullptr;
    }
    client = new TunnelClient(tunName);
    if (!client->init())
    {
        delete client;
        client = nullptr;
    }

    return client;
}

void *TunnelClient::workerThread(void *arg)
{
    int readyFds = 0;
    struct epoll_event events[2];
    TunnelClient *tunClient = static_cast<TunnelClient *>(arg);

    while (TUN_RUNNING == tunClient->m_state)
    {
        logger.write(MsgLogger::MSG_DBG, "I'm the worker thread\n");
        readyFds = epoll_wait(tunClient->m_epollHandle, events, sizeof(events)/sizeof(events[0]), -1);
        if (-1 == readyFds)
        {
            if (EINTR == errno)
            {
                continue;
            }
            else
            {
                logger.write(MsgLogger::MSG_ERR, "Failed to wait for ready fd: %s\n",
                             std::strerror(errno));
                tunClient->m_state = TUN_STOPPED;

                return reinterpret_cast<void *>(-1);
            }
        }
        for (int i = 0; i < readyFds; ++i)
        {
            if (tunClient->m_tunHandle == events[i].data.fd)
            {
                tunClient->tunHandleIO(events[i].events);
            }
            else
            {
                tunClient->sockHandleIO(events[i].events);
            }
        }
    }

    return reinterpret_cast<void *>(0);
}


TunnelClient::TunnelClient(const char *tunName)
    :m_name(tunName)
{
}

TunnelClient::~TunnelClient()
{
    stop();
    close(m_tunHandle);
    close(m_sockHandle);
    close(m_epollHandle);
}

bool TunnelClient::start()
{
    if (!(m_state & (TUN_INITED | TUN_STOPPED))
         || 0 != pthread_create(&m_workerID, nullptr, workerThread, this))
    {
        // log
        return false;
    }
    m_state = TUN_RUNNING;

    return true;
}

bool TunnelClient::stop()
{
    void *workerRet = nullptr;

    if (TUN_RUNNING == m_state)
    {
        // kill thread
        m_state = TUN_STOPPED;
        return (0 == pthread_join(m_workerID, &workerRet));
    }

    return true;
}

bool TunnelClient::sendToTun(uint16_t type, ProtoBufPtr pBuf)
{
    std::cout << "receive type: " type << "and " << pBuf->bodySize() << " bytes:" << std::endl;
    std::cout << std::hex;
    for (std::size_t i = 0; i != pBuf->bodySize(); ++i)
    {
        std::cout << (pBuf->data())[i] << " ";
    }

    return true;
}

int TunnelClient::allocTun(const char *tunName)
{
    struct ifreq ifr;
    int fd = -1;

    if (nullptr == tunName || std::strlen(tunName) > IFNAMSIZ)
    {
        logger.write(MsgLogger::MSG_ERR, "Invalid tunnel name");
        return -1;
    }
    if (-1 == (fd = open("/dev/net/tun", O_RDWR|O_NONBLOCK)))
    {
        logger.write(MsgLogger::MSG_ERR, "Open clone tun failed: %s\n", std::strerror(errno));
        return fd;
    }
    std::memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    std::strncpy(ifr.ifr_name, tunName, IFNAMSIZ);
    if (-1 == ioctl(fd, TUNSETIFF, static_cast<void *>(&ifr))
        || -1 == ioctl(fd, SIOCGIFHWADDR, static_cast<void *>(&ifr)))
    {
        logger.write(MsgLogger::MSG_ERR, "Failed to ioctl tun: %s\n", std::strerror(errno));
        close(fd);
        return -1;
    }
    m_tunMAC.assign(ifr.ifr_hwaddr.sa_data, ifr.ifr_hwaddr.sa_data+TUN_HLEN_MAC);

    return fd;
}

bool TunnelClient::init()
{
    bool ret = false;

    if (TUN_UNINITED != m_state)
    {
        // log
        return false;
    }

    if (-1 == (m_tunHandle = allocTun(m_name.data())))
    {
        logger.write(MsgLogger::MSG_ERR, "Failed to allocate tunnel.\n");
    }
    else if (-1 == (m_sockHandle = socket(AF_PACKET, SOCK_RAW|SOCK_NONBLOCK, htons(ETH_P_ALL))))
    {
        logger.write(MsgLogger::MSG_ERR, "Failed to create SOCK_RAW socket: %s\n", std::strerror(errno));
    }
    else if (-1 == (m_epollHandle = epoll_create1(0)))
    {
        logger.write(MsgLogger::MSG_ERR, "Failed to create epoll: %s\n", std::strerror(errno));
    }
    else
    {
        struct epoll_event tunEvent, sockEvent;

        tunEvent.events = sockEvent.events = EPOLLIN|EPOLLOUT|EPOLLET;
        tunEvent.data.fd = m_tunHandle;
        sockEvent.data.fd = m_sockHandle;
        if (-1 == epoll_ctl(m_epollHandle, EPOLL_CTL_ADD, m_tunHandle, &tunEvent)
            || -1 == epoll_ctl(m_epollHandle, EPOLL_CTL_ADD, m_sockHandle, &sockEvent))
        {
            logger.write(MsgLogger::MSG_ERR, "Failed to add handle to epoll: %s\n", std::strerror(errno));
        }
        // Initialize tunnel client successfully
        else
        {
            logger.write(MsgLogger::MSG_INFO, "Succeeded to add tunHandle[%d] and sockHandle[%d].\n",
                         m_tunHandle, m_sockHandle);
            ret = true;
            m_state = TUN_INITED;
        }
    }
    if (!ret)
    {
        close(m_tunHandle);
        close(m_sockHandle);
        close(m_epollHandle);
        m_tunHandle = m_sockHandle = m_epollHandle = -1;
        m_tunMAC.clear();
    }

    return ret;
}

void TunnelClient::tunRxIP(ProtoBufPtr pBuf)
{
}

void TunnelClient::tunRxARP(ProtoBufPtr pBuf)
{
    /* Ethernet ARP Packet for IPv4 Protocol Type
    ----------------------------------------------------
    |bits |          0-7         |        8 - 15             |
    ----------------------------------------------------
    |    0 |             Hardware Type                  |
    ----------------------------------------------------
    |  16 |             Protocol Type                  |
    ----------------------------------------------------
    |  32 |HW Addr Len (HLEN)| Protocol Addr Len (PLEN)|
    ----------------------------------------------------
    |  48 |                Operation (Req = 1, Reply= 2)  |
    ----------------------------------------------------
    |  64 |                Source MAC      (HLEN Byte = 6)|
    ----------------------------------------------------
    | 112 |                Source IP        (PLEN Byte = 4)|
    ----------------------------------------------------
    | 144 |                Destination MAC (HLEN Byte = 6)|
    ----------------------------------------------------
    | 192 |                Destination IP    (PLEN Byte = 4)|
    ----------------------------------------------------
    */
    // ARP packet for Ethernet type IPv4 is 28 bytes, do not process if less than that
    if(TUN_ARP_SIZE > pBuf->bodySize())
    {
        return;
    }

    uint8_t *p = pBuf->data();
    uint16_t hType = get_be16(p); // Hardware Type
    uint16_t ptype = get_be16(p + TUN_PTYPE_OFFSET); // Protocol Type
    uint16_t operation = get_be16(p + TUN_OPER_OFFSET);
    uint8_t hlen = *(p + TUN_HLEN_OFFSET); // Length of HW Address in Bytes
    uint8_t plen = *(p + TUN_PLEN_OFFSET); // Length of IP Address in Bytes

    // Qualify Hardware Type, Protocol Type, HW and Protocol Address Length for IPv4
    if (!(TUN_HTYPE_ETH == hType && TUN_PTYPE_IPV4 == ptype 
         && TUN_HLEN_MAC == hlen && TUN_PLEN_IPV4 == plen))
    {
        return;
    }
    // Process only ARP Request for tunnel
    if (TUN_ARP_REQ != operation || std::memcmp(p + TUN_TPA_OFFSET, g_tunIP, TUN_PLEN_IPV4))
    {
        return;
    }

    // Construct ARP reply in place
    set_be16(p + TUN_OPER_OFFSET, TUN_ARP_REPLY); 
    std::memcpy(p + TUN_THA_OFFSET, p + TUN_SHA_OFFSET, TUN_HLEN_MAC);
    std::memcpy(p + TUN_TPA_OFFSET, p + TUN_SPA_OFFSET, TUN_PLEN_IPV4);
    std::memcpy(p + TUN_SHA_OFFSET, m_tunMAC.data(), TUN_HLEN_MAC);
    std::memcpy(p + TUN_SPA_OFFSET, g_tunIP, TUN_PLEN_IPV4);
    if (!sendToTun(ETH_P_ARP, std::move(pBuf)))
    {
        char tunIP[INET_ADDRSTRLEN] = {0};
        
        logger.write(MsgLogger::MSG_ERR, "Failed to send ARP Resp to: %s",
                        inet_ntop(AF_INET, g_tunIP, tunIP, INET_ADDRSTRLEN));
    }

    return;
}

void TunnelClient::tunHandleIO(unsigned events)
{
    logger.write(MsgLogger::MSG_DBG, "tunHandle: Not supported.\n");
}

void TunnelClient::sockHandleIO(unsigned events)
{
    static std::size_t evtSeq= 0;

    if (events & EPOLLIN)
    {
        // read
        ssize_t bytesRead = recv(m_sockHandle, g_tunReadBuf, sizeof(g_tunReadBuf), 0);

        logger.write(MsgLogger::MSG_DBG, "in-event[%lu] of sockHandle, bytesRead: %d\n",
                     ++evtSeq, bytesRead);
        if (-1 == bytesRead)
        {
            logger.write(MsgLogger::MSG_ERR, "Read from sockHandle error: %s.\n", std::strerror(errno));
        }
        else if (bytesRead < TUN_ETH_HDR_LEN)
        {
            logger.write(MsgLogger::MSG_ERR, "Invalid ethernet frame.\n");
        }
        else
        {
            uint8_t *payload = g_tunReadBuf + TUN_ETH_HDR_LEN;
            std::size_t len = bytesRead - TUN_ETH_HDR_LEN;
            uint16_t type = get_be16(g_tunReadBuf + TUN_ETH_TYPE_OFFSET);
            ProtoBufPtr pBuf(new ProtoBuf(TUN_RX_HDR_SZ, len, TUN_RX_TRL_SZ));

            std::memcpy(pBuf->data(), payload, len);
            switch (type)
            {
                case ETH_P_IP:
                    logger.write(MsgLogger::MSG_INFO, "IP packet\n");
                    tunRxIP(std::move(pBuf));
                    break;
                case ETH_P_ARP:
                    logger.write(MsgLogger::MSG_INFO, "ARP packet\n");
                    tunRxARP(std::move(pBuf));
                    break;
                default:
                    logger.write(MsgLogger::MSG_INFO, "Not support ethernet type: %#x\n", type);
                    break;
            }
        }
    }
    if (events & EPOLLOUT)
    {
        // write
        logger.write(MsgLogger::MSG_DBG, "out-event of socketHandle.\n");
    }
    if (!(events & (EPOLLIN|EPOLLOUT)))
    {
        logger.write(MsgLogger::MSG_DBG, "not supported event: %d\n", events);
    }
}

