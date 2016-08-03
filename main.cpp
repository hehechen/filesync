#include "syncserver.h"

int main()
{
    muduo::net::EventLoop loop;
    SyncServer server("/home/chen/Desktop/server/",&loop,8888); //最后要有/
    server.start();
    loop.loop();
    return 0;
}
