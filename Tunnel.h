#ifndef _TUNNEL_H_
#define _TUNNEL_H_

class Tunnel
{
public:
    ~Tunnel();
    Tunnel *allocTun();
    ssize_t tunRead(char *buf, std::size_t len);
    ssize_t tunWrite(const char *buf, std::size_t len);

private:
    Tunnel(int fd): m_fd(fd) {}
    Tunnel(const Tunnel &);
    Tunnel &operator=(const Tunnel &);

    int m_fd;
};

#endif // _TUNNEL_H_

