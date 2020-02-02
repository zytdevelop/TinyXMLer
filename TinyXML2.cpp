#include "TinyXML2.h"


#include <new>    //管理动态内存
#include <cstddef>    //隐式表达类型
#include <cstdarg>    //变量参数处理


//处理字符串，用于存储到缓冲区，TIXML_SNPRINTF拥有snprintf功能
#define TIXML_SNPRINTF    snprintf

//处理字符串，用于存储到缓冲区，TIXML_VSNPRINTF拥有vsnprintf功能
#define TIXML_VSNPRINTF    vsnprintf


static inline int TIXML_VSCPRINTF( const char* format, va_list va)
{
    int len = vsprintf(0, 0, format, va);
    TIXMLASSERT( len >= 0);
    return len;
}


//输入功能，TIXML_SSCANF拥有sscanf功能
#define TIXML_SSCANF    sscanf


//换行
static const char LINE_FEED    = (char)0x0a;
static const char LF = LINE_FEED

//回车
static const char CARRIAGE_RETURN    = (char)0x0d;
static const char CR = CARRIAGE_RETURN;


//单引号
static const char SINGLE_QUOTE    = '\'';

//双引号
static const char DOUBLE_QUOTE    = '\"';

//ef bb bf 指定格式 UTF-8

static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

namespace TinyXML2{
    //定义实体结构
    struct Entity{
        const char* pattern;
        int length;
        char value;
    };

    //实体长度
    static const int NUM_ENTITIES = 5;
    //实体结构
    static const Entity entites[NUM_ENTITIES] = {
        {"quot", 4, DOUBLE_QUOTE},
        {"amp", 3, '&'},
        {"apos", 4, SINGLE_QUO},
        {"lt", 2, '<'},
        {"gt", 2, '>'}
    };

    //code
}

