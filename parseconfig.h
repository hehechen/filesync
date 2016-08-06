//解析配置文件的类
#ifndef _PARSECONFIH_H
#define _PARSECONFIH_H

#include "common.h"
#include <unordered_map>
#include <string>
//饿汉模式单例类
class ParseConfig
{
public:
	//返回单例对象
    static ParseConfig *getInstance();
	void loadfile();	//加载配置文件
	//获取配置数据
    std::string getSyncRoot()   {return syncRoot;}
    //检查用户名和密码是否正确
    bool checkUser(std::string &username,std::string &pw)
    {
        auto it = userInfos.find(username);
        if(it != userInfos.end())
        {
            if(it->second == pw)
                return true;
        }
        return false;
    }

private:
	static ParseConfig *m_instance;
	static void destroy();	//销毁该单例
	const char* CONFILE;//配置文件的文件名
	static const int MAX_CONF_LEN = 1024;
    ParseConfig();
	~ParseConfig();

    std::string syncRoot;   //服务端的同步目录
    std::unordered_map<std::string,std::string> userInfos;  //用户名和密码
};

#endif
