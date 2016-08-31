#include "rsync.h"
#include <fstream>
#include <stdio.h>
#include "sysutil.h"
using namespace std;

Rsync::Rsync(string &filename):filename(filename)
{

}

void Rsync::addHashItem(int adler32, DataBlockPtr dataBlockPtr)
{
    adler_blockMaps[adler32] = dataBlockPtr;
}
//找到匹配的数据块后就会从adler_blockMaps中删除，所以最后adler_blockMaps剩下的就是空洞链表
void Rsync::calRsync()
{
    int ad;//adler32和md5
    string md;
    int firstCh,lastCh;
    FILE *fp = fopen(filename.c_str(),"rb");
    /*获取需要计算的块数，并分块计算，输出到文件*/
    fseek(fp, 0, SEEK_END);//获取文件尾
    int end = ftell(fp);
    int size = end / BLOCK_SIZE;
    rewind(fp);//重置文件指针
    char* buff = (char*)malloc(sizeof(char)*(BLOCK_SIZE+1 ));
    for(int i=0;i<size;i++)
    {
        fread(buff,sizeof(char), BLOCK_SIZE, fp);//每次循环开始，先读取一块数据计算校检和
        buff[BLOCK_SIZE] = '\0';
        ad = sysutil::adler32(buff,BLOCK_SIZE);
        auto it = adler_blockMaps.find(ad);
        if(it != adler_blockMaps.end())
        {
            md = sysutil::getStringMd5(buff);
            if(md == it->second->md5)
            {
                DataBlockPtr dataBlockPtr(new DataBlock(i*BLOCK_SIZE,
                                                        it->second->s_offset,ad,md));
                dataList.push_back(dataBlockPtr);
                adler_blockMaps.erase(it);
            }
            else
                goto roll;
        }
        else
        {
roll:
            for (int in = 0; in < BLOCK_SIZE; in++)
            {
                moveCh(&firstCh, &lastCh, fp,buff);
                ad = sysutil::adler32_rolling_checksum(ad, BLOCK_SIZE, firstCh, lastCh);//滚动后的校验和
                if (in == BLOCK_SIZE - 1)
                {
                    fseek(fp, -BLOCK_SIZE, SEEK_CUR);
                    break;//跳出本次循环
                }
                auto it = adler_blockMaps.find(ad);
                if (it!=adler_blockMaps.end())
                {
                    md = sysutil::getStringMd5(buff);
                    if (it->second->md5 == md)//如果MD5也匹配
                    {
                        DataBlockPtr dataBlockPtr(new DataBlock(in+i*BLOCK_SIZE,
                                                                it->second->s_offset,ad,md));
                        dataList.push_back(dataBlockPtr);
                        break;//跳出本次循环
                    }
                    adler_blockMaps.erase(it);
                }
            }
        }
    }
    fclose(fp);
    //生成空洞链表
    for(auto it = adler_blockMaps.begin();it!=adler_blockMaps.end();++it)
    {
        HoleInfoPtr holeInfoPtr(new HoleInfo(it->second->s_offset,BLOCK_SIZE));
        holeList.push_back(holeInfoPtr);
    }
    holeList.sort([](HoleInfoPtr p1,HoleInfoPtr p2){
        return p1->offset < p2->offset;
    });
    dataList.sort([](DataBlockPtr p1,DataBlockPtr p2){
        return p1->s_offset < p2->s_offset;
    });
    free(buff);
}

void Rsync::reconstruct(char *data, int len,string temFilename)
{
    FILE *s_fp = fopen(filename.c_str(),"rb");
    FILE *t_fp = fopen(temFilename.c_str(),"wb");
    char buf[BLOCK_SIZE+1];
    while(len > BLOCK_SIZE)
    {
        auto it_hole = holeList.begin();
        auto it_data = dataList.begin();
        if(!dataList.empty() && (*it_data)->s_offset < (*it_hole)->offset)
        {//从已有文件复制过去
            fseek(s_fp,(*it_data)->t_offset,SEEK_SET);
            fread(buf,sizeof(char),BLOCK_SIZE,s_fp);
            fwrite(buf,sizeof(char),BLOCK_SIZE,t_fp);
            dataList.pop_front();
        }
        else
        {
            fwrite(data,sizeof(char),BLOCK_SIZE,t_fp);
            holeList.pop_front();
            len -= BLOCK_SIZE;
            data += BLOCK_SIZE;
        }
    }
    while(!dataList.empty())
    {//从已有文件复制过去
        auto it_data = dataList.begin();
        fseek(s_fp,(*it_data)->t_offset,SEEK_SET);
        fread(buf,sizeof(char),BLOCK_SIZE,s_fp);
        fwrite(buf,sizeof(char),BLOCK_SIZE,t_fp);
        dataList.pop_front();
    }
    //不足一个BLOCK_SIZE的都写到文件尾
    fwrite(data,sizeof(char),len,t_fp);
    fclose(s_fp);
    fclose(t_fp);
}

//将fp指针向后挪一个字节
void Rsync::moveCh(int* firstCh, int* lastCh, FILE * fp , char * buff)
{
    fseek(fp, -BLOCK_SIZE, SEEK_CUR);
    *firstCh = fgetc(fp);
    fread(buff, sizeof(char), BLOCK_SIZE, fp);
    *lastCh = buff[BLOCK_SIZE-1];
}
