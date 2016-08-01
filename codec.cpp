#include "codec.h"
#include <muduo/base/Logging.h>

#include <memory>
#include <zlib.h>  // adler32
#include <arpa/inet.h>  // htonl, ntohl
using namespace std;
Codec::Codec()
{

}

int asInt32(const char* buf)
{
    int be32 = 0;
    ::memcpy(&be32, buf, sizeof(be32));
    return ::ntohl(be32);
}

/**
 * @brief Codec::parse  解析数据
 * @param conn
 * @param inputBuffer
 */
void Codec::parse(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *inputBuffer)
{
    while (inputBuffer->readableBytes() >= kMinMessageLen + kHeaderLen)
    {
        const int len = inputBuffer->peekInt32();
        if (len < kMinMessageLen)
        {
            LOG_ERROR<<"parse protobuf error:invalid length";
            break;
        }
        else if (inputBuffer->readableBytes() >= muduo::implicit_cast<size_t>(len + kHeaderLen))
        {
            MessagePtr message = parse_aux(inputBuffer->peek()+kHeaderLen,len);
            if (message)
            {
                inputBuffer->retrieve(kHeaderLen+len);
                dispatch(conn,message);
            }
            else
            {
                LOG_ERROR<<"parse protobuf error";
                break;
            }
        }
        else
        {
            break;
        }
    }
}

/**
 * @brief Codec::parse_aux //解析数据得到相应的Message
 * @param buf   消息字符串，长度信息已被跳过
 * @param len   消息长度
 * @return
 */
MessagePtr Codec::parse_aux(const char* buf, int len)
{
    MessagePtr message;

    // check sum
    int expectedCheckSum = asInt32(buf + len - kHeaderLen);
    int checkSum = static_cast<int>(
                ::adler32(1,
                          reinterpret_cast<const Bytef*>(buf),
                          static_cast<int>(len - kHeaderLen)));
    if (checkSum == expectedCheckSum)
    {
        // get message type name
        int nameLen = asInt32(buf);
        if (nameLen >= 2 && nameLen <= len - 2*kHeaderLen)
        {
            std::string typeName(buf + kHeaderLen, buf + kHeaderLen + nameLen - 1);
            // create message object
            message.reset(createMessage(typeName));
            if (message)
            {
                // parse from buffer
                const char* data = buf + kHeaderLen + nameLen;
                int dataLen = len - nameLen - 2*kHeaderLen;
                if (!message->ParseFromArray(data, dataLen))
                {
                    LOG_ERROR<<"ParseError";
                }
            }
            else
            {
                LOG_ERROR<<"UnownMessageType";
            }
        }
        else
        {
            LOG_ERROR<<"InvalidNameLen";
        }
    }
    else
    {
        LOG_ERROR<<"CheckSumError";
    }

    return message;
}

void Codec::dispatch(const muduo::net::TcpConnectionPtr& conn, MessagePtr message)
{
    auto it = callbackMap.find(message->GetDescriptor());
    if (it != callbackMap.end())
    {
      it->second->onMessage(conn, message);
    }
    else
        LOG_ERROR<<message->GetTypeName()<<"didn't register callback";
}

/**
 * @brief Codec::encode 将要发送的信息打包
 * @param message   要发送的信息
 * @return 如果message.AppendToString失败则返回空字符串
 */
string Codec::enCode(const google::protobuf::Message& message)
{
    string result;

    result.resize(kHeaderLen);

    const string& typeName = message.GetTypeName();
    int32_t nameLen = static_cast<int32_t>(typeName.size()+1);
    int32_t be32 = ::htonl(nameLen);
    result.append(reinterpret_cast<char*>(&be32), sizeof be32);
    result.append(typeName.c_str(), nameLen);
    bool succeed = message.AppendToString(&result);

    if (succeed)
    {
        const char* begin = result.c_str() + kHeaderLen;
        int32_t checkSum = adler32(1, reinterpret_cast<const Bytef*>(begin), result.size()-kHeaderLen);
        int32_t be32 = ::htonl(checkSum);
        result.append(reinterpret_cast<char*>(&be32), sizeof be32);

        int32_t len = ::htonl(result.size() - kHeaderLen);
        std::copy(reinterpret_cast<char*>(&len),
                  reinterpret_cast<char*>(&len) + sizeof len,
                  result.begin());
    }
    else
    {
        result.clear();
    }

    return result;
}


google::protobuf::Message* Codec::createMessage(const std::string& typeName)
{
    google::protobuf::Message* message = NULL;
    const google::protobuf::Descriptor* descriptor =
            google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
    if (descriptor)
    {
        const google::protobuf::Message* prototype =
                google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype)
        {
            message = prototype->New();
        }
    }
    return message;
}
