//#include "../socket.h"
//#include "../sysutil.h"
//#include "../protobuf/filesync.pb.h"
//#include <google/protobuf/descriptor.h>
//#include <google/protobuf/message.h>
//#include <string>

//using namespace std;
//using namespace sysutil;
//using namespace google::protobuf;

//inline int32_t asInt32(const char* buf)
//{
//  int32_t be32 = 0;
//  ::memcpy(&be32, buf, sizeof(be32));
//  return ::ntohl(be32);
//}
//Message* createMessage(const std::string& typeName)
//{
//  Message* message = NULL;
//  const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
//  if (descriptor)
//  {
//    const Message* prototype = MessageFactory::generated_factory()->GetPrototype(descriptor);
//    if (prototype)
//    {
//      message = prototype->New();
//    }
//  }
//  return message;
//}

//int main()
//{
//   TcpSocket socket;
//   socket.bind(8888);
//   socket.listen();
//   int connect_fd = socket.accept();
//   char buf[10000];
//   readn(connect_fd,buf,4);
//   int size = atoi(buf);
//   memset(buf,0,sizeof(buf));
//   readn(connect_fd,buf,size);
//   int namelen = asInt32(buf);
//   string typeName(&buf[4],namelen);
//   cout<<typeName<<endl;
//   // google::protobuf::Message* message = createMessage(string(buf,size));
//   // filesync::init msg;
//   // msg.ParseFromArray(buf,size);
//   // cout<<"id:"<<msg.id()<<"filename"<<msg.filename()<<endl;
//   close(connect_fd);
//   return 0;
//}


