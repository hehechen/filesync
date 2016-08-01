#ifndef CODEC_H
#define CODEC_H

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <muduo/net/Buffer.h>
#include <muduo/net/TcpConnection.h>
#include <google/protobuf/descriptor.h> 
#include <google/protobuf/message.h> 


//Message* 创建后需要释放，用智能指针管理
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class Callback
{
 public:
  virtual ~Callback() {}
  virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                         const MessagePtr& message) const = 0;
};

template <typename T>
/**
 * @brief The CallbackT class  利用模板保存不同类型message的回调函数，继承自一个非目标类便于map保存
 */
class CallbackT : public Callback
{
public:
  typedef std::function<void (const muduo::net::TcpConnectionPtr&,
                              const std::shared_ptr<T>& message)> ProtobufMessageTCallback;

  CallbackT(const ProtobufMessageTCallback& callback)
    : callback_(callback)
  {
  }

  virtual void onMessage(const muduo::net::TcpConnectionPtr& conn,
                         const MessagePtr& message) const
  {
    std::shared_ptr<T> concrete = std::static_pointer_cast<T>(message);
    assert(concrete != NULL);
    callback_(conn, concrete);
  }
 private:
  ProtobufMessageTCallback callback_;
};


/**
协议的格式
struct Message{
int len;
int namelen;
char typename[namelen];
char protobufData[len-namelen];
int checkSum;	//用adler32产生校验码，根据nameLen,typeName和protobufData产生
};
*/
class Codec
{
public:
    Codec();
    //注册消息类型对应的回调函数
    template<typename T>
    void registerCallback(const typename CallbackT<T>::ProtobufMessageTCallback& callback)
    {
        std::shared_ptr<CallbackT<T> > pd(new CallbackT<T>(callback));
        callbackMap[T::descriptor()] = pd;
    }
    //将要发送的信息打包，打包方式如上的注释
    std::string enCode(const google::protobuf::Message& message);    
    //解析数据并执行相应的回调函数
    void parse(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *inputBuffer);
private:
    static const int kHeaderLen =  4;
    static const int kMinMessageLen = 2*kHeaderLen + 2; // nameLen + typeName + checkSum
    //根据packege.message来获得相应的Message
    google::protobuf::Message* createMessage(const std::string& typeName);
    std::unordered_map<const google::protobuf::Descriptor*,
                                std::shared_ptr<Callback> > callbackMap;
    //解析数据得到相应的Message
    MessagePtr parse_aux(const char *buf, int len);
    //根据消息类型执行相应的回调函数
    void dispatch(const muduo::net::TcpConnectionPtr &conn, MessagePtr message);
};

#endif // CODEC_H
