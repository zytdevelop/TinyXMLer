#ifndef TINYXML2_INCLUDED
#define TINYXML2_INCLUDED
#include <cctype>   //字符处理库
#include <climites>    //类型支持库
#include <cstdio>    //标准输入输出流库
#include <cstdlib>    //杂项工具支持库
#include <cstring>    //字符串标准库
#include <stdint.h>    //位宽整形库



//允许动态库导出，此方法对其他模块可见
#define TIXML2_LIB    __attribute__((visibility("default")))



//诊断宏，判断是否为零值
#define TIXMLASSERT( x )    {}


//限制元素深度，避免堆栈溢出
static const int TINYXML2_MAX_ELEMENT_DEPTH = 100;

namespace tinyxml2{
    class XMLDocument;
    class XMLElement;
    class XMLAttribute;
    class XMLComment;
    class XMLText;
    class XMLDeclaration;
    class XMLUnknown;
    class XMLPrinter;


    //警告代码，需匹配相应的名称
    enum XMLError{
        XML_SUCCESS = 0;
        XML_NO_ATTRIBUTE,
        XML_WRONG_ATTRIBUTE_TYPE,
        XMLERROR_FILE_NOT_FOUND,
        XML_ERROR_FILE_CLOD_NOT_BE_OPENED,
        XML_ERROR_READ_ERROR,
        XML_ERROR_PARSING_ELEMENT,
        XML_ERROR_PARSING_ATTRIBUTE,
        XML_ERROR_PARSING_TEXT,
        XML_ERROR_PARSING_CDATA,
        XML_ERROR_PARSING_COMMENT,
        XML_ERROR_PARSING_DECLARATION,
        XML_ERROR_PARSING_UNKNOWN,
        XML_ERROR_EMPTY_DOCUMENT,
        XML_ERROR_MISMATCHED_ELEMENT,
        XML_ERROR_PARSING,
        XML_CAN_NOT_CONVERT_TEXT,
        XML_NO_TEXT_NODE,
        XML_ELEMENT_DEPTH_EXCEEDED,

        XML_ERROR_COUNT
    };

    //code
}



#endif







