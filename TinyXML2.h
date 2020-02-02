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
template <class T, int INIITAL_SIZE>
class DynArray{  //动态存储数据
public:
    //code
    //构造函数
    DynArray():_mem(_pool), _allocated( INITIAL_SIZE), _size(0){}

    //析构函数
    ~DynArray(){
        if(_mem != _pool){
            delete []_mem;
        }
    }
    //清空元素
    void Clear(){
        _size = 0;
    }

    //在数组末尾添加元素
    void Push( T t){
        TIXMLASSERT( _size < INT_MAX);
        //检查空间容量
        EnsureCapacity( _size+1 );
        _mem[_size] = t;
        ++_size;
    }

    //添加段
    T* PushArr( int count){
        TIXMLASSERT( count >= 0);
        TIXMLASSERT( _size <= INI_MAX - count);
        //检查并申请内存
        EnsureCapacity( _size + count );
        //添加段
        T* ret = &_mem[_size];
        _size += count;
        //返回添加后的数组
        return ret;
    }

    //返回并删除末尾元素
    T Pop(){
        TIXMLASSERT( _size > 0);
        --_size;
        return _mem[_size];
    }

    //删除段
    void PopArr( int count){
        TIXMLASSERT( _size >= count);
        _size -= count;
    }

    //判断是否为空
    bool Empty() const{
        return _size == 0;
    }

    //重载下标符
    T& operator[](int i){
        TIXMLASSERT( i >= 0 && i< _size );
        return _mem[i];
    }

private:
    //code
    T* _mem;
    T _pool[INITIAL_SIZE];    //内存池
    int _allocated;    //分配器
    int _size;    //元素数量

    //拷贝构造和赋值重载
    DynArray(const DynArray& );    //不需要实现
    void operator=( const DynArray&);    //不需要实现
    void EnsureCapacity( int cap ){
        //确定cap大于零
        TIXMLASSERT( cap > 0);

        //如果内存池已满，则申请空间
        if( cap > _allocated ){
            TIXMLASSERT( cap <= INT_MAX / 2);

            //申请新的内存池来接收，每次申请两倍空间
            int newAllocated = cap * 2;
            T* newMem = new T[newAllocated];
            TIXMLASSERT(newAllocated >= _size);

            //替换数据
            memcpy( newMem, _mem, sizeof(T)* _size);

            //释放 _mem
            if(_mem != _pool){
                delete []_mem;
            }
            _mem = newMem;
            _allocated = newAllocated;
    }
};



#endif







