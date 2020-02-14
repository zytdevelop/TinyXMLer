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
    //
    class TINYXML2_LIB XMLComment : public XMLNode
    {
        friend class XMLDocument;
    public:
        //访问者接口
        virtual bool Accept( XMLVisitor* visitor ) const;

        //直接返回注释内容
        virtual XMLComment* ToComment(){
            return this;
        }
        virtual XMLComment* ToComment() const{
            return this;
        }

        //克隆,建立一个副本
        virtual XMLNode* ShallowClone( XMLDocument* document ) const;

        //比较
        virtual bool ShallowEqual( const XMLNode* compare ) const;



    protected:
        //构造函数
        explicit XMLComment( XMLDocument* doc ) : XMLNode(doc){}
        //析构函数
        virtual ~XMLComment(){}

        //深度解析
        char* ParseDeep( char* p, StrPari* parentEndTag, int* curLineNumPtr );

        //
    private:
        //构造函数和复制构造函数
        XMLComment( const XMLComment& );
        XMLComment& operator=(const XMLComment& );

    };

    class TINYXML2_LIB XMLText : public XMLNode
    {
        friend class XMLDocument;
    public:
        //code
        //访问者接口
        virtual bool Accept( XMLVisitor* visitor ) const;

        //直接返回文本内容
        virtual XMLText* ToText(){
            return this;
        }
        virtual const XMLText* ToText() const{
            return this;
        }

        //类型声明
        void SetCData( bool isCData ){
            _isCData = isCData;
        }
        //如果是CData文本元素,则返回true
        bool CData() const {
            return _isCData;
        }

        //克隆
        virtual XMLNode* ShallowClone( XMLDocument* document ) const;

        //
    protected:
        //code
        //
        //构造和析构
        explicit XMLText( XMLDocument* doc ) : XMLNode( doc ), _isCData( false ) {}
        virtual ~XMLText(){}

        //深度解析
        char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

    private:
        //code
        bool _isCData;
        XMLText( const XMLText& );
        XMLText& operator=( const XMLText& );
    };

    //XML文档对象模型(DOM)中除了XMLAtrributes之外的每个对象的基类
    //数据结构:链表、二叉树
    //C++特性:面向对象,内存处理
    class TINYXML2_LIB XMLNode{
        //包含友元类(可以访问私有成员和保护成员)
        friend class XMLDocument;
        friend class XMLElement;
    public:
        //code

        //获取文档
        const XMLDocument* GetDocument() const{
            //返回文档节点
            TIXMLASSERT( _document );
            return _document;
        }

        XMLDocument* GetDocument(){
            TIXMLASSERT( _document );
            return _document;
        }

        //获取元素
        virtual XMLElement* ToElement(){
            return 0;
        }

        virtual const XMLElement* ToElement() const{
            return 0;
        }

        //获取文本
        virtual XMLText* ToText(){
            return 0;
        }
        virtual const XMLText* ToText() const{
            return 0;
        }


        //获取注释
        virtual XMLComment* ToComment(){
            return 0;
        }

        virtual const XMLComment* ToComment() const{
            return 0;
        }

        //获取文档
        virtual XMLDocument* ToDocument() {
            return0;
        }

        virtual XMLDocument* ToDocument() const{
            return 0;
        }

        //获取声明
        virtual XMLDeclaration* ToDeclaration(){
            return 0;
        }

        virtual XMLDeclaration* ToDeclaration() const{
            return 0;
        }

        //获取未知对象
        virtual XMLUnknown* ToUnknown(){
            return 0;
        }

        virtual const XMLUnknown* ToUnknow() const{
            return 0;
        }

        //检查数据
        const char* Value() const;

        //设置节点值
        void SetValue( const char* val, bool staticMem=false );

        //获取父节点
        const XMLNode* Parent() const{
            return _parent;
        }

        XMLNode* Parent(){
            return _parent;
        }

        //空子节点
        bool NoChildren() const{
            return !_firstChild;
        }

        const XMLNode* FirstChild() const{
            return _firstChild;
        }

        XMLNode* FirstChlid(){
            return _firstChild;
        }

        //获取最后一个子节点,如果不存在,则返回null
        const XMLNode* LastChild() const{
            return _lastChild;
        }

        XMLNode* LastChild(){
            return _lastChild;
        }


        //获取此节点的上一个(左)兄弟节点
        const XMLNode* PreviousSibling() const{
            return _prev;
        }

        XMLNode* PreviousSibling(){
            return _prev;
        }

        const XMLElement* PreviousSibling(const char* name = 0 ) const;

        XMLElement* PreviousSibling(const char* name = 0 ) {
            return const_cast<XMLElement*>(const_cast<const XMLElement*>(this)->PreviousSiblingElement( name ) );
        }

        //获取子元素
        const XMLElement* FirstChildElement( const char* name = 0 ) const;

        XMLElement* FirstChildElement( const char* name = 0 ){
            //强制转换
            return const_cast<XMLElment*>(const_cast<const XMLNode*>(this)->FirstChildElement( name ) );
        }

        //获取具有指定名称的最后一个子元素或可选的最后一个子元素
        const XMLElement* LastChildElement( const char* name = 0 ) const;

        XMLElement* LastChildElement( const char* name = 0){
            //强制转换
            return const_cast<XMLElement>(const_cast<const XMLNode*>(this)->LastChildElement( name ) );
        }

        //获取此节点的下一个(右)兄弟元素,并提供可选的名称

        const XMLNode* NextSibling() const{
            return _next;
        }

        XMLNode* NextSibling(){
            return _next;
        }

        const XMLElement* NextSiblingElement( const char* name = 0 ) const;

        XMLElement* NextSiblingElemnt( const char* name = 0 ){
            return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->NextSiblingElement( name ) );
        }

        //添加(右)子节点
        XMLNode* InsertEndChild( XMLNode* addThis );

        //添加(左)子节点
        XMLNode* InsertFirstChild( XMLNode* addThis );

        //添加指定子节点
        XMLNode* InsertAfterChild( XMLNode* afterThis, XMLNode* addThis );

        //删除子节点
        void DeleteChild( XMLNode* node);

        //删除节点的所有子节点
        void DeleteChildren();

        //克隆函数,纯虚函数,用于继承,克隆当前节点
        virtual XMLNode* ShallowClone( XMLNode* document ) const = 0;

        //深度克隆,克隆当前节点所有子节点
        XMLNode* DeepClone( XMLDocument* target ) const;

        //节点比较
        virtual bool ShallowEqual( const XMLNode* compare ) const = 0;

        //访问接口
        virtual bool Accept( XMLVisitor* visitor ) const = 0;

        //用户数据
        void SetUserData(void* userData){ _userData = userData; }

        //将用户数据集添加到XMLNode 中
        void* GetUserData() const { return _userData; }







    protected:
        //code
        XMLDocument* _document;
        XMLNode* _parent;
        mutable StrPair _value;    //被mutable修饰的变量,将永远处于可变状态,包括const修饰下
        //节点
        XMLNode* _firstChild;
        XMLNode* _lastChild;
        XMLNode* _prev;
        XMLNode* _next;
        void* _userData;

        //构造和析构函数
        explicit XMLNode( XMLDocument* );
        virtual ~XMLNode();



        //深度解析
        virtual char* ParseDeep( char* p, StrPari* parentEndTag, int* curLineNumPtr);






    private:
        //code
        MemPool* _memPool;    //内存分配

        //断开孩子节点的连接
        void Unlink( XMLNode* child);

        //删除节点
        static void DeleteNode( XMLNode* node );

        //插入节点前的准备
        //首先检查节点是否有链接,如果有则断开链接;
        //如果没有则检查是否有值,然后跟踪内存
        void InsertChildPreamble( XMLNode* insertThis ) const;

        //判断是否有该名称的元素,如果存在则返回元素.
        const XMLElement* ToElementWithName( const char* name ) const;

        //拷贝构造和重载函数
        XMLNode( const XMLNode& );    //无需实现
        XMLNode& operator=( const XMLNode& );    //无需实现

    };

    //实用工具类包含编码相关功能,字符解析,空格判断等功能
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

       //判断名称
       inline static bool IsNameChar( unsigned char ch ){
           return IsNameStartChar( ch )
               || isdigit( ch )    //检查是否为十进制数
               || ch == '.'
               || ch == '-';
       }

       //字符串比较
       inline static bool StringEuql( const char* p, const char* q, in nChar=INT_MAX ){
           if( p == q ){
               return true;
           }
           TIXMLASSERT( p );
           TIXMLASSERT( q );
           TIXMLASSERT( nChar >= 0 );
           return strncmp( p, q, nChar ) == 0;
       }

       //验证编码方式
       static const char* ReadBOM( const char* p, bool* hasBOM );
       //UCS转UTF-8
       //将UCS4编码转换成UTF8编码，首先根据UCS4 编码范围确定对应的UTF8编码字节数
       static void ConvertUTF32ToUTF8( unsigned long input, char* output, int* length);

       //获取字符特征
       //合法字符：
       // #x9（水平制表符）、#xA（回车符）、#xD（换行符）、
       // #x20-#xD7FF、#xE000-#xFFFD、#x10000 - #x10FFFF，
       // 即任何Unicode字符
       // p是字符串的起始位置，value用于存储 UTF-8 格式
       static const char* GetCharacterRef( const char* p, char* value, int* length );
       static void ToStr( int v, char* buffer, int bufferSize );
       static void ToStr( unsigned v, char* buffer, int bufferSize );
       static void ToStr( bool v, char* buffer, int bufferSize );
       static void ToStr( float v, char* buffer, int bufferSize );
       static void ToStr( double v, char* buffer, int bufferSize );
       static void ToStr( int64_t v, char* buffer, int bufferSize );

       //将字符串转换位基本类型
       //主要使用sscanf函数,将value读入str中
       static bool ToInt( const char* str, int* value );
       static bool ToUnsigned( const char* str, unsigned* value );
       static bool ToBool( const char* str, bool* value );
       static bool ToFloat( const char* str, float* value );
       static bool ToDouble( const char* str, double* value );
       static bool ToInt64( const char* str, int64_t* value );

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







