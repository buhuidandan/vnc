extern "C"
{
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
}
#include <string>
#include <cstring>
#include "MsgLogger.h"
#include "Tunnel.h"

extern MsgLogger logger;

static const char *TUN_DEV_NAME = "tap-bubblesunny";

Tunnel *Tunnel::allocTun()
{
    char *dev = nullptr;
    struct ifreq ifr;
    int fd = 0, err = 0;

    if (0 > (fd = open("/dev/net/tun", O_RDWR)))
    {
        logger.write();
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

    return new Tunnel(fd);
}

Tunnel::~Tunnel()
{
    close(m_fd);
}

void Tunnel::tunRead(char *buf, std::size_t len)
{
    ssize_t nBytes = read(m_fd, buf, len);

    if (nBytes > 0)
    return;
}

void Tunnel::tunWrite(char *buf, std::size_t len)
{
    return;
}

