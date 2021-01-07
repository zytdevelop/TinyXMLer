#ifndef TINYXML2_INCLUDED
#define TINYXML2_INCLUDED
#include <cctype>			//字符处理库
#include <climits>			//类型支持库
#include <cstdio>			//标准输入输出流库
#include <cstdlib>			//杂项工具支持库
#include <cstring>			//字符串标准库
#include <stdint.h>			//位宽整形库

//允许动态库导出，此方法对其他模块可见
#define TINYXML2_LIB 		__attribute__((visibility("default")))	

//诊断宏，判断是否为零值
#define TIXMLASSERT( x )	{}	

//限制元素深度，避免堆栈溢出
static const int TINYXML2_MAX_ELEMENT_DEPTH = 100;

namespace tinyxml2{
	//以下是文档解析需要实现的类,需要提前声明
	class XMLDocument;
    class XMLElement;
    class XMLAttribute;
    class XMLComment;
    class XMLText;
    class XMLDeclaration;
    class XMLUnknown;
    class XMLPrinter;
    
    //警告，需匹配相应的名称
    enum XMLError {
        XML_SUCCESS = 0,
        XML_NO_ATTRIBUTE,
        XML_WRONG_ATTRIBUTE_TYPE,
        XML_ERROR_FILE_NOT_FOUND,
        XML_ERROR_FILE_COULD_NOT_BE_OPENED,
        XML_ERROR_FILE_READ_ERROR,
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
    class StrPair{
	public:
		//code
		enum {
			NEEDS_ENTITY_PROCESSING			= 0x01,		//实体处理
			NEEDS_NEWLINE_NORMALIZATION		= 0x02,		//正常处理
			NEEDS_WHITESPACE_COLLAPSING     = 0x04,		//空白处理
			//实体化处理文本元素，0x03
			TEXT_ELEMENT		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,		
			//正常处理文本元素
			TEXT_ELEMENT_LEAVE_ENTITIES		= NEEDS_NEWLINE_NORMALIZATION,
			ATTRIBUTE_NAME		            = 0,		//属性名
			//实体化处理属性值，0x03
			ATTRIBUTE_VALUE		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
			//正常处理属性值
			ATTRIBUTE_VALUE_LEAVE_ENTITIES  = NEEDS_NEWLINE_NORMALIZATION,
			//正常处理注释
			COMMENT							= NEEDS_NEWLINE_NORMALIZATION
		};
        StrPair() : _flags( 0 ), _start( 0 ), _end( 0 ) {}
        ~StrPair(){
            Reset();
        }
        void Set( char* start, char* end, int flags ) {
            TIXMLASSERT( start );
            TIXMLASSERT( end );
            Reset();
            _start  = start;
            _end    = end;
            _flags  = flags | NEEDS_FLUSH;
        }
        void Reset();
        void SetInternedStr( const char* str ) {
             Reset();
             _start = const_cast<char*>(str);
         }

         void SetStr( const char* str, int flags=0 );

        const char* GetStr();
		
        bool Empty() const {
            return _start == _end;
        }

        char* ParseText( char* in, const char* endTag, int strFlags, int* curLineNumPtr );

        char* ParseName( char* in );

        void TransferTo( StrPair* other );

	private:
		//code
		int     _flags;
		char*   _start;
		char*   _end;
		enum {
    		NEEDS_FLUSH = 0x100,
    		NEEDS_DELETE = 0x200
		};
        StrPair( const StrPair& other );            // 不需要实现
        void operator=( const StrPair& other );     // 不需要实现，使用TransferTo()替代
        void CollapseWhitespace();
	};

    template <class T, int INITIAL_SIZE>
    class DynArray{
    public:
        //code
        DynArray():_mem(_pool),
        _allocated( INITIAL_SIZE ),
        _size( 0 )
        {
        }

        ~DynArray() {
            if ( _mem != _pool ) {
                delete [] _mem;
            }
        }

        void Clear() {
            _size = 0;
        }

        void Push( T t ) {
            TIXMLASSERT( _size < INT_MAX );
            //检查空间容量
            EnsureCapacity( _size+1 );
            _mem[_size] = t;
            ++_size;
        }

        T* PushArr( int count ) {
            TIXMLASSERT( count >= 0 );
            TIXMLASSERT( _size <= INT_MAX - count );
            EnsureCapacity( _size+count );
            T* ret = &_mem[_size];
            _size += count;
            return ret;
        }

        T Pop() {
            TIXMLASSERT( _size > 0 );
            --_size;
            return _mem[_size];
        }

        void PopArr( int count ) {
            TIXMLASSERT( _size >= count );
            _size -= count;
        }

        bool Empty() const  {
            return _size == 0;
        }

        T& operator[](int i)    {
            TIXMLASSERT( i>= 0 && i < _size );
            return _mem[i];
        }

        //只读
        const T& operator[](int i) const    {
            TIXMLASSERT( i>= 0 && i < _size );
            return _mem[i];
        }

        const T& PeekTop() const    {
            TIXMLASSERT( _size > 0 );
            return _mem[ _size - 1];
        }

        int Size() const    {
            TIXMLASSERT( _size >= 0 );
            return _size;
        }

        int Capacity() const    {
            TIXMLASSERT( _allocated >= INITIAL_SIZE );
            return _allocated;
        }

        void SwapRemove(int i) {
            TIXMLASSERT(i >= 0 && i < _size);
            TIXMLASSERT(_size > 0);
            _mem[i] = _mem[_size - 1];
            --_size;
        }

        T* Mem()    {
            TIXMLASSERT( _mem );
            return _mem;
        }

        //只读
        const T* Mem() const    {
            TIXMLASSERT( _mem );
            return _mem;
        }

        
    private:
        //code
        DynArray( const DynArray& );            //不需要实现
        void operator=( const DynArray& );      //不需要实现

        void EnsureCapacity( int cap ) {
            TIXMLASSERT( cap > 0 );
            //如果内存池已满，则申请空间
            if ( cap > _allocated ) {
                TIXMLASSERT( cap <= INT_MAX / 2 );
                //申请新的内存池类来接收，每次申请两倍空间
                int newAllocated = cap * 2;
                T* newMem = new T[newAllocated];
                TIXMLASSERT( newAllocated >= _size );
                //替换数据
                memcpy( newMem, _mem, sizeof(T)*_size );
                //释放_mem
                if ( _mem != _pool ) {
                    delete [] _mem;
                }
                _mem = newMem;
                _allocated = newAllocated;
            }
        }

        T*  _mem;
        T   _pool[INITIAL_SIZE];    //内存池
        int _allocated;             //分配器
        int _size;                  //元素数量
    };

    class MemPool
    {
    public:
        MemPool() {}
        virtual ~MemPool() {}
        
        virtual int ItemSize() const = 0;
        virtual void* Alloc() = 0;
        virtual void Free( void* ) = 0;
        virtual void SetTracked() = 0;
    };

    template< int ITEM_SIZE >
    class MemPoolT : public MemPool{
    public:
        //code
        enum { ITEMS_PER_BLOCK = (4 * 1024) / ITEM_SIZE };
        MemPoolT() : _blockPtrs(), _root(0), _currentAllocs(0), _nAllocs(0), _maxAllocs(0), _nUntracked(0)  {}
        ~MemPoolT() {
            MemPoolT< ITEM_SIZE >::Clear();
        }

        void Clear() {
            // 删除块
            while( !_blockPtrs.Empty()) {
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

        virtual int ItemSize() const    {
            return ITEM_SIZE;
        }

        int CurrentAllocs() const       {
            return _currentAllocs;
        }

        virtual void* Alloc() {
            if ( !_root ) {
                // 需要一个新的块
                Block* block = new Block();
                _blockPtrs.Push( block );
                //把块的内容复制到节点
                Item* blockItems = block->items;
                for( int i = 0; i < ITEMS_PER_BLOCK - 1; ++i ) {
                    blockItems[i].next = &(blockItems[i + 1]);
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
            if ( _currentAllocs > _maxAllocs ) {
                _maxAllocs = _currentAllocs;
            }
            ++_nAllocs;
            ++_nUntracked;
            return result;
        }

        virtual void Free( void* mem ) {
            //为空返回
            if ( !mem ) {
                return;
            }
            //当前分配数--
            --_currentAllocs;
            //强制转换为空间节点类型
            Item* item = static_cast<Item*>( mem );
        //如果采用DEBUG调试，item全置为0xfe
        #ifdef TINYXML2_DEBUG
            memset( item, 0xfe, sizeof( *item ) );
        #endif
            //清空item
            item->next = _root;
            _root = item;
        }

        void Trace( const char* name ) {
            printf( "Mempool %s watermark=%d [%dk] current=%d size=%d nAlloc=%d blocks=%d\n", name, _maxAllocs, _maxAllocs * ITEM_SIZE / 1024, _currentAllocs,ITEM_SIZE, _nAllocs, _blockPtrs.Size() );
        }

        //调用过跟踪信息后，未跟踪数-1
        void SetTracked() {
            --_nUntracked;
        }

        int Untracked() const {
            return _nUntracked;
        }

    private:
        //code
        MemPoolT( const MemPoolT& );        //不实现
        void operator=( const MemPoolT& );  //不实现

        union Item {
            Item*   next;
            char    itemData[ITEM_SIZE];
        };

        struct Block {
            Item items[ITEMS_PER_BLOCK];
        };
        //定义一个动态数组
        DynArray< Block*, 10 > _blockPtrs;
        //定义根节点
        Item* _root;

        int _currentAllocs;         //当前分配数
        int _nAllocs;               //分配总次数
        int _maxAllocs;             //最大分配数
        int _nUntracked;            //未跟踪次数
    };

    class TINYXML2_LIB XMLVisitor{
    public:
        //code
         virtual ~XMLVisitor() {}
         virtual bool VisitEnter( const XMLDocument&){
            return true;
        }

        virtual bool VisitExit( const XMLDocument&){
            return true;
        }

        virtual bool VisitEnter( const XMLElement&, const XMLAttribute*){
            return true;
        }

        virtual bool VisitExit( const XMLElement&){
            return true;
        }

        virtual bool Visit( const XMLDeclaration& ){
            return true;
        }

        virtual bool Visit( const XMLText& ){
            return true;
        }

        virtual bool Visit( const XMLComment& ){
            return true;
        }

        virtual bool Visit( const XMLUnknown& ){
            return true;
        }
    };

    class TINYXML2_LIB XMLUtil{
    public:
        //code
        static void SetBoolSerialization(const char* writeTrue, const char* writeFalse);
        inline static bool IsUTF8Continuation( char p ) {
            return ( p & 0x80 ) != 0;
        }

        static bool IsWhiteSpace( char p ){
            return !IsUTF8Continuation(p) && isspace( static_cast<unsigned char>(p) );
        }
        
        static const char* SkipWhiteSpace( const char* p, int* curLineNumPtr )  {
            TIXMLASSERT( p );
            
            //如果是空白，则需考虑换行
            while( IsWhiteSpace(*p) ) {
                if (curLineNumPtr && *p == '\n') {
                    ++(*curLineNumPtr);
                }
                ++p;
            }
            TIXMLASSERT( p );
            return p;
        }

        static char* SkipWhiteSpace( char* p, int* curLineNumPtr )              {
            //结果类型和参数强制转换为常量
            return const_cast<char*>( SkipWhiteSpace( const_cast<const char*>(p), curLineNumPtr ) );
        }

        inline static bool IsNameStartChar( unsigned char ch ) {
            if ( ch >= 128 ) {
                return true;
            }
            if ( isalpha( ch ) ) {
                return true;
            }
            return ch == ':' || ch == '_';
        }

        inline static bool IsNameChar( unsigned char ch ) {
            return IsNameStartChar( ch )
            || isdigit( ch )        //检查是否为十进制数字
            || ch == '.'
            || ch == '-';
        }

        inline static bool StringEqual( const char* p, const char* q, int nChar=INT_MAX )  {
            if ( p == q ) {
                return true;
            }
            TIXMLASSERT( p );
            TIXMLASSERT( q );
            TIXMLASSERT( nChar >= 0 );
            return strncmp( p, q, nChar ) == 0;
        }

        static const char* ReadBOM( const char* p, bool* hasBOM );

        static void ConvertUTF32ToUTF8( unsigned long input, char* output, int* length );

        //p是字符串起始位置，value用于存储 UTF-8 格式
        static const char* GetCharacterRef( const char* p, char* value, int* length );

        static void ToStr( int v, char* buffer, int bufferSize );
        static void ToStr( unsigned v, char* buffer, int bufferSize );
        static void ToStr( bool v, char* buffer, int bufferSize );
        static void ToStr( float v, char* buffer, int bufferSize );
        static void ToStr( double v, char* buffer, int bufferSize );
        static void ToStr(int64_t v, char* buffer, int bufferSize);

        static bool ToInt( const char* str, int* value );
        static bool ToUnsigned( const char* str, unsigned* value );
        static bool ToBool( const char* str, bool* value );
        static bool ToFloat( const char* str, float* value );
        static bool ToDouble( const char* str, double* value );
        static bool ToInt64(const char* str, int64_t* value);

    private:
        //code
        static const char* writeBoolTrue;
        static const char* writeBoolFalse;
    
    };

    class TINYXML2_LIB XMLNode{
        
        friend class XMLDocument;
        friend class XMLElement;
    public:
        //code
        const XMLDocument* GetDocument() const{
        TIXMLASSERT( _document );
            return _document;
        }

        XMLDocument* GetDocument(){
            TIXMLASSERT( _document );
            return _document;
        }

        virtual XMLElement*     ToElement()     {
            return 0;
        }

        virtual const XMLElement*       ToElement() const       {
            return 0;
        }

        virtual XMLText*        ToText()        {
            return 0;
        }

        virtual const XMLText*          ToText() const          {
            return 0;
        }

        virtual XMLComment*     ToComment()     {
            return 0;
        }

        virtual const XMLComment*       ToComment() const       {
            return 0;
        }

        virtual XMLDocument*    ToDocument()    {
            return 0;
        }

        virtual const XMLDocument*      ToDocument() const      {
            return 0;
        }

        virtual XMLDeclaration* ToDeclaration() {
            return 0;
        }

        virtual const XMLDeclaration*   ToDeclaration() const   {
            return 0;
        }

        virtual XMLUnknown*     ToUnknown()     {
            return 0;
        }

        virtual const XMLUnknown*       ToUnknown() const       {
            return 0;
        }

        const char* Value() const;
        
        void SetValue( const char* val, bool staticMem=false );

        int GetLineNum() const { return _parseLineNum; }

        const XMLNode*  Parent() const          {
            return _parent;
        }

        XMLNode* Parent()                       {
            return _parent;
        }

        bool NoChildren() const                 {
            return !_firstChild;
        }

        const XMLNode*  FirstChild() const      {
            return _firstChild;
        }

        XMLNode*        FirstChild()            {
            return _firstChild;
        }

        const XMLNode* LastChild() const                        {
            return _lastChild;
         }

        XMLNode*        LastChild()                             {
            return _lastChild;
        }

        const XMLNode*  PreviousSibling() const                 {
            return _prev;
        }

        XMLNode*    PreviousSibling()                           {
            return _prev;
        }

        const XMLElement*   PreviousSiblingElement( const char* name = 0 ) const ;

        XMLElement* PreviousSiblingElement( const char* name = 0 ) {
            return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->PreviousSiblingElement( name ) );
        }

        const XMLElement* FirstChildElement( const char* name = 0 ) const;

        XMLElement* FirstChildElement( const char* name = 0 )   {
                //强制转换
            return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->FirstChildElement( name ));
        }

        const XMLElement* LastChildElement( const char* name = 0 ) const;

        XMLElement* LastChildElement( const char* name = 0 )    {
            //强制转换
            return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->LastChildElement(name) );
        }

        const XMLNode*  NextSibling() const                     {
            return _next;
        }

        XMLNode*    NextSibling()                               {
            return _next;
        }

        const XMLElement*   NextSiblingElement( const char* name = 0 ) const;

        XMLElement* NextSiblingElement( const char* name = 0 )  {
            return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->NextSiblingElement( name ) );
        }


        XMLNode* InsertEndChild( XMLNode* addThis );

        XMLNode* LinkEndChild( XMLNode* addThis )   {
            return InsertEndChild( addThis );
        }

        XMLNode* InsertFirstChild( XMLNode* addThis );

        XMLNode* InsertAfterChild( XMLNode* afterThis, XMLNode* addThis );

        void DeleteChild( XMLNode* node );

        void DeleteChildren();

        virtual XMLNode* ShallowClone( XMLDocument* document ) const = 0;

        XMLNode* DeepClone( XMLDocument* target ) const;

        virtual bool ShallowEqual( const XMLNode* compare ) const = 0;

        virtual bool Accept( XMLVisitor* visitor ) const = 0;

        void SetUserData(void* userData)    { _userData = userData; }

        void* GetUserData() const           { return _userData; }

    protected:
        //code
        explicit XMLNode( XMLDocument* );
        virtual ~XMLNode();

        virtual char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr);

        XMLDocument*    _document;
        XMLNode*        _parent;
        mutable StrPair _value;             //被mutable修饰的变量，将永远处于可变的状态，包括const修饰下
        int             _parseLineNum;
        //节点
        XMLNode*        _firstChild;
        XMLNode*        _lastChild;
        XMLNode*        _prev;
        XMLNode*        _next;
        void*           _userData;

    private:
        //code
        void Unlink( XMLNode* child );

        static void DeleteNode( XMLNode* node );

        void InsertChildPreamble( XMLNode* insertThis ) const;

        const XMLElement* ToElementWithName( const char* name ) const;

        XMLNode( const XMLNode& );              //无需实现
        XMLNode& operator=( const XMLNode& );   //无需实现

        MemPool*        _memPool;           //内存分配
        
    };

    class TINYXML2_LIB XMLText: public XMLNode
    {
        friend class XMLDocument;
    public:
        //code
        virtual bool Accept( XMLVisitor* visitor ) const;

        virtual XMLText* ToText()           {
            return this;
        }

        virtual const XMLText* ToText() const   {
            return this;
        }

        void SetCData( bool isCData )           {
            _isCData = isCData;
        }

        //如果是CData文本元素，则返回true。
        bool CData() const                      {
            return _isCData;
        }

        virtual XMLNode* ShallowClone( XMLDocument* document ) const;

        virtual bool ShallowEqual( const XMLNode* compare ) const;
        
    protected:
        //code
        explicit XMLText( XMLDocument* doc )    : XMLNode( doc ), _isCData( false ) {}

        virtual ~XMLText() {}

        char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

    private:
        //code
        bool _isCData;
        XMLText( const XMLText& );
        XMLText& operator=( const XMLText& );
    };

    class TINYXML2_LIB XMLComment: public XMLNode
    {
        friend class XMLDocument;
    public:
        //code
        virtual bool Accept( XMLVisitor* visitor ) const;

        virtual XMLComment* ToComment()                 {
            return this;
        }
        virtual const XMLComment* ToComment() const     {
            return this;
        }

        virtual XMLNode* ShallowClone( XMLDocument* document ) const;

        virtual bool ShallowEqual( const XMLNode* compare ) const;
        
    protected:
        //code
        explicit XMLComment( XMLDocument* doc ): XMLNode( doc ) {}

        virtual ~XMLComment()   {}

        char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

    private:
        //code
        XMLComment( const XMLComment& );
        XMLComment& operator=( const XMLComment& ); 
        
    };

    class TINYXML2_LIB XMLDeclaration: public XMLNode
    {
        friend class XMLDocument;
    public:
        //code
        virtual bool Accept( XMLVisitor* visitor ) const;

        virtual XMLDeclaration* ToDeclaration()                 {
            return this;
        }
        virtual const XMLDeclaration* ToDeclaration() const     {
            return this;
        }

        virtual XMLNode* ShallowClone( XMLDocument* document ) const;

        virtual bool ShallowEqual( const XMLNode* compare ) const;
        
    protected:
        //code
        explicit XMLDeclaration( XMLDocument* doc ): XMLNode( doc ) {}

        virtual ~XMLDeclaration()   {}

        char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

    private:
        //code
        XMLDeclaration( const XMLDeclaration& );
        XMLDeclaration& operator=( const XMLDeclaration& );
        
    };

    class TINYXML2_LIB XMLUnknown: public XMLNode
    {
        friend class XMLDocument;
    public:
        //code
        virtual bool Accept( XMLVisitor* visitor ) const;

        virtual XMLNode* ShallowClone( XMLDocument* document ) const;

        virtual bool ShallowEqual( const XMLNode* compare ) const;

        virtual XMLUnknown* ToUnknown()                 {
            return this;
        }
        virtual const XMLUnknown* ToUnknown() const     {
            return this;
        }
        
    protected:
        //code
        explicit XMLUnknown( XMLDocument* doc ) : 
        XMLNode( doc ){}

        virtual ~XMLUnknown(){}
        char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

    private:
        //code
        XMLUnknown( const XMLUnknown& );
        XMLUnknown& operator=( const XMLUnknown& );
        
    };

    class TINYXML2_LIB XMLAttribute
    {
        friend class XMLElement;
    public:
        //code
        const char* Name() const;

        const char* Value() const;

        int GetLineNum() const { return _parseLineNum; }

        const XMLAttribute* Next() const {
            return _next;
        }

        //转换为整数
        XMLError QueryIntValue( int* value ) const;
        //转换为无符号数
        XMLError QueryUnsignedValue( unsigned int* value ) const;
        //转换为64位整数
        XMLError QueryInt64Value(int64_t* value) const;
        //转换为布尔型
        XMLError QueryBoolValue( bool* value ) const;
        //转换为双精度型
        XMLError QueryDoubleValue( double* value ) const;
        //转换为浮点型
        XMLError QueryFloatValue( float* value ) const;

        int IntValue() const                    {
          int i = 0;
          QueryIntValue(&i);
          return i;
        }

        int64_t Int64Value() const              {
              int64_t i = 0;
              QueryInt64Value(&i);
              return i;
        }

        unsigned UnsignedValue() const          {
            unsigned i=0;
            QueryUnsignedValue( &i );
            return i;
        }

        bool     BoolValue() const              {
            bool b=false;
            QueryBoolValue( &b );
            return b;
        }

        double   DoubleValue() const            {
            double d=0;
            QueryDoubleValue( &d );
            return d;
        }

        float    FloatValue() const             {
            float f=0;
            QueryFloatValue( &f );
            return f;
        }

        //字符型
        void SetAttribute( const char* value );
        //整数型
        void SetAttribute( int value );
        //无符号型
        void SetAttribute( unsigned value );
        //64位整数型
        void SetAttribute(int64_t value);
        //布尔型
        void SetAttribute( bool value );
        //双精度型
        void SetAttribute( double value );
        //浮点型
        void SetAttribute( float value );

    private:
        //code

        XMLAttribute() : _name(), _value(),_parseLineNum( 0 ), _next( 0 ), _memPool( 0 ) {}

        virtual ~XMLAttribute() {}

        XMLAttribute( const XMLAttribute& );    
        void operator=( const XMLAttribute& );

        void SetName( const char* name );

        char* ParseDeep( char* p, bool processEntities, int* curLineNumPtr );

        enum { BUF_SIZE = 200 };        //内存池大小
        mutable StrPair _name;
        mutable StrPair _value;
        int             _parseLineNum;
        XMLAttribute*   _next;
        MemPool*        _memPool;
        
    };

    class TINYXML2_LIB XMLElement: public XMLNode
    {
        friend class XMLDocument;
    public:
        //code
        enum ElementClosingType {
            OPEN,               // <foo>
            CLOSED,             // <foo/>
            CLOSING             // </foo>
        };

        const char* Name() const {
            return Value();
        }

        void SetName( const char* str, bool staticMem=false )   {
            SetValue( str, staticMem );
        }

        virtual XMLElement* ToElement()             {
            return this;
        }

        virtual const XMLElement* ToElement() const {
            return this;
        }

        virtual bool Accept( XMLVisitor* visitor ) const;

        const char* Attribute( const char* name, const char* value=0 ) const;

        XMLError QueryIntAttribute( const char* name, int* value ) const
        {
            const XMLAttribute* a = FindAttribute( name );
            if ( !a ) {
                return XML_NO_ATTRIBUTE;
            }
            return a->QueryIntValue( value );
        }


        XMLError QueryUnsignedAttribute( const char* name, unsigned int* value ) const  {
            const XMLAttribute* a = FindAttribute( name );
            if ( !a ) {
                return XML_NO_ATTRIBUTE;
            }
            return a->QueryUnsignedValue( value );
        }


        XMLError QueryInt64Attribute(const char* name, int64_t* value) const {
            const XMLAttribute* a = FindAttribute(name);
            if (!a) {
                return XML_NO_ATTRIBUTE;
            }
            return a->QueryInt64Value(value);
        }


        XMLError QueryBoolAttribute( const char* name, bool* value ) const      {
            const XMLAttribute* a = FindAttribute( name );
            if ( !a ) {
                return XML_NO_ATTRIBUTE;
            }
            return a->QueryBoolValue( value );
        }

        XMLError QueryDoubleAttribute( const char* name, double* value ) const
        {
            const XMLAttribute* a = FindAttribute( name );
            if ( !a ) {
                return XML_NO_ATTRIBUTE;
            }
            return a->QueryDoubleValue( value );
        }

        XMLError QueryFloatAttribute( const char* name, float* value ) const
        {
            const XMLAttribute* a = FindAttribute( name );
            if ( !a ) {
                return XML_NO_ATTRIBUTE;
            }
            return a->QueryFloatValue( value );
        }

        XMLError QueryStringAttribute(const char* name, const char** value) const {
            const XMLAttribute* a = FindAttribute(name);
            if (!a) {
                return XML_NO_ATTRIBUTE;
            }
            *value = a->Value();
            return XML_SUCCESS;
        }

        int IntAttribute(const char* name, int defaultValue = 0) const;

        unsigned UnsignedAttribute(const char* name, unsigned defaultValue = 0) const;

        int64_t Int64Attribute(const char* name, int64_t defaultValue = 0) const;

        bool BoolAttribute(const char* name, bool defaultValue = false) const;

        double DoubleAttribute(const char* name, double defaultValue = 0) const;

        float FloatAttribute(const char* name, float defaultValue = 0) const;

        XMLError QueryAttribute( const char* name, int* value ) const {
          return QueryIntAttribute( name, value );
        }

        XMLError QueryAttribute( const char* name, unsigned int* value ) const {
          return QueryUnsignedAttribute( name, value );
        }

        XMLError QueryAttribute(const char* name, int64_t* value) const {
          return QueryInt64Attribute(name, value);
        }

        XMLError QueryAttribute( const char* name, bool* value ) const {
          return QueryBoolAttribute( name, value );
        }

        XMLError QueryAttribute( const char* name, double* value ) const {
          return QueryDoubleAttribute( name, value );
        }

        XMLError QueryAttribute( const char* name, float* value ) const {
          return QueryFloatAttribute( name, value );
        }

        void SetAttribute( const char* name, const char* value )    {
            XMLAttribute* a = FindOrCreateAttribute( name );
            a->SetAttribute( value );
        }

        void SetAttribute( const char* name, int value )            {
            XMLAttribute* a = FindOrCreateAttribute( name );
            a->SetAttribute( value );
        }

        void SetAttribute( const char* name, unsigned value )       {
            XMLAttribute* a = FindOrCreateAttribute( name );
            a->SetAttribute( value );
        }

        void SetAttribute(const char* name, int64_t value) {
          XMLAttribute* a = FindOrCreateAttribute(name);
          a->SetAttribute(value);
        }

        void SetAttribute( const char* name, bool value )           {
            XMLAttribute* a = FindOrCreateAttribute( name );
            a->SetAttribute( value );
        }

        void SetAttribute( const char* name, double value )     {
            XMLAttribute* a = FindOrCreateAttribute( name );
            a->SetAttribute( value );
        }

        void SetAttribute( const char* name, float value )      {
            XMLAttribute* a = FindOrCreateAttribute( name );
            a->SetAttribute( value );
        }

        void DeleteAttribute( const char* name );

        const XMLAttribute* FirstAttribute() const {
            return _rootAttribute;
        }

        const XMLAttribute* FindAttribute( const char* name ) const;

        const char* GetText() const;

        void SetText( const char* inText );

        void SetText( int value );

        void SetText( unsigned value );

        void SetText(int64_t value);

        void SetText( bool value );

        void SetText( double value );

        void SetText( float value );

        XMLError QueryIntText( int* ival ) const;

        XMLError QueryUnsignedText( unsigned* uval ) const;

        XMLError QueryInt64Text(int64_t* uval) const;

        XMLError QueryBoolText( bool* bval ) const;

        XMLError QueryDoubleText( double* dval ) const;

        XMLError QueryFloatText( float* fval ) const;

        int IntText(int defaultValue = 0) const;

        unsigned UnsignedText(unsigned defaultValue = 0) const;

        int64_t Int64Text(int64_t defaultValue = 0) const;

        bool BoolText(bool defaultValue = false) const;

        double DoubleText(double defaultValue = 0) const;

        float FloatText(float defaultValue = 0) const;

        ElementClosingType ClosingType() const {
            return _closingType;
        }

        virtual XMLNode* ShallowClone( XMLDocument* document ) const;

        virtual bool ShallowEqual( const XMLNode* compare ) const;
        
    protected:
        //code
        char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

    private:
        //code
        XMLElement( XMLDocument* doc );
        virtual ~XMLElement();

        XMLElement( const XMLElement& );
        void operator=( const XMLElement& );

        XMLAttribute* CreateAttribute();

        static void DeleteAttribute( XMLAttribute* attribute );

        XMLAttribute* FindOrCreateAttribute( const char* name );

        char* ParseAttributes( char* p, int* curLineNumPtr );

        enum { BUF_SIZE = 200 };
        ElementClosingType _closingType;        //元素展开状态
        XMLAttribute* _rootAttribute;           //属性列表
        
    };

    //处理空白方式
    enum Whitespace {
        PRESERVE_WHITESPACE,
        COLLAPSE_WHITESPACE
    };

    class TINYXML2_LIB XMLDocument : public XMLNode{
        friend class XMLElement;
        friend class XMLNode;
        friend class XMLText;
        friend class XMLComment;
        friend class XMLDeclaration;
        friend class XMLUnknown;
    public:
        //code

        XMLDocument( bool processEntities = true, Whitespace whitespaceMode = PRESERVE_WHITESPACE );
        ~XMLDocument();

        virtual XMLDocument* ToDocument()               {
            TIXMLASSERT( this == _document );
            return this;
        }

        virtual const XMLDocument* ToDocument() const   {
            TIXMLASSERT( this == _document );
            return this;
        }

        XMLError Parse( const char* xml, size_t nBytes=(size_t)(-1) );

        XMLError LoadFile( const char* filename );

        XMLError LoadFile( FILE* );

        XMLError SaveFile( const char* filename, bool compact = false );

        XMLError SaveFile( FILE* fp, bool compact = false );

        bool ProcessEntities() const        {
            return _processEntities;
        }

         Whitespace WhitespaceMode() const  {
            return _whitespaceMode;
         }

         //如果此文档具有UTF8标记，则返回true
        bool HasBOM() const {
            return _writeBOM;
        }

        //设置在写入文件时是否写入BOM。
        void SetBOM( bool useBOM ) {
            _writeBOM = useBOM;
        }

        XMLElement* RootElement()               {
            return FirstChildElement();
        }

        const XMLElement* RootElement() const   {
            return FirstChildElement();
        }

        void Print( XMLPrinter* streamer=0 ) const;

        virtual bool Accept( XMLVisitor* visitor ) const;

        XMLElement* NewElement( const char* name );

        XMLComment* NewComment( const char* comment );

        XMLText* NewText( const char* text );

        XMLDeclaration* NewDeclaration( const char* text=0 );

        XMLUnknown* NewUnknown( const char* text );

        void DeleteNode( XMLNode* node );

        void ClearError() {
            SetError(XML_SUCCESS, 0, 0);
        }

        bool Error() const {
            return _errorID != XML_SUCCESS;
        }

        XMLError  ErrorID() const {
            return _errorID;
        }

        static const char* ErrorIDToName(XMLError errorID);
        const char* ErrorName() const;

        const char* ErrorStr() const;

        void PrintError() const;

        int ErrorLineNum() const
        {
            return _errorLineNum;
        }

        void Clear();

        void DeepCopy(XMLDocument* target) const;

        char* Identify( char* p, XMLNode** node );

        void MarkInUse(XMLNode*);

        virtual XMLNode* ShallowClone( XMLDocument*) const  {
            //不支持，默认返回0
            return 0;
        }

        virtual bool ShallowEqual( const XMLNode*) const    {
            //不支持，默认返回false
            return false;
        }


    private:
        //code
        XMLDocument( const XMLDocument& );
        void operator=( const XMLDocument& );
        void Parse();
        void SetError( XMLError error, int lineNum, const char* format, ... );

        class DepthTracker {
        public:
            //构造函数
            explicit DepthTracker(XMLDocument * document) {
                this->_document = document;
                document->PushDepth();
            }
            ~DepthTracker() {
                //降低深度
                _document->PopDepth();
            }
        private:
            XMLDocument * _document;
        };

        void PushDepth();
        void PopDepth();

        template<class NodeType, int PoolElementSize>
        NodeType* CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool );

        bool                                _writeBOM;          //是否写入
        bool                                _processEntities;   //实体
        XMLError                            _errorID;           //错误ID
        Whitespace                          _whitespaceMode;    //空白
        mutable StrPair                     _errorStr;          //错误字符
        int                                 _errorLineNum;      //错误行号
        char*                               _charBuffer;        //字符缓存区
        int                                 _parseCurLineNum;   //当前解析行
        int                                 _parsingDepth;      //解析深度  
        DynArray<XMLNode*, 10>              _unlinked;          //未链接节点
        MemPoolT< sizeof(XMLElement) >      _elementPool;       //元素内存池
        MemPoolT< sizeof(XMLAttribute) >    _attributePool;     //属性内存池
        MemPoolT< sizeof(XMLText) >         _textPool;          //文本内存池
        MemPoolT< sizeof(XMLComment) >      _commentPool;       //注释内存池
        //错误代码，在源代码中赋值
        static const char* _errorNames[XML_ERROR_COUNT];        
    };

    template<class NodeType, int PoolElementSize>
    inline NodeType* XMLDocument::CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool )
    {
        TIXMLASSERT( sizeof( NodeType ) == PoolElementSize );
        TIXMLASSERT( sizeof( NodeType ) == pool.ItemSize() );
        //创建节点并分配空间
        NodeType* returnNode = new (pool.Alloc()) NodeType( this );
        TIXMLASSERT( returnNode );
        returnNode->_memPool = &pool;
        _unlinked.Push(returnNode);
        return returnNode;
    }

    class TINYXML2_LIB XMLHandle
    {
    public:
        //code
        //从任何节点（在树的任何深度处）创建，可以是空指针
        explicit XMLHandle( XMLNode* node ) : _node( node ) {
        }

        //从节点创建
        explicit XMLHandle( XMLNode& node ) : _node( &node ) {
        }

        //拷贝构造
        XMLHandle( const XMLHandle& ref ) : _node( ref._node ) {
        }

        XMLHandle& operator=( const XMLHandle& ref )                            {
            _node = ref._node;
            return *this;
        }

        XMLHandle FirstChild()                                                  {
            //节点存在则返回，不存在返回0
            return XMLHandle( _node ? _node->FirstChild() : 0 );
        }

        XMLHandle FirstChildElement( const char* name = 0 )                     {
            return XMLHandle( _node ? _node->FirstChildElement( name ) : 0 );
        }

        XMLHandle LastChild()                                                   {
            return XMLHandle( _node ? _node->LastChild() : 0 );
        }

        XMLHandle LastChildElement( const char* name = 0 )                      {
            return XMLHandle( _node ? _node->LastChildElement( name ) : 0 );
        }

        XMLHandle PreviousSibling()                                             {
            return XMLHandle( _node ? _node->PreviousSibling() : 0 );
        }

        XMLHandle PreviousSiblingElement( const char* name = 0 )                {
            return XMLHandle( _node ? _node->PreviousSiblingElement( name ) : 0 );
        }

        XMLHandle NextSibling()                                                 {
            return XMLHandle( _node ? _node->NextSibling() : 0 );
        }

        XMLHandle NextSiblingElement( const char* name = 0 )                    {
            return XMLHandle( _node ? _node->NextSiblingElement( name ) : 0 );
        }

        //安全的转换为节点，可以为空。
        XMLNode* ToNode()                           {
            return _node;
        }

        //安全的转换为元素，可以为空。
        XMLElement* ToElement()                     {
            return ( _node ? _node->ToElement() : 0 );
        }

        //安全的转换为文本，可以为空。
        XMLText* ToText()                           {
        return ( _node ? _node->ToText() : 0 );
        }

        //安全的转换为未知内容，可以为空。
        XMLUnknown* ToUnknown()                     {
        return ( _node ? _node->ToUnknown() : 0 );
        }

        //安全的转换为声明，可以为空。
        XMLDeclaration* ToDeclaration()             {
            return ( _node ? _node->ToDeclaration() : 0 );
        }

    private:
        //code
        XMLNode* _node;
    };

    class TINYXML2_LIB XMLConstHandle
    {
    public:
        //code

        //从任何节点（在树的任何深度处）创建，可以是空指针
        explicit XMLConstHandle(const XMLNode* node ) : _node( node ) {
        }

        //从节点创建
        explicit XMLConstHandle(const XMLNode& node ) : _node( &node ) {
        }

        //拷贝构造
        XMLConstHandle( const XMLConstHandle& ref ) : _node( ref._node ) {
        }

        XMLConstHandle& operator=( const XMLConstHandle& ref )
        {
            _node = ref._node;
            return *this;
        }

        const XMLConstHandle FirstChildElement( const char* name = 0 ) const
        {
            return XMLConstHandle( _node ? _node->FirstChildElement( name ) : 0 );
        }

        const XMLConstHandle LastChild()    const
        {
            return XMLConstHandle( _node ? _node->LastChild() : 0 );
        }

        const XMLConstHandle LastChildElement( const char* name = 0 ) const
        {
            return XMLConstHandle( _node ? _node->LastChildElement( name ) : 0 );
        }

        const XMLConstHandle PreviousSibling() const
        {
            return XMLConstHandle( _node ? _node->PreviousSibling() : 0 );
        }

        const XMLConstHandle PreviousSiblingElement( const char* name = 0 ) const
        {
            return XMLConstHandle( _node ? _node->PreviousSiblingElement( name ) : 0 );
        }

        const XMLConstHandle NextSibling() const
        {
            return XMLConstHandle( _node ? _node->NextSibling() : 0 );
        }

        const XMLConstHandle NextSiblingElement( const char* name = 0 ) const
        {
            return XMLConstHandle( _node ? _node->NextSiblingElement( name ) : 0 );
        }

        //安全的转换为节点，可以为空。
        const XMLNode* ToNode() const               {
            return _node;
        }

        //安全的转换为元素，可以为空。
        const XMLElement* ToElement() const         {
            return ( _node ? _node->ToElement() : 0 );
        }

        //安全的转换为文本，可以为空。
        const XMLText* ToText() const               {
            return ( _node ? _node->ToText() : 0 );
        }

        //安全的转换为未知内容，可以为空。
        const XMLUnknown* ToUnknown() const         {
            return ( _node ? _node->ToUnknown() : 0 );
        }

        //安全的转换为声明，可以为空。
        const XMLDeclaration* ToDeclaration() const {
            return ( _node ? _node->ToDeclaration() : 0 );
        }

    private:
        //code
        const XMLNode* _node;
    };

    class TINYXML2_LIB XMLPrinter : public XMLVisitor{
    public:
        //code
        XMLPrinter( FILE* file=0, bool compact = false, int depth = 0 );

        virtual ~XMLPrinter()   {}

        void PushHeader( bool writeBOM, bool writeDeclaration );

        void PushDeclaration( const char* value );

        void OpenElement( const char* name, bool compactMode=false );

        virtual void CloseElement( bool compactMode=false );

        void PushAttribute( const char* name, const char* value );
        void PushAttribute( const char* name, int value );
        void PushAttribute( const char* name, unsigned value );
        void PushAttribute(const char* name, int64_t value);
        void PushAttribute( const char* name, bool value );
        void PushAttribute( const char* name, double value );

        void PushText( const char* text, bool cdata=false );
        void PushText( int value );
        void PushText( unsigned value );
        void PushText(int64_t value);
        void PushText( bool value );
        void PushText( float value );
        void PushText( double value );

        void PushComment( const char* comment );

        void PushUnknown( const char* value );

        //进入
        virtual bool VisitEnter( const XMLDocument&);

        //退出
        virtual bool VisitExit( const XMLDocument&)         {
            return true;
        }

        //进入
        virtual bool VisitEnter( const XMLElement& element, const XMLAttribute* attribute );

        //退出
        virtual bool VisitExit( const XMLElement& element );

        //访问文本
        virtual bool Visit( const XMLText& text );
        //访问注释
        virtual bool Visit( const XMLComment& comment );
        //访问声明
        virtual bool Visit( const XMLDeclaration& declaration );
        //访问未知内容
        virtual bool Visit( const XMLUnknown& unknown );

        const char* CStr() const {
            return _buffer.Mem();
        }

        int CStrSize() const {
            return _buffer.Size();
        }

        void ClearBuffer() {
            _buffer.Clear();
            _buffer.Push(0);
            _firstElement = true;
        }
        
    protected:
        //code
        virtual bool CompactMode( const XMLElement& )   { return _compactMode; }

        virtual void PrintSpace( int depth );

        void Print( const char* format, ... );

        void Write( const char* data, size_t size );

        void Putc( char ch );

        //内联调用
        inline void Write( const char* data )           { Write( data, strlen( data ) ); }

        void SealElementIfJustOpened();

        bool                            _elementJustOpened;         //元素打开标记
        DynArray< const char*, 10 >     _stack;                     //动态栈
        
    private:
        //code
        void PrintString( const char*, bool restrictedEntitySet );

        XMLPrinter( const XMLPrinter& );
        XMLPrinter& operator=( const XMLPrinter& );


        bool                _firstElement;      //首元素标记
        FILE*               _fp;                //文件处理
        int                 _depth;             //解析深度
        int                 _textDepth;         //文本深度
        bool                _processEntities;   //实体处理标记
        bool                _compactMode;       //模式标记

        enum {
                            ENTITY_RANGE = 64,  
                            BUF_SIZE = 200
        };
        bool                _entityFlag[ENTITY_RANGE];              //实体标记
        bool                _restrictedEntityFlag[ENTITY_RANGE];    //特定实体标记

        DynArray< char, 20 > _buffer;                               //动态缓存区
    };

} 



#endif