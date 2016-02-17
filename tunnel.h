#ifndef _TUNNEL_H_
#define _TUNNEL_H_

#include <string>
#include <cstddef>

class Tunnel
{
public:
    static Tunnel *allocTun(const std::string &devName);
    ~Tunnel();
    bool start();
    void stop();
    std::size_t read(char *buf, std::size_t len);
    std::size_t write(const char *buf, std::size_t len);
    std::string getDevName() const;

private:
    Tunnel(const char *devName, int fd);
    Tunnel(const Tunnel &);
    operator=(const Tunnel &);

    std::string m_devName;
    int m_fd;
};

#endif // _TUNNEL_H_

