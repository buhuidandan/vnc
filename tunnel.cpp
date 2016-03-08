extern "C"
{
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
}
#include <cstring>
#include "MsgLogger.h"
#include "tunnel.h"

const char *TUN_DEV_NAME = "PaopaoDandan";

Tunnel *Tunnel::allocTun(TunnelType type)
{
    char *dev = NULL;
    struct ifreq ifr;
    int fd = 0, err = 0;

    if (0 > (fd = open("/dev/net/tun", O_RDWR)))
    {
        // log
        return nullptr;
    }
    std::memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    std::strncpy(ifr.ifr_name, TUN_DEV_NAME, IFNAMSIZ);
    if (0 > (err = ioctl(fd, TUNSETIFF, static_cast<void *>(&ifr))))
    {
        // log
        return nullptr;
    }

    return new Tunnel(ifr.ifr_name, fd); 
}

Tunnel::~Tunnel()
{
    if (m_fd >= 0)
    {
        close(m_fd);
    }
}

void Tunnel::read(ProtoBufPtr pBuf)
{
    return;
}

void Tunnel::write(ProtoBufPtr pBuf)
{
    return;
}

