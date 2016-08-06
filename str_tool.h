#ifndef STR_TOOL_H
#define STR_TOOL_H
#include "common.h"

using namespace std;
//去除行首尾的空格和换行符
void str_trim_crlf(char *str);
void str_split(char *str , char *left, char *right, char c);
char *str_delspace(char *str);
void str_upper(char *str);
unsigned int str_octal_to_uint(char *str);
long long str_to_longlong(const char *str);

#endif // STR_TOOL_H
