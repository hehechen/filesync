#include "str_tool.h"

/**
 * 去除换行符
 * @param str 要处理的字符串
 */
void str_trim_crlf(char *str)
{
	char *p = &str[strlen(str)-1];
	while (*p == '\r' || *p == '\n')
		*p-- = '\0';

}
/**
 * 分割字符串，分割后两个子串不包含c
 * @param str   要处理的字符串
 * @param left  左子字符串
 * @param right 右子字符串
 * @param c     分割标志
 */
void str_split(char *str , char *left, char *right, char c)
{
	char *p = strchr(str, c);
	if (p == NULL)
		strcpy(left, str);
	else
	{
		*p = '\0';
		strcpy(left, str);
		strcpy(right, p+1);
	}
}
/**
 * 去除首尾空格
 * @param  str 要处理的字符串
 * @return 返回处理后的字符串
 */
char *str_delspace(char *str)
{
	char* end,*sp,*ep;//结尾指针，两个游标
	size_t len;//用于保存长度长度
	sp = str;//sp指针首先指向行首
	end = ep = str + strlen(str) -1;//end指针指向结尾
	//从行首开始去空格
	while(sp <= end && isspace(*sp))//是空格返回非零
		sp++;//移动sp指针直到指向的不是空格
	//从行尾去除空格
	while(ep >= sp && isspace(*ep))
		ep--;
	len = (ep < sp)? 0 :(ep - sp)+1;//计算长度+1,指向换行符
	sp[len] = '\0';//换行符置为'\0'
	return sp;
}

/**
 * 将字符串转换为大写
 * @param str 要处理的字符串
 */
void str_upper(char *str)
{
	while (*str)
	{
		*str = toupper(*str);
		str++;
	}
}

/**
 * 字符串表示的8进制数转换为unsigned int
 * @param  str 要处理的字符串
 * @return     unsigned int类型的整数
 */
unsigned int str_octal_to_uint(char *str)
{
	unsigned int result = 0;
	int seen_non_zero_digit = 0;

	while (*str)
	{
		int digit = *str;
		if (!isdigit(digit) || digit > '7')//8进制数最大值为7
			break;

		if (digit != '0')	//找到第一个不为0的字符
			seen_non_zero_digit = 1;

		if (seen_non_zero_digit)
		{
			result <<= 3;		//乘8
			result += (digit - '0');
		}
		str++;
	}
	return result;
}
/**
 * 将字符串转换为long long类型的数
 * @param  str 要处理的字符串
 * @return     long long类型的数
 */
long long str_to_longlong(const char *str)
{
	//return atoll(str);
/*	long long result = 0;
	long long mult = 1;
	unsigned int len = strlen(str);
	unsigned int i;

	if (len > 15)
		return 0;


	for (i=0; i<len; i++)
	{
		char ch = str[len-(i+1)];
		long long val;
		if (ch < '0' || ch > '9')
			return 0;

		val = ch - '0';
		val *= mult;
		result += val;
		mult *= 10;
	}
	*/

	long long result = 0;
	long long mult = 1;
	unsigned int len = strlen(str);
	int i;

	if (len > 15)
		return 0;

	for (i=len-1; i>=0; i--)
	{
		char ch = str[i];
		long long val;
		if (ch < '0' || ch > '9')
			return 0;

		val = ch - '0';
		val *= mult;
		result += val;
		mult *= 10;
	}

	return result;
}
