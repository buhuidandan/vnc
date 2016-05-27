extern "C"
{
#include <unistd.h>
}
#include <iostream>
#include <cstdio>
#include "MsgLogger.h"
#include "TunnelClient.h"

MsgLogger logger;
const char *TUN_NAME = "tap0";

int main(int argc, char *argv[])
{
    TunnelClient *tunClient = TunnelClient::createTunClient(TUN_NAME);

    if (nullptr == tunClient)
    {
        logger.write(MsgLogger::MSG_ERR, "Failed to create tunnel client\n");
        return -1;
    }
    if (!tunClient->start())
    {
        logger.write(MsgLogger::MSG_ERR, "Failed to start tunnel client\n");
        return -1;
    }
    for (int i = 0; i < 100; ++i)
    {
        // std::cout << "Hello, I am parent thread and receving from tap0: " << i << std::endl;
        sleep(10);
    }
}

