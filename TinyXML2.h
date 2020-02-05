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
    class TINYXML_LIB_XMLUtil{
    public:
        //code
       static void SetBoolSerialization(const char* writeTrue, const char* writeFalse);

       //判断编码格式
       //本程序以UTF-8为基础
       inline static bool IsUTF8Continuation(char p){
           return (p & 0x80 ) != 0;
       }

       //判断空白
       //需要用到<ctype> isspace(),功能是:如果参数是除字母、数字和空格外可打印字符
       //函数返回非零，否则返回零
       static bool IsWhiteSpace( char p ){
           return !IsUTF8Continuation(p) && isspace( static_cast<unsigned char>(p) );
       }


       //跳过空白
       static const char* SkipWhiteSpace( const char* p, int* curLineNumPtr ){
           //确定参数不为空
           TIXMLASSERT( p );

           //如果是空白,则需考虑换行
           while( IsWhiteSpace(*p) ){
               if(curLineNumptr && *p == '\n'){
                   ++(*curLineNumptr);
               }
               ++p;
           }
           TIXMLASSERT( p );
           return p;
       }


       //跳过空白
       static char* SkipWhiteSpace( char* p, int* curLineNumPtr ){
           //结果类型和参数强制转换为常量
           return const_cast<char*>( SkipWhiteSpace( const_cast<const char*>(p), curLineNumPtr ) );
       }

       //判断名称首位
       /* XML元素命名规则:
        * 可以包含字母、数字及其它字母
        * 不能以数字开头或下划线开头
        * 不能用XML开头
        * 不能包含空格和冒号
        */
       inline static bool IsNameStartChar( unsigned char ch ){
           if( ch >= 128 ){
               return true;
           }
           if( isalpha( ch ) ){
               return true;
           }
           return ch == ':' || ch == '_';
       }




    private:
        //code
        static const char* writeBoolTrue;
        static const char* writeBoolFalse;

    };


    //父虚类,辅助快速的分配和释放内存，只作继承所以不用实现
    class MemPool
    {
    public:
        MemPool(){}
        virtual ~MemPool(){}

        //纯虚函数
        virtual int ItemSize() const = 0;
        virtual void* Alloc() = 0;
        virtual void Free(void*) = 0;
        virtual void SetTracked() = 0;
    };


    //内存池子类，用于创建符合大小要求的内存池
    template< int ITEM_SIZE>
    class MemPoolT : public MemPool{
    public:
        //code
        //定义每个块的大小
        enum { ITEMS_PER_BLOCK = (4 * 1024) / ITEM_SIZE};

        //构造函数和析构函数
        MemPoolT() : _blockPtrs(), _root(0), _currentAllocs(0), nAllocs(0), _maxAllocs(0), _nUntracked(0){}
        ~MemPoolT(){
            MemPoolT< ITEM_SIZE >::Clear();
        }

        //清空内存
        void Clear(){
            //删除块
            while( !_blockPtrs.Empty()){
                Block* lastBlock = _blockPtrs.Pop();
                delete lastBlock;
            }

            //初始化
            _root = 0;
            _currentAllocs = 0;
            _nAllocs = 0;
            _maxAllocs = 0;
            _nUntracked = 0;
        }

        //内存块数
        virtual int ItemSize() const {
            return ITEM_SIZE;
        }

        //当前分配数
        int CurrentAllocs() const {
            return _currentAllocs;
        }

        //分配器
        virtual void* Alloc() {
            if( !_root ){
                //需要一个新的块
                Block* block = new Block();
                _blockPtrs.Push( block );

                //把块的内容复制到节点
                Item* blockItems = block->items;
                for(int i = 0; i < ITEMS_PER_BLOCK - 1; ++i){
                    blockItems[i].next = &(blockItems[i + 1];
                }
                blockItems[ITEMS_PER_BLOCK - 1].next = 0;
                _root = blockItems;
            }

            //设置结果为常量
            Item* const result = _root;
            TIXMLASSERT( result != 0 );
            _root = _root->next;


            //更新状态
            ++_currentAllocs;
            if( _currentAllocs > _maxAllocs ){
                _maxAllocs = _currentAllocs;
            }
            ++_nAllocs;
            ++_nUntracked;
            return result;
        }

        //释放内存
        virtual void Free( void* mem){
            //为空，返回
            if( !mem ){
                return;
            }

            //当前分配数
            --_currentAllocs;
            //强制转化为空间节点类型
            Item* item = static_cast<Item*>( mem );
            //如果采用DEBUG调试，item全设置为0xfe
        #ifdef TINYXML2_DEBUG
            memset( item, 0xfe, sizeof( *item ) );
        #endif
            //清空item
            item->next = _root;
            _root = item;
        }

        //  跟踪状态信息
        //  打印当前所有状态信息
        void Trace( const char* name ){
            printf("Mempool %s watermark=%d [%dk] current=%d size=%d nAlloc=%d blocks=%d\n", name, _maxAllocs * ITEM_SIZE / 1024, _currentAllocs, ITEM_SIZE, _nAllocs, _blockPtrs.Size());
        }

        //更新跟踪状态
        void SetTracked(){
            --_nUntracked;
        }

        //返回跟踪状态
        int Untracked() const {
            return _nUntracked;
        }


    private:
        //code
        MemPoolT( const MemPoolT& );   //不实现
        void operator=( const MemPoolT& );    //不实现


        //空间节点联合体
        union Item{
            Item* next;
            char itemData[ITEM_SIZE];
        };
        //内存块结构体
        struct Block{
            Item items[ITEM_PER_BLOCK];
        };

        //定义一个动态数组
        DynArray< Block*, 10> _blockPtrs;

        //定义根节点
        Item* _root;


        int _currentAllocs;     //当前分配数
        int _nAllocs;           //分配总次数
        int _maxAllocs;         //最大分配数
        int _nUntracked;        //未跟踪次数

    };

    //定义访问接口
    class TINYXML2_LIB XMLVisitor{
    public:
        //code
        //此类中的函数全是用虚函数，不需要构造函数
        virtual ~XMLVisitor(){}

        //进入
        virtual bool VisitEnter( const XMLDocument& ){
            return true;
        }
        virtual bool VisitEnter( const XMLElement&, const XMLAttribute* ){
            return true;
        }
        //退出
        virtual bool VisitExit( const XMLDocument& ){
            return true;
        }
        virtual bool VisitExit( const XMLElement& ){
            return true;
        }

        //访问声明
        virtual bool Visit( const XMLDeclaration& ){
            return true;
        }

        //访问文本节点
        virtual bool Visit( const XMLText& ){
            return true;
        }

        //访问注释
        virtual bool Visit( const XMLComment& ){
            return true;
        }

        //访问未知节点
        virtual bool Visit( const XMLUnknow& ){
            return true;
        }

        //
    };




    }
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

        //查看末尾元素
        const T& PeekTop(){
            TIXMLASSERT( _size > 0 );
            return _mem[_size - 1];
        }

        //元素数量
        int Size() const {
            TIXMLASSERT( _size >= 0);
            return _size;
        }

        //空间容量
        int Capacity() const {
            TIXMLASSERT( _allocated >= INITIAL_SIZE );
            return _size;
        }

        //交换删除
        //将需要删除的元素与末尾元素交换，然后删除
        void SwapRemove(int i){
            TIXMLASSERT(i >= 0 && i < _size);
            TIXMLASSERT(_size > 0);
            _mem[i] = _mem[_size - 1];
            --_size;
        }

        //返回数组元素
        T* Mem(){
            TIXMLASSERT( _mem );
            return _mem;
        }

        const T* Mem() const {
            TIXMLASSERT( _mem );
            return _mem;
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
}



#endif







