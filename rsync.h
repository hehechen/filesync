#ifndef RSYNC_H
#define RSYNC_H

#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <memory>
#include <list>
//文件数据块,即服务端和客户端都有的块
struct DataBlock
{
    int t_offset;       //此数据块在服务端文件中的offset
    int s_offset;       //此数据块在客户端文件中的offset
    int  adler32;
    std::string md5;
    DataBlock(int t_offset,int s_offset,int adler32,std::string &md5):
        t_offset(t_offset),s_offset(s_offset),adler32(adler32),md5(md5)    {}
};
//空洞块，即服务端没有客户端有的块
struct HoleInfo
{
    int offset;
    int len;
    HoleInfo(int offset,int len):offset(offset),len(len)    {}
};

typedef std::shared_ptr<HoleInfo> HoleInfoPtr;
typedef std::shared_ptr<DataBlock> DataBlockPtr;

class Rsync
{
public:
    Rsync(std::string &filename);
    //添加到adler_blockMaps
    void addHashItem(int adler32,DataBlockPtr dataBlockPtr);
    //添加完adler_blockMaps后执行Rsync算法，生成空洞链表
    void calRsync();
    //将空洞信息发送给客户端
    void sendHoleInfo();
    //生成新文件
    void reconstruct(char *data, int len, std::__cxx11::string temFilename);

    //仅供测试
    std::list<HoleInfoPtr> getHoleList()    {return holeList;}
private:
    std::string filename;
    static const int BLOCK_SIZE = 2;
    void rollChecksum();
    //空洞链表
    std::list<HoleInfoPtr> holeList;
    //服务端和客户端都有的数据块的链表,offset以服务端为准
    std::list<DataBlockPtr> dataList;
    //客户端的adler32和数据块的映射
    std::unordered_map<int,DataBlockPtr> adler_blockMaps;
    unsigned int adler32(char *data, int len);
    unsigned int adler32_rolling_checksum(unsigned int csum, int len, char c1, char c2);
    void moveCh(int *firstCh, int *lastCh, FILE *fp, char *buff);
};

#endif // RSYNC_H
