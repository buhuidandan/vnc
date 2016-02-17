extern "C"
{
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
}
#include "tunnel.h"

Tunnel *Tunnel::allocTun(const std::string &devName)
{
    char *dev = NULL;
    struct ifreq ifr;
    int fd = 0, err = 0;

    if (0 > (fd = open("/dev/net/tun", O_RDWR)))
    {
        // log
        return nullptr;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (!devName.empty())
    {
        std::strncpy(ifr.ifr_name, devName.c_str(), IFNAMSIZ);
    }
    if (0 > (err = ioctl(fd, TUNSETIFF, static_cast<void *>(&ifr))))
    {
        // log
        return nullptr;
    }

    return new Tunnel(ifr.ifr_name, fd); 
}

Tunnel::Tunnel(const char *devName, int fd): m_devName(devName), m_fd(fd)
{
}
Tunnel::~Tunnel()
{
    if (m_fd >= 0)
    {
        close(m_fd);
    }
}

bool Tunnel::start()
{
    return false;
}

void Tunnel::stop()
{
}
