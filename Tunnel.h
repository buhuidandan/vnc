#ifndef _TUNNEL_H_
#define _TUNNEL_H_

class Tunnel
{
public:
    ~Tunnel();
    Tunnel *allocTun();
    void tunRead(char *buf, std::size_t len);
    void tunWrite(char *buf, std::size_t len);

private:
    Tunnel(int fd): m_fd(fd) {}
    Tunnel(const Tunnel &);
    operator=(const Tunnel &);

    int m_fd;
};

#endif // _TUNNEL_H_

