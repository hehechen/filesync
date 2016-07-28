#include "../socket.h"
#include "../sysutil.h"
#include "../protobuf/filesync.init.pb.h"
using namespace std;
using namespace sysutil;

int main()
{
    TcpSocket socket;
    socket.bind(8888);
    socket.listen();
    int connect_fd = socket.accept();
    char buf[10000];
    int n = recv(connect_fd,buf,10000,0);
    filesync::init msg;
    msg.ParseFromArray(buf,n);
    cout<<"id:"<<msg.id()<<"filename"<<msg.filename()<<endl;
    close(connect_fd);
    return 0;
}
