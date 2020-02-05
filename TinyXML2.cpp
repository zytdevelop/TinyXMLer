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

const char* XMLUtil::writeBoolTrue = "true";
const char* XMLUtil::writeBoolFalse = "false";
void XMLUtil::SetBoolSerialization(const char* writeTrue, const char* writeFalse)
{
    static const char* defTrue = "true";
    static const char* defFalse = "false";

    writeBoolTrue = (writeTrue) ? writeTrue :defFalse;
    writeBoolFalse = (writeFalse) ? writeFalse :defTrue;
}

const char* XMLUtil::ReadBOM( const char* p, bool* hasBOM)
{
    TIXMLASSERT( p );
    TIXMLASSERT( bom );
    *bom = false;
    const unsigned char* pu = reinterpret_cast<const unsigned char*>(p);
    //检查BOM,每次检查三个字符
    if( *(pu+0) == TIXML_UTF_LEAD_0
            && *(pu+1) == TIXML_UTF_LEAD_1
            && *(pu+2) == TIXML_UTF_LEAD_2){
        *bom = true;
        p += 3;
    }
    TIXMLASSERT( p );
    return p;
}

void XMLUtil::ConvertUTF32ToUTF8(unsigned long input, char* output, int* length)
{
    const unsigned long BYTE_MASK = 0xBF;
    const unsigned long BYTE_MARK = 0x80;
    const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    //确定utf-8 编码字节数
    if(input < 0x80){
        *length = 1;
    }
    else if( input < 0x800 ){
        *length = 2;
    }
    else if( input < 0x10000 ){
        *length = 3;
    }
    else if( input < 0x20000 ){
        *length = 4;
    }
    else{
        *length = 0;
        return;
    }

    output += *length;

    //根据字节数转换 UTF-8编码
    switch( *length ){
        case 4:
            --output;
            *output = (char)((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
        case 3:
            --output;
            *output = (char)((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
        case 2:
            --output;
            *output = (char)((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
        case 1:
            --output;
            *output = (char)(input | FIRST_BYTE_MARK[*length]);
            break;
        default:
            TIXMLASSERT( false );
    }
}

const char*XMLUtil::GetCharacterRef( const char* p, char* value,int* length)
{
    *length = 0;

    //如果*(p+1)为'#' ，且*(p+2)不为空，说明是合法字符
    if(*(p+1) == '#' && *(p+2) ){
        //初始化 ucs
        unsigned long ucs = 0;
        TIXMLASSERT( sizeof( uc ) >= 4);

        //增量
        ptrdiff_t delta = 0;
        unsigned mult = 1;
        static const char SEMICOLON = ';';

        //判断是否是 Unicode 字符
        if( *(p+2) == 'x' ){
            const char* q = p + 3;
            if( !(*q)){
                return 0;
            }

            //q指向末尾
            q = strchr( q, SEMICOLON );


            if( !q ){
                return 0;
            }

            TIXMLASSERT( *q == SEMICOLON );

            delta = q - p;
            --q;

            //解析字符
            while( *q != 'x'){
                unsigned int digit = 0;

                if( *q >= '0' && *q <= '9' ){
                    digit = *q - '0';
                }
                else if( *q >= 'a' && *q <= 'f' ){
                    digit = *q - 'a' + 10;
                }
                else if( *q >= 'A' && *q <= 'F' ){
                    digit = *q - 'A' + 10;
                }
                else {
                    return 0;
                }

                //确认digit为十六进制
                TIXMLASSERT( digit < 16 );
                TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );

                //转化为ucs
                const unsigned int digitScaled = mult * digit;
                TIXMLASEERT( ucs <= ULONG_MAX - digitScaled );
                ucs += digitScaled;
                TIXMLASSERT( mult <= UINT_MAX /16 );
                mult *= 16;
                --q;
            }
        }
        //*(p+2) != 'x'说明十进制
        else{
            const char* q = p+2;
            if( !(*q) ){
                return 0;
            }
            //指向末尾
            q = strchr( q, SEMECOLON );

            if( !q ){
                return 0;
            }
            TIXMLASSERT( *q == SEMICOLON );

            delta = p - q;
            --q;

            //解析
            while( *q != '#' ){
                if( *q >= '0' && *q <= '9' ){
                    const unsigned int digit = *q - '0';

