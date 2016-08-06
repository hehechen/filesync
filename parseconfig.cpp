#include "parseconfig.h"
#include "str_tool.h"
//在进入主函数前就初始化，保证线程安全	
ParseConfig *ParseConfig::m_instance = new ParseConfig();
ParseConfig *ParseConfig::getInstance()
{
    return m_instance;
}

ParseConfig::ParseConfig():CONFILE("syncserver.conf")
{
    atexit(destroy);    //在程序退出时销毁
}
ParseConfig::~ParseConfig()
{
}

void ParseConfig::destroy()
{
    if(m_instance)
    {
        delete m_instance;
        m_instance = nullptr;
        CHEN_LOG(DEBUG,"ParseConfig destroy");
    }
}
/**
 * 加载配置文件
 */
void ParseConfig::loadfile()
{
    FILE* fp = fopen(CONFILE,"r");	//只读模式打开配置文件
    if(fp == NULL)
        CHEN_LOG(ERROR,"open confile error");

    int linenumber = 0;//这个参数用于记录配置文件的行数，便于在配置文件出错时打印提醒
    char* line;//行指针
    char key[128] = {0};//保存命令的key
    char value[128] = {0};//保存命令的value
    char linebuf[MAX_CONF_LEN+1];//保存读取到的行
    while(fgets(linebuf,sizeof(linebuf),fp))
    {
        ++linenumber;//行数++
        line = str_delspace(linebuf);//去除行首和行尾的空格、\r\n
        if (line[0] == '#' || line[0] == '\0')
        {
            continue;//注释或空行直接跳过
        }
        str_split(line,key,value,'=');//根据等号切割
        strcpy(key,str_delspace(key));//去除空格
        strcpy(value,str_delspace(value));//去除空格
        if(0 == strlen(value))
        {//某些key没有配置value,提示key和行号
            CHEN_LOG(WARN,"missing value in %s for %s locate line %d",CONFILE,key,linenumber);
        }
        //strcasecmp忽略大小写比较字符串
        if(strcasecmp(key, "syncroot") == 0)//服务端同步路径
        {
            syncRoot = value;
            CHEN_LOG(DEBUG,"SYNCROOT:%s",value);
        }
        else
        {//用户名和密码
            userInfos[string(key)] = std::move(string(value));
            CHEN_LOG(DEBUG,"user:%s,pw:%s",key,value);
        }
    }
    //关闭配置文件
    fclose(fp);
}






