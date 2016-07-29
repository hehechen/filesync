//#include "../socket.h"
//#include "../sysutil.h"
//#include "../protobuf/filesync.init.pb.h"
//using namespace std;
//using namespace sysutil;

//int main()
//{
//    TcpSocket socket;
//    socket.bind(8888);
//    socket.listen();
//    int connect_fd = socket.accept();
//    char buf[10000];
//    readn(connect_fd,buf,4);
//    int size = atoi(buf);
//    memset(buf,0,sizeof(buf));
//    readn(connect_fd,buf,size);
//    filesync::init msg;
//    msg.ParseFromArray(buf,size);
//    cout<<"id:"<<msg.id()<<"filename"<<msg.filename()<<endl;
//    close(connect_fd);
//    return 0;
//}


