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

extern MsgLogger logger;

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

std::size_t TunnelClient::tunWrite(ProtoBufPtr pBuf)
{
    std::cout << "receive " << pBuf->bodySize() << " bytes:" << std::endl;
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
    if (-1 == ioctl(fd, TUNSETIFF, static_cast<void *>(&ifr)))
    {
        logger.write(MsgLogger::MSG_ERR, "Failed to ioctl tun: %s\n", std::strerror(errno));
        close(fd);
        return -1;
    }

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
    }

    return ret;
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
        unsigned char buf[1500] = {0};
        ssize_t bytesRead = recv(m_sockHandle, buf, 1500, 0);

        logger.write(MsgLogger::MSG_DBG, "in-event[%lu] of sockHandle, bytesRead: %d\n",
                     ++evtSeq, bytesRead);
        if (-1 == bytesRead)
        {
            logger.write(MsgLogger::MSG_ERR, "Read from sockHandle error: %s.\n", std::strerror(errno));
        }
        else
        {
            for (ssize_t i = 0; i < bytesRead; ++i)
            {
                logger.write(MsgLogger::MSG_DBG, "%#x ", buf[i]);
            }
            logger.write(MsgLogger::MSG_DBG, "\n");
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

