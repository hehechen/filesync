#include "syncserver.h"

int main()
{
    muduo::net::EventLoop loop;
    SyncServer server(&loop,8888);
    server.start();
    loop.loop();
    return 0;
}
