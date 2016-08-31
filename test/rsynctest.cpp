//#include "rsync.h"
//#include "sysutil.h"
//#include <stdio.h>
//#include <string.h>
//using namespace std;
//#define Block_Size 2

//list<DataBlockPtr> dataList;
//char* holeData = (char*)malloc(sizeof(char)*(Block_Size*1024)); //1MB

//void calSrcFile(string filename)
//{
//    FILE *fp = fopen(filename.c_str(),"rb");
//    /*获取需要计算的块数，并分块计算，输出到文件*/
//    fseek(fp, 0, SEEK_END);//获取文件尾
//    int end = ftell(fp);
//    int size = end / Block_Size;
//    rewind(fp);//重置文件指针
//    char* buff = (char*)malloc(sizeof(char)*(Block_Size+1 ));
//    for(int i=0;i<size;i++)
//    {
//        fread(buff,sizeof(char),Block_Size,fp);
//        buff[Block_Size] = '\0';
//        int adler32 = sysutil::adler32(buff,Block_Size);
//        string md5 = sysutil::getStringMd5(buff);
//        DataBlockPtr dataPtr(new DataBlock(-1,i*Block_Size,adler32,md5));
//        dataList.push_back(dataPtr);
//    }
//    CHEN_LOG(DEBUG,"list size:%d",dataList.size());
//    fclose(fp);
//}
////根据空洞链表发送服务端所缺失的数据
//int sendData(string &filename,list<HoleInfoPtr> holeInfoList)
//{
//    FILE *fp = fopen(filename.c_str(),"rb");
//    int len = 0;
//    char *tmp = holeData;
//    for(auto it:holeInfoList)
//    {
//        fseek(fp,it->offset,SEEK_SET);
//        fread(tmp,sizeof(char),it->len,fp);
//        tmp += it->len;
//        len += it->len;
//    }
//    fseek(fp, 0, SEEK_END);//获取文件尾
//    int end = ftell(fp);
//    int num = end / Block_Size;
//    int size = end % Block_Size;
//    fseek(fp,num*Block_Size,SEEK_SET);
//    fread(tmp,sizeof(char),size,fp);
//    return len+size;
//}

//int main()
//{
//    string filename("/home/chen/Desktop/a.cpp");
//    string t_filename("/home/chen/Desktop/hoho.cpp");
//    calSrcFile(filename);
//    Rsync rsync(t_filename);
//    for(auto it:dataList)
//        rsync.addHashItem(it->adler32,it);
//    rsync.calRsync();
//    memset(holeData,0,Block_Size*1024);
//    int len = sendData(filename,rsync.getHoleList());
//    rsync.reconstruct(holeData,len,"/home/chen/Desktop/hohohoh.cpp");
//}
