#include "tinyxml2.h"

#include <new>					//管理动态存储
#include <cstddef>				//隐式表达类型
#include <cstdarg>				//变量参数处理

//处理字符串，用于存储到缓存区，TIXML_SNPRINTF拥有snprintf功能
#define TIXML_SNPRINTF	snprintf		
//处理字符串，用于存储到缓存区，TIXML_VSNPRINTF拥有vsnprintf功能
#define TIXML_VSNPRINTF	vsnprintf		
//字符数量
static inline int TIXML_VSCPRINTF( const char* format, va_list va )
{
	int len = vsnprintf( 0, 0, format, va );
	TIXMLASSERT( len >= 0 );
	return len;
}

//输入功能，TIXML_SSCANF拥有sscanf功能
#define TIXML_SSCANF   sscanf

//换行
static const char LINE_FEED				= (char)0x0a;			
static const char LF = LINE_FEED;
//回车
static const char CARRIAGE_RETURN		= (char)0x0d;			
static const char CR = CARRIAGE_RETURN;

//单引号
static const char SINGLE_QUOTE			= '\'';
//双引号
static const char DOUBLE_QUOTE			= '\"';

//ef bb bf 指定格式 UTF-8
static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

namespace tinyxml2{
	//定义实体结构
	struct Entity {
        const char* pattern;
        int length;
        char value;
    };
	//实体长度
    static const int NUM_ENTITIES = 5;
    static const Entity entities[NUM_ENTITIES] = {
        { "quot", 4,	DOUBLE_QUOTE },
        { "amp", 3,		'&'  },
        { "apos", 4,	SINGLE_QUOTE },
        { "lt",	2, 		'<'	 },
        { "gt",	2,		'>'	 }
    };
    
    //code
    void StrPair::CollapseWhitespace()
    {
        //避免警告
        TIXMLASSERT(( _flags & NEEDS_DELETE) == 0);
        //初始化起始位置,忽略空白,调用XMLUtil中SkipWhiteSpace功能
        _start = XMLUtil::SkipWhiteSpace(_start, 0);

        if (*_start) {
            const char* p = _start;     // 读指针
            char* q = _start;           // 写指针

            while(*p) {
                //判断是否为空白
                if (XMLUtil::IsWhiteSpace(*p)) {
                    //跳过空白,换行
                    p = XMLUtil::SkipWhiteSpace(p, 0);
                    if (*p == 0) {
                        break;
                    }
                    *q = ' ';       
                    ++q;
                }
                //写入q
                *q = *p;    
                ++q;
                ++p;
            }
        //初始化q
        *q = 0;
        }
    }

    void StrPair::Reset()
    {
        if ( _flags & NEEDS_DELETE ) {
            delete [] _start;
        }
        _flags = 0;
        _start = 0;
        _end = 0;
    }

    void StrPair::SetStr( const char* str, int flags )
    {
        TIXMLASSERT( str );
        Reset();
        size_t len = strlen( str );
        TIXMLASSERT( _start == 0 );
        _start = new char[ len+1 ];
        memcpy( _start, str, len+1 );
        _end = _start + len;
        _flags = flags | NEEDS_DELETE;
    }

    const char* StrPair::GetStr()
    {
        TIXMLASSERT( _start );
        TIXMLASSERT( _end );
        //规范化类型
        if ( _flags & NEEDS_FLUSH ) {
            *_end = 0;
            _flags ^= NEEDS_FLUSH;

            if ( _flags ) {
                //读指针
                const char* p = _start; 
                //写指针
                char* q = _start;   

                while( p < _end ) {
                    //当p指向回车符，p++，如果p+1指向行换行符p+2
                    if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == CR ) {
                        if ( *(p+1) == LF ) {
                            p += 2;
                        }
                        else {
                            ++p;
                        }
                        *q = LF;
                        ++q;
                    }
                    //当p指向换行符，p+1，如果p+1指向回车符p+2
                    else if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == LF ) {
                        if ( *(p+1) == CR ) {
                            p += 2;
                        }
                        else {
                            ++p;
                        }
                        *q = LF;
                        ++q;
                    }
                    //如果p指向&，则假设后面为实体，然后读取出来
                    else if ( (_flags & NEEDS_ENTITY_PROCESSING) && *p == '&' ) {             
                        //由tinyXML2处理的实体
                        //实体表中的特殊实体[in/out]
                        //数字字符引用[in] &#20013 或 &#x4e2d
                        if ( *(p+1) == '#' ) {
                            const int buflen = 10;
                            char buf[buflen] = { 0 };
                            int len = 0;
                            //处理十个字符
                            char* adjusted = const_cast<char*>( XMLUtil::GetCharacterRef( p, buf, &len ) );
                            //没有找到实体，++p
                            if ( adjusted == 0 ) {
                                *q = *p;
                                ++p;
                                ++q;
                            }
                            //找到实体，则拷贝到q中
                            else {
                                TIXMLASSERT( 0 <= len && len <= buflen );
                                TIXMLASSERT( q + len <= adjusted );
                                p = adjusted;
                                memcpy( q, buf, len );
                                q += len;
                            }
                        }
                        //和默认实体比较
                        else {
                            bool entityFound = false;
                            for( int i = 0; i < NUM_ENTITIES; ++i ) {
                                const Entity& entity = entities[i];
                                if ( strncmp( p + 1, entity.pattern, entity.length ) == 0 && *( p + entity.length + 1 ) == ';' ) {
                                    //找到一个实体
                                    *q = entity.value;
                                    ++q;
                                    p += entity.length + 2;
                                    entityFound = true;
                                    break;
                            }
                        }
                        if ( !entityFound ) {
                            ++p;
                            ++q;
                        }
                    }
                }
                else {
                    *q = *p;
                    ++p;
                    ++q;
                }
            }
            *q = 0;
        }
            //处理空白，这个模式有待优化
            if ( _flags & NEEDS_WHITESPACE_COLLAPSING ) {
                CollapseWhitespace();
            }
            _flags = (_flags & NEEDS_DELETE);
        }
        TIXMLASSERT( _start );
        return _start;
    }

    char* StrPair::ParseText( char* p, const char* endTag, int strFlags, int* curLineNumPtr )
    {
        TIXMLASSERT( p );
        TIXMLASSERT( endTag && *endTag );
        TIXMLASSERT(curLineNumPtr);

        char* start = p;
        char  endChar = *endTag;
        size_t length = strlen( endTag );

        //解析文本
        while ( *p ) {
            //*p为结尾标签，则返回结尾指针
            if ( *p == endChar && strncmp( p, endTag, length ) == 0 ) {
                Set( start, p, strFlags );
                return p + length;
            } 
            //*p为换行符，换行
            else if (*p == '\n') {
                ++(*curLineNumPtr);
            }
            ++p;
            TIXMLASSERT( p );
        }
        return 0;
    }

    char* StrPair::ParseName( char* p )
    {
        //空字符
        if ( !p || !(*p) ) {
            return 0;
        }
        //不是名称起始字符
        if ( !XMLUtil::IsNameStartChar( *p ) ) {
            return 0;
        }
        char* const start = p;
        ++p;
        while ( *p && XMLUtil::IsNameChar( *p ) ) {
            ++p;
        }
        //写入p
        Set( start, p, 0 );
        return p;
    }

    void StrPair::TransferTo( StrPair* other )
    {
        if ( this == other ) {
            return;
        }

        TIXMLASSERT( other != 0 );
        TIXMLASSERT( other->_flags == 0 );
        TIXMLASSERT( other->_start == 0 );
        TIXMLASSERT( other->_end == 0 );

        other->Reset();
        //传递
        other->_flags = _flags;
        other->_start = _start;
        other->_end = _end;
        //充值str
        _flags = 0;
        _start = 0;
        _end = 0;
    }

    const char* XMLUtil::writeBoolTrue  = "true";
    const char* XMLUtil::writeBoolFalse = "false";

    void XMLUtil::SetBoolSerialization(const char* writeTrue, const char* writeFalse)
    {
        static const char* defTrue  = "true";
        static const char* defFalse = "false";

        writeBoolTrue = (writeTrue) ? writeTrue : defTrue;
        writeBoolFalse = (writeFalse) ? writeFalse : defFalse;
    }

    const char* XMLUtil::ReadBOM( const char* p, bool* bom )
    {
        TIXMLASSERT( p );
        TIXMLASSERT( bom );
        *bom = false;
        const unsigned char* pu = reinterpret_cast<const unsigned char*>(p);
        // 检查BOM，每次检查三个字符:
        if (    *(pu+0) == TIXML_UTF_LEAD_0
            && *(pu+1) == TIXML_UTF_LEAD_1
            && *(pu+2) == TIXML_UTF_LEAD_2 ) {
            *bom = true;
            p += 3;
        }
        TIXMLASSERT( p );
        return p;
    }

    void XMLUtil::ConvertUTF32ToUTF8( unsigned long input, char* output, int* length )
    {
        const unsigned long BYTE_MASK = 0xBF;
        const unsigned long BYTE_MARK = 0x80;
        const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
        
        //确定 UTF-8 编码字节数
        if (input < 0x80) {
            *length = 1;
        }
        else if ( input < 0x800 ) {
            *length = 2;
        }
        else if ( input < 0x10000 ) {
            *length = 3;
        }
        else if ( input < 0x200000 ) {
            *length = 4;
        }
        else {
            *length = 0;
            return;
        }

        output += *length;
        
        //根据字节数转换 UTF-8 编码
        switch (*length) {
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

    const char* XMLUtil::GetCharacterRef( const char* p, char* value, int* length )
    {
        *length = 0;
        
        //如果*(p+1)为'#'，且*(p+2)不为空，说明是合法字符
        if ( *(p+1) == '#' && *(p+2) ) {
            //初始化 ucs
            unsigned long ucs = 0;
            TIXMLASSERT( sizeof( ucs ) >= 4 );
            //增量
            ptrdiff_t delta = 0;
            unsigned mult = 1;
            static const char SEMICOLON = ';';
            
            //判断是否是 Unicode 字符
            if ( *(p+2) == 'x' ) {         
                const char* q = p+3;
                if ( !(*q) ) {
                    return 0;
                }
                
                //q指向末尾
                q = strchr( q, SEMICOLON );

                if ( !q ) {
                    return 0;
                }
                TIXMLASSERT( *q == SEMICOLON );

                delta = q-p;
                --q;

                //解析字符
                while ( *q != 'x' ) {
                    unsigned int digit = 0;
                    
                    if ( *q >= '0' && *q <= '9' ) {
                        digit = *q - '0';
                    }
                    else if ( *q >= 'a' && *q <= 'f' ) {
                        digit = *q - 'a' + 10;
                    }
                    else if ( *q >= 'A' && *q <= 'F' ) {
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
                    TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
                    ucs += digitScaled;
                    TIXMLASSERT( mult <= UINT_MAX / 16 );
                    mult *= 16;
                    --q;
                }
            }
             //*(p+2)!='x'说明为十进制
            else {           
                const char* q = p+2;
                if ( !(*q) ) {
                    return 0;
                }
                //指向末尾
                q = strchr( q, SEMICOLON );

                if ( !q ) {
                    return 0;
                }
                TIXMLASSERT( *q == SEMICOLON );

                delta = q-p;
                --q;
                //解析
                while ( *q != '#' ) {
                    if ( *q >= '0' && *q <= '9' ) {
                        const unsigned int digit = *q - '0';
                        TIXMLASSERT( digit < 10 );
                        TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );                  
                        //转换为ucs
                        const unsigned int digitScaled = mult * digit;
                        TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
                        ucs += digitScaled;
                    }
                    else {
                        return 0;
                    }
                    TIXMLASSERT( mult <= UINT_MAX / 10 );
                    mult *= 10;
                    --q;
                }
            }
            // ucs 转换为 utf-8
            ConvertUTF32ToUTF8( ucs, value, length );
            return p + delta + 1;
        }
        return p+1;
    }

    void XMLUtil::ToStr( int v, char* buffer, int bufferSize )
    {
        TIXML_SNPRINTF( buffer, bufferSize, "%d", v );
    }


    void XMLUtil::ToStr( unsigned v, char* buffer, int bufferSize )
    {
        TIXML_SNPRINTF( buffer, bufferSize, "%u", v );
    }


    void XMLUtil::ToStr( bool v, char* buffer, int bufferSize )
    {
        TIXML_SNPRINTF( buffer, bufferSize, "%s", v ? writeBoolTrue : writeBoolFalse);
    }

    void XMLUtil::ToStr( float v, char* buffer, int bufferSize )
    {
        TIXML_SNPRINTF( buffer, bufferSize, "%.8g", v );
    }


    void XMLUtil::ToStr( double v, char* buffer, int bufferSize )
    {
        TIXML_SNPRINTF( buffer, bufferSize, "%.17g", v );
    }


    void XMLUtil::ToStr(int64_t v, char* buffer, int bufferSize)
    {
        TIXML_SNPRINTF(buffer, bufferSize, "%lld", (long long)v);
    }

    bool XMLUtil::ToInt( const char* str, int* value )
    {
        if ( TIXML_SSCANF( str, "%d", value ) == 1 ) {
            return true;
        }
        return false;
    }

    bool XMLUtil::ToUnsigned( const char* str, unsigned *value )
    {
        if ( TIXML_SSCANF( str, "%u", value ) == 1 ) {
            return true;
        }
        return false;
    }

    bool XMLUtil::ToBool( const char* str, bool* value )
    {
        int ival = 0;
        if ( ToInt( str, &ival )) {
            *value = (ival==0) ? false : true;
            return true;
        }
        if ( StringEqual( str, "true" ) ) {
            *value = true;
            return true;
        }
        else if ( StringEqual( str, "false" ) ) {
            *value = false;
            return true;
        }
        return false;
    }


    bool XMLUtil::ToFloat( const char* str, float* value )
    {
        if ( TIXML_SSCANF( str, "%f", value ) == 1 ) {
            return true;
        }
        return false;
    }


    bool XMLUtil::ToDouble( const char* str, double* value )
    {
        if ( TIXML_SSCANF( str, "%lf", value ) == 1 ) {
            return true;
        }
        return false;
    }


    bool XMLUtil::ToInt64(const char* str, int64_t* value)
    {
        long long v = 0;
        if (TIXML_SSCANF(str, "%lld", &v) == 1) {
            *value = (int64_t)v;
            return true;
        }
        return false;
    }

    void XMLNode::Unlink( XMLNode* child )
    {
        //判断节点是否存在
        TIXMLASSERT( child );
        TIXMLASSERT( child->_document == _document );
        TIXMLASSERT( child->_parent == this );
        //重新链接孩子节点，_firstChild指向下一个节点
        if ( child == _firstChild ) {
            _firstChild = _firstChild->_next;
        }
        //_firstChild指向上一个节点
        if ( child == _lastChild ) {
            _lastChild = _lastChild->_prev;
        }
        //child指向下一个节点
        if ( child->_prev ) {
            child->_prev->_next = child->_next;
        }
        //child指向上一个节点
        if ( child->_next ) {
            child->_next->_prev = child->_prev;
        }
        //清空节点
        child->_next = 0;
        child->_prev = 0;
        child->_parent = 0;
    }


    void XMLNode::DeleteNode( XMLNode* node )
    {
        //如果节点为空返回
        if ( node == 0 ) {
            return;
        }
        TIXMLASSERT(node->_document);
        //检查节点是否有值
        if (!node->ToDocument()) {
          node->_document->MarkInUse(node);
      }

        //释放节点
        MemPool* pool = node->_memPool;
        node->~XMLNode();
        pool->Free( node );
    }

    void XMLNode::InsertChildPreamble( XMLNode* insertThis ) const
    {
        TIXMLASSERT( insertThis );
        TIXMLASSERT( insertThis->_document == _document );
        //断开链接
        if (insertThis->_parent) {
            insertThis->_parent->Unlink( insertThis );
        }
        //跟踪
        else {
          insertThis->_document->MarkInUse(insertThis);
          insertThis->_memPool->SetTracked();
        }
    }

    const XMLElement* XMLNode::ToElementWithName( const char* name ) const
    {
        const XMLElement* element = this->ToElement();
        if ( element == 0 ) {
            return 0;
        }
        if ( name == 0 ) {
            return element;
        }
        //比较元素名和名称
        if ( XMLUtil::StringEqual( element->Name(), name ) ) {
           return element;
       }
       return 0;
    }

    XMLNode::XMLNode( XMLDocument* doc ) :
    _document( doc ),
    _parent( 0 ),
    _value(),
    _parseLineNum( 0 ),
    _firstChild( 0 ), _lastChild( 0 ),
    _prev( 0 ), _next( 0 ),
    _userData( 0 ),
    _memPool( 0 )
    {
    }


    XMLNode::~XMLNode()
    {
        DeleteChildren();
        if ( _parent ) {
            _parent->Unlink( this );
        }
    }

    char* XMLNode::ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr )
    {
        XMLDocument::DepthTracker tracker(_document);
        if (_document->Error())
            return 0;
        //当P不为空
        while( p && *p ) {
            XMLNode* node = 0;
            //识别 p 内容
            p = _document->Identify( p, &node );
            TIXMLASSERT( p );
            if ( node == 0 ) {
                break;
            }

            //解析行数
            int initialLineNum = node->_parseLineNum;
            //找到结尾标记
            StrPair endTag;
            p = node->ParseDeep( p, &endTag, curLineNumPtr );
            //如果p为空，则报错
            if ( !p ) {
                DeleteNode( node );
                if ( !_document->Error() ) {
                    _document->SetError( XML_ERROR_PARSING, initialLineNum, 0);
                }
                break;
            }
            
            //查找声明
            XMLDeclaration* decl = node->ToDeclaration();
            if ( decl ) {
                //如果第一个节点是声明，而最后一个节点也是声明，那么到目前为止只添加了声明。
                bool wellLocated = false;
                //如果是文本内容则检查首尾是否为声明
                if (ToDocument()) {
                    if (FirstChild()) {
                        wellLocated =
                        FirstChild() &&
                        FirstChild()->ToDeclaration() &&
                        LastChild() &&
                        LastChild()->ToDeclaration();
                    }
                    else {
                        wellLocated = true;
                    }
                }
                //如果不是，则报错并且删除节点
                if ( !wellLocated ) {
                    _document->SetError( XML_ERROR_PARSING_DECLARATION, initialLineNum, "XMLDeclaration value=%s", decl->Value());
                    DeleteNode( node );
                    break;
                }
            }
            //识别元素
            XMLElement* ele = node->ToElement();
            if ( ele ) {
                //读取结束标记，将其返回给父母。
                if ( ele->ClosingType() == XMLElement::CLOSING ) {
                    if ( parentEndTag ) {
                        ele->_value.TransferTo( parentEndTag );
                    }
                    //建立一个跟踪，然后立即删除
                    node->_memPool->SetTracked();
                    DeleteNode( node );
                    return p;
                }

                //处理返回到该处的结束标记
                bool mismatch = false;
                if ( endTag.Empty() ) {
                    if ( ele->ClosingType() == XMLElement::OPEN ) {
                        mismatch = true;
                    }
                }
                else {
                    if ( ele->ClosingType() != XMLElement::OPEN ) {
                        mismatch = true;
                    }
                    else if ( !XMLUtil::StringEqual( endTag.GetStr(), ele->Name() ) ) {
                        mismatch = true;
                    }
                }
                if ( mismatch ) {
                    _document->SetError( XML_ERROR_MISMATCHED_ELEMENT, initialLineNum, "XMLElement name=%s", ele->Name());
                    DeleteNode( node );
                    break;
                }
            }
            InsertEndChild( node );
        }
        return 0;
    }

    const char* XMLNode::Value() const
    {
        // XMLDocuments 没有值,返回null。
        if ( this->ToDocument() )
            return 0;
        return _value.GetStr();
    }


    void XMLNode::SetValue( const char* str, bool staticMem )
    {   
        //以插入方式
        if ( staticMem ) {
            _value.SetInternedStr( str );
        }
        //直接赋值
        else {
            _value.SetStr( str );
        }
    }

    const XMLElement* XMLNode::PreviousSiblingElement( const char* name ) const
    {
        for( const XMLNode* node = _prev; node; node = node->_prev ) {
            const XMLElement* element = node->ToElementWithName( name );
            if ( element ) {
                return element;
            }
        }
        return 0;
    }

    const XMLElement* XMLNode::FirstChildElement( const char* name ) const
    {
        //遍历节点，找到第一个元素
        for( const XMLNode* node = _firstChild; node; node = node->_next ) {
            const XMLElement* element = node->ToElementWithName( name );
            if ( element ) {
                return element;
            }
        }
        return 0;
    }

    const XMLElement* XMLNode::LastChildElement( const char* name ) const
    {
        //遍历，从最后一个节点开始
        for( const XMLNode* node = _lastChild; node; node = node->_prev ) {
            const XMLElement* element = node->ToElementWithName( name );
            if ( element ) {
                return element;
            }
        }
        return 0;
    }

    const XMLElement* XMLNode::NextSiblingElement( const char* name ) const
    {
        for( const XMLNode* node = _next; node; node = node->_next ) {
            const XMLElement* element = node->ToElementWithName( name );
            if ( element ) {
                return element;
            }
        }
        return 0;
    }


    XMLNode* XMLNode::InsertEndChild( XMLNode* addThis )
    {
        TIXMLASSERT( addThis );
        if ( addThis->_document != _document ) {
            TIXMLASSERT( false );
            return 0;
        }
        //准备插入
        InsertChildPreamble( addThis );
        //添加
        if ( _lastChild ) {
            TIXMLASSERT( _firstChild );
            TIXMLASSERT( _lastChild->_next == 0 );
            _lastChild->_next = addThis;
            addThis->_prev = _lastChild;
            _lastChild = addThis;

            addThis->_next = 0;
        }
        //（右）子节点就是（左）子节点
        else {
            TIXMLASSERT( _firstChild == 0 );
            _firstChild = _lastChild = addThis;

            addThis->_prev = 0;
            addThis->_next = 0;
        }
        addThis->_parent = this;
        return addThis;
    }

    XMLNode* XMLNode::InsertFirstChild( XMLNode* addThis )
    {
        TIXMLASSERT( addThis );
        if ( addThis->_document != _document ) {
            TIXMLASSERT( false );
            return 0;
        }
        InsertChildPreamble( addThis );

        if ( _firstChild ) {
            TIXMLASSERT( _lastChild );
            TIXMLASSERT( _firstChild->_prev == 0 );

            _firstChild->_prev = addThis;
            addThis->_next = _firstChild;
            _firstChild = addThis;

            addThis->_prev = 0;
        }
        else {
            TIXMLASSERT( _lastChild == 0 );
            _firstChild = _lastChild = addThis;

            addThis->_prev = 0;
            addThis->_next = 0;
        }
        addThis->_parent = this;
        return addThis;
    }

    XMLNode* XMLNode::InsertAfterChild( XMLNode* afterThis, XMLNode* addThis )
    {
        TIXMLASSERT( addThis );
        if ( addThis->_document != _document ) {
            TIXMLASSERT( false );
            return 0;
        }

        TIXMLASSERT( afterThis );

        if ( afterThis->_parent != this ) {
            TIXMLASSERT( false );
            return 0;
        }
        if ( afterThis == addThis ) {
            return addThis;
        }

        if ( afterThis->_next == 0 ) {
            //最后一个节点或唯一节点。
            return InsertEndChild( addThis );
        }
        //插入准备
        InsertChildPreamble( addThis );
        //插入
        addThis->_prev = afterThis;
        addThis->_next = afterThis->_next;
        afterThis->_next->_prev = addThis;
        afterThis->_next = addThis;
        addThis->_parent = this;
        return addThis;
    }

    void XMLNode::DeleteChild( XMLNode* node )
    {
        TIXMLASSERT( node );
        TIXMLASSERT( node->_document == _document );
        TIXMLASSERT( node->_parent == this );
        Unlink( node );
        TIXMLASSERT(node->_prev == 0);
        TIXMLASSERT(node->_next == 0);
        TIXMLASSERT(node->_parent == 0);
        DeleteNode( node );
    }

    void XMLNode::DeleteChildren()
    {
        while( _firstChild ) {
            TIXMLASSERT( _lastChild );
            DeleteChild( _firstChild );
        }
        _firstChild = _lastChild = 0;
    }

    XMLNode* XMLNode::DeepClone(XMLDocument* target) const
    {
        XMLNode* clone = this->ShallowClone(target);
        if (!clone) return 0;
        //利用递归遍历
        for (const XMLNode* child = this->FirstChild(); child; child = child->NextSibling()) {
            XMLNode* childClone = child->DeepClone(target);
            TIXMLASSERT(childClone);
            clone->InsertEndChild(childClone);
        }
        return clone;
    }

    char* XMLText::ParseDeep( char* p, StrPair*, int* curLineNumPtr )
    {
        //如果为CData型，则以”]]>“作为结尾标志解析
        if ( this->CData() ) {
            p = _value.ParseText( p, "]]>", StrPair::NEEDS_NEWLINE_NORMALIZATION, curLineNumPtr );
            //错误
            if ( !p ) {
                _document->SetError( XML_ERROR_PARSING_CDATA, _parseLineNum, 0 );
            }
            return p;
        }
        //如果不是，则按照正常规则解析
        else {
            int flags = _document->ProcessEntities() ? StrPair::TEXT_ELEMENT : StrPair::TEXT_ELEMENT_LEAVE_ENTITIES;
            if ( _document->WhitespaceMode() == COLLAPSE_WHITESPACE ) {
                flags |= StrPair::NEEDS_WHITESPACE_COLLAPSING;
            }
            //以“<”作为结束标志
            p = _value.ParseText( p, "<", flags, curLineNumPtr );
            //-1，去掉“<”
            if ( p && *p ) {
                return p-1;
            }
            //错误
            if ( !p ) {
                _document->SetError( XML_ERROR_PARSING_TEXT, _parseLineNum, 0 );
            }
        }
        return 0;
    }

    bool XMLText::Accept( XMLVisitor* visitor ) const
    {
        TIXMLASSERT( visitor );
        return visitor->Visit( *this );
    }

    XMLNode* XMLText::ShallowClone( XMLDocument* doc ) const
    {
        if ( !doc ) {
            doc = _document;
        }
        //复制到text
        XMLText* text = doc->NewText( Value() );
        text->SetCData( this->CData() );
        return text;
    }

    bool XMLText::ShallowEqual( const XMLNode* compare ) const
    {
        TIXMLASSERT( compare );
        const XMLText* text = compare->ToText();
        return ( text && XMLUtil::StringEqual( text->Value(), Value() ) );
    }

    char* XMLComment::ParseDeep( char* p, StrPair*, int* curLineNumPtr )
    {
        //以"-->"结尾标志解析
        p = _value.ParseText( p, "-->", StrPair::COMMENT, curLineNumPtr );
        if ( p == 0 ) {
            _document->SetError( XML_ERROR_PARSING_COMMENT, _parseLineNum, 0 );
        }
        return p;
    }

    bool XMLComment::Accept( XMLVisitor* visitor ) const
    {
        TIXMLASSERT( visitor );
        return visitor->Visit( *this );
    }

    XMLNode* XMLComment::ShallowClone( XMLDocument* doc ) const
    {
        if ( !doc ) {
            doc = _document;
        }
        //复制到comment
        XMLComment* comment = doc->NewComment( Value() );
        return comment;
    }

    bool XMLComment::ShallowEqual( const XMLNode* compare ) const
    {
        TIXMLASSERT( compare );
        const XMLComment* comment = compare->ToComment();
        return ( comment && XMLUtil::StringEqual( comment->Value(), Value() ));
    }

    char* XMLDeclaration::ParseDeep( char* p, StrPair*, int* curLineNumPtr )
    {
        //以"?>"为结尾解析
        p = _value.ParseText( p, "?>", StrPair::NEEDS_NEWLINE_NORMALIZATION, curLineNumPtr );
        if ( p == 0 ) {
            _document->SetError( XML_ERROR_PARSING_DECLARATION, _parseLineNum, 0 );
        }
        return p;
    }

    bool XMLDeclaration::Accept( XMLVisitor* visitor ) const
    {
        TIXMLASSERT( visitor );
        return visitor->Visit( *this );
    }

    XMLNode* XMLDeclaration::ShallowClone( XMLDocument* doc ) const
    {
        if ( !doc ) {
            doc = _document;
        }
        //复制到dec
        XMLDeclaration* dec = doc->NewDeclaration( Value() );
        return dec;
    }

    bool XMLDeclaration::ShallowEqual( const XMLNode* compare ) const
    {
        TIXMLASSERT( compare );
        const XMLDeclaration* declaration = compare->ToDeclaration();
        return ( declaration && XMLUtil::StringEqual( declaration->Value(), Value() ));
    }

    bool XMLUnknown::Accept( XMLVisitor* visitor ) const
    {
        TIXMLASSERT( visitor );
        return visitor->Visit( *this );
    }

    char* XMLUnknown::ParseDeep( char* p, StrPair*, int* curLineNumPtr )
    {
        //以">"为结尾解析
        p = _value.ParseText( p, ">", StrPair::NEEDS_NEWLINE_NORMALIZATION, curLineNumPtr );
        if ( !p ) {
            _document->SetError( XML_ERROR_PARSING_UNKNOWN, _parseLineNum, 0 );
        }
        return p;
    }


    XMLNode* XMLUnknown::ShallowClone( XMLDocument* doc ) const
    {
        if ( !doc ) {
            doc = _document;
        }
        XMLUnknown* text = doc->NewUnknown( Value() );
        return text;
    }

    bool XMLUnknown::ShallowEqual( const XMLNode* compare ) const
    {
        TIXMLASSERT( compare );
        const XMLUnknown* unknown = compare->ToUnknown();
        return ( unknown && XMLUtil::StringEqual( unknown->Value(), Value() ));
    }

    void XMLAttribute::SetName( const char* n )
    {
        _name.SetStr( n );
    }

    char* XMLAttribute::ParseDeep( char* p, bool processEntities, int* curLineNumPtr )
    {
        //调用字符串解析名称函数
        p = _name.ParseName( p );
        if ( !p || !*p ) {
            return 0;
        }

        //忽略空白
        p = XMLUtil::SkipWhiteSpace( p, curLineNumPtr );
        if ( *p != '=' ) {
            return 0;
        }

        //定位到文本开头
        ++p;
        p = XMLUtil::SkipWhiteSpace( p, curLineNumPtr );
        //判断是否为双引号或者单引号开头
        if ( *p != '\"' && *p != '\'' ) {
            return 0;
        }

        //结束标志'\0'
        char endTag[2] = { *p, 0 };
        //定位到属性开头
        ++p;

        //解析
        p = _value.ParseText( p, endTag, processEntities ? StrPair::ATTRIBUTE_VALUE : StrPair::ATTRIBUTE_VALUE_LEAVE_ENTITIES, curLineNumPtr );
        return p;
    }

    const char* XMLAttribute::Name() const
    {
        return _name.GetStr();
    }

    const char* XMLAttribute::Value() const
    {
        return _value.GetStr();
    }


    XMLError XMLAttribute::QueryIntValue( int* value ) const
    {
        if ( XMLUtil::ToInt( Value(), value )) {
            return XML_SUCCESS;
        }
        return XML_WRONG_ATTRIBUTE_TYPE;
    }

    XMLError XMLAttribute::QueryUnsignedValue( unsigned int* value ) const
    {
        if ( XMLUtil::ToUnsigned( Value(), value )) {
            return XML_SUCCESS;
        }
        return XML_WRONG_ATTRIBUTE_TYPE;
    }

    XMLError XMLAttribute::QueryInt64Value(int64_t* value) const
    {
        if (XMLUtil::ToInt64(Value(), value)) {
            return XML_SUCCESS;
        }
        return XML_WRONG_ATTRIBUTE_TYPE;
    }

    XMLError XMLAttribute::QueryBoolValue( bool* value ) const
    {
        if ( XMLUtil::ToBool( Value(), value )) {
            return XML_SUCCESS;
        }
        return XML_WRONG_ATTRIBUTE_TYPE;
    }

    XMLError XMLAttribute::QueryFloatValue( float* value ) const
    {
        if ( XMLUtil::ToFloat( Value(), value )) {
            return XML_SUCCESS;
        }
        return XML_WRONG_ATTRIBUTE_TYPE;
    }

    XMLError XMLAttribute::QueryDoubleValue( double* value ) const
    {
        if ( XMLUtil::ToDouble( Value(), value )) {
            return XML_SUCCESS;
        }
        return XML_WRONG_ATTRIBUTE_TYPE;
    }

    void XMLAttribute::SetAttribute( const char* v )
    {
        _value.SetStr( v );
    }

    void XMLAttribute::SetAttribute( int v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        _value.SetStr( buf );
    }

    void XMLAttribute::SetAttribute( unsigned v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        _value.SetStr( buf );
    }

    void XMLAttribute::SetAttribute(int64_t v)
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr(v, buf, BUF_SIZE);
        _value.SetStr(buf);
    }


    void XMLAttribute::SetAttribute( bool v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        _value.SetStr( buf );
    }

    void XMLAttribute::SetAttribute( double v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        _value.SetStr( buf );
    }

    void XMLAttribute::SetAttribute( float v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        _value.SetStr( buf );
    }


    XMLElement::XMLElement( XMLDocument* doc ) : XMLNode( doc ),
    _closingType( OPEN ),
    _rootAttribute( 0 )
    {
    }


    XMLElement::~XMLElement()
    {
        //释放节点
        while( _rootAttribute ) {
            XMLAttribute* next = _rootAttribute->_next;
            DeleteAttribute( _rootAttribute );
            _rootAttribute = next;
        }
    }

    XMLAttribute* XMLElement::CreateAttribute()
    {
        //获取元素大小
        TIXMLASSERT( sizeof( XMLAttribute ) == _document->_attributePool.ItemSize() );
        //新建属性并申请内存
        XMLAttribute* attrib = new (_document->_attributePool.Alloc() ) XMLAttribute();
        TIXMLASSERT( attrib );
        //初始化内存地址
        attrib->_memPool = &_document->_attributePool;
        attrib->_memPool->SetTracked();
        return attrib;
    }

    XMLAttribute* XMLElement::FindOrCreateAttribute( const char* name )
    {
        //初始化属性表
        XMLAttribute* last = 0;
        XMLAttribute* attrib = 0;
        //检查是否存在属性
        for( attrib = _rootAttribute;
            attrib;
            last = attrib, attrib = attrib->_next ) {
            if ( XMLUtil::StringEqual( attrib->Name(), name ) ) {
                break;
            }
        }
        //如果不存在下一个属性，则创建一个
        if ( !attrib ) {
            attrib = CreateAttribute();
            TIXMLASSERT( attrib );
            //如果当前属性存在，属性表链接新属性
            if ( last ) {
                TIXMLASSERT( last->_next == 0 );
                last->_next = attrib;
            }
            //如果不存在，新属性为根属性
            else {
                TIXMLASSERT( _rootAttribute == 0 );
                _rootAttribute = attrib;
            }
            attrib->SetName( name );
        }
        return attrib;
    }

    void XMLElement::DeleteAttribute( XMLAttribute* attribute )
    {
        if ( attribute == 0 ) {
            return;
        }
        MemPool* pool = attribute->_memPool;
        //调用析构函数
        attribute->~XMLAttribute();
        //释放内存
        pool->Free( attribute );
    }

    char* XMLElement::ParseAttributes( char* p, int* curLineNumPtr )
    {
        XMLAttribute* prevAttribute = 0;

        //解析
        while( p ) {
            //跳过空白
            p = XMLUtil::SkipWhiteSpace( p, curLineNumPtr );
            //如果字符为空，报错
            if ( !(*p) ) {
                _document->SetError( XML_ERROR_PARSING_ELEMENT, _parseLineNum, "XMLElement name=%s", Name() );
                return 0;
            }

            //解析属性
            if (XMLUtil::IsNameStartChar( *p ) ) {
                XMLAttribute* attrib = CreateAttribute();
                TIXMLASSERT( attrib );
                //获取文档行号
                attrib->_parseLineNum = _document->_parseCurLineNum;
                int attrLineNum = attrib->_parseLineNum;
                //深度解析本行内容
                p = attrib->ParseDeep( p, _document->ProcessEntities(), curLineNumPtr );
                //如果字符为空或者存在属性，首先删除该属性并报错
                if ( !p || Attribute( attrib->Name() ) ) {
                    DeleteAttribute( attrib );
                    _document->SetError( XML_ERROR_PARSING_ATTRIBUTE, attrLineNum, "XMLElement name=%s", Name() );
                    return 0;
                }
                //如果存在上一个属性，则链接当前属性
                if ( prevAttribute ) {
                    TIXMLASSERT( prevAttribute->_next == 0 );
                    prevAttribute->_next = attrib;
                }
                //否则，新属性为根属性
                else {
                    TIXMLASSERT( _rootAttribute == 0 );
                    _rootAttribute = attrib;
                }
                prevAttribute = attrib;
            }
            //结束标志1
            else if ( *p == '>' ) {
                ++p;
                break;
            }
            //结束标志2
            else if ( *p == '/' && *(p+1) == '>' ) {
                _closingType = CLOSED;
                return p+2;
            }
            //其他情况则解析错误
            else {
                _document->SetError( XML_ERROR_PARSING_ELEMENT, _parseLineNum, 0 );
                return 0;
            }
        }
        return p;
    }

    char* XMLElement::ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr )
    {
        //读取元素
        p = XMLUtil::SkipWhiteSpace( p, curLineNumPtr );

        //设置结尾标志
        if ( *p == '/' ) {
            _closingType = CLOSING;
            ++p;
        }

        //解析名称
        p = _value.ParseName( p );
        if ( _value.Empty() ) {
            return 0;
        }

        //解析属性
        p = ParseAttributes( p, curLineNumPtr );
        if ( !p || !*p || _closingType != OPEN ) {
            return p;
        }

        p = XMLNode::ParseDeep( p, parentEndTag, curLineNumPtr );
        return p;
    }

    bool XMLElement::Accept( XMLVisitor* visitor ) const
    {
        TIXMLASSERT( visitor );
        //进入访问者模式
        if ( visitor->VisitEnter( *this, _rootAttribute ) ) {
            for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() ) {
                if ( !node->Accept( visitor ) ) {
                    break;
                }
            }
        }
        //退出访问者模式
        return visitor->VisitExit( *this );
    }

    const char* XMLElement::Attribute( const char* name, const char* value ) const
    {
        //查找属性
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return 0;
        }
        //返回值
        if ( !value || XMLUtil::StringEqual( a->Value(), value )) {
            return a->Value();
        }
        return 0;
    }

    int XMLElement::IntAttribute(const char* name, int defaultValue) const
    {
        int i = defaultValue;
        QueryIntAttribute(name, &i);
        return i;
    }

    unsigned XMLElement::UnsignedAttribute(const char* name, unsigned defaultValue) const
    {
        unsigned i = defaultValue;
        QueryUnsignedAttribute(name, &i);
        return i;
    }

    int64_t XMLElement::Int64Attribute(const char* name, int64_t defaultValue) const
    {
        int64_t i = defaultValue;
        QueryInt64Attribute(name, &i);
        return i;
    }

    bool XMLElement::BoolAttribute(const char* name, bool defaultValue) const
    {
        bool b = defaultValue;
        QueryBoolAttribute(name, &b);
        return b;
    }

    double XMLElement::DoubleAttribute(const char* name, double defaultValue) const
    {
        double d = defaultValue;
        QueryDoubleAttribute(name, &d);
        return d;
    }

    float XMLElement::FloatAttribute(const char* name, float defaultValue) const
    {
        float f = defaultValue;
        QueryFloatAttribute(name, &f);
        return f;
    }

    void XMLElement::DeleteAttribute( const char* name )
    {
        //prev接收属性表
        XMLAttribute* prev = 0;
        for( XMLAttribute* a=_rootAttribute; a; a=a->_next ) {
            //找到属性并断开其链表指针
            if ( XMLUtil::StringEqual( name, a->Name() ) ) {
                if ( prev ) {
                    prev->_next = a->_next;
                }
                else {
                    _rootAttribute = a->_next;
                }
                //释放
                DeleteAttribute( a );
                break;
            }
            prev = a;
        }
    }

    const XMLAttribute* XMLElement::FindAttribute( const char* name ) const
    {
        //遍历属性表
        for( XMLAttribute* a = _rootAttribute; a; a = a->_next ) {
            if ( XMLUtil::StringEqual( a->Name(), name ) ) {
                return a;
            }
        }
        return 0;
    }

    const char* XMLElement::GetText() const
    {
        if ( FirstChild() && FirstChild()->ToText() ) {
            return FirstChild()->Value();
        }
        return 0;
    }

    void XMLElement::SetText( const char* inText )
    {
        //找到第一个子节点并重新设置值
        if ( FirstChild() && FirstChild()->ToText() )
            FirstChild()->SetValue( inText );
        //直接插入
        else {
            XMLText* theText = GetDocument()->NewText( inText );
            InsertFirstChild( theText );
        }
    }


    void XMLElement::SetText( int v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        SetText( buf );
    }


    void XMLElement::SetText( unsigned v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        SetText( buf );
    }


    void XMLElement::SetText(int64_t v)
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr(v, buf, BUF_SIZE);
        SetText(buf);
    }


    void XMLElement::SetText( bool v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        SetText( buf );
    }


    void XMLElement::SetText( float v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        SetText( buf );
    }


    void XMLElement::SetText( double v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        SetText( buf );
    }

    XMLError XMLElement::QueryIntText( int* ival ) const
    {
        if ( FirstChild() && FirstChild()->ToText() ) {
            const char* t = FirstChild()->Value();
            if ( XMLUtil::ToInt( t, ival ) ) {
                return XML_SUCCESS;
            }
            return XML_CAN_NOT_CONVERT_TEXT;
        }
        return XML_NO_TEXT_NODE;
    }


    XMLError XMLElement::QueryUnsignedText( unsigned* uval ) const
    {
        if ( FirstChild() && FirstChild()->ToText() ) {
            const char* t = FirstChild()->Value();
            if ( XMLUtil::ToUnsigned( t, uval ) ) {
                return XML_SUCCESS;
            }
            return XML_CAN_NOT_CONVERT_TEXT;
        }
        return XML_NO_TEXT_NODE;
    }


    XMLError XMLElement::QueryInt64Text(int64_t* ival) const
    {
        if (FirstChild() && FirstChild()->ToText()) {
            const char* t = FirstChild()->Value();
            if (XMLUtil::ToInt64(t, ival)) {
                return XML_SUCCESS;
            }
            return XML_CAN_NOT_CONVERT_TEXT;
        }
        return XML_NO_TEXT_NODE;
    }


    XMLError XMLElement::QueryBoolText( bool* bval ) const
    {
        if ( FirstChild() && FirstChild()->ToText() ) {
            const char* t = FirstChild()->Value();
            if ( XMLUtil::ToBool( t, bval ) ) {
                return XML_SUCCESS;
            }
            return XML_CAN_NOT_CONVERT_TEXT;
        }
        return XML_NO_TEXT_NODE;
    }


    XMLError XMLElement::QueryDoubleText( double* dval ) const
    {
        if ( FirstChild() && FirstChild()->ToText() ) {
            const char* t = FirstChild()->Value();
            if ( XMLUtil::ToDouble( t, dval ) ) {
                return XML_SUCCESS;
            }
            return XML_CAN_NOT_CONVERT_TEXT;
        }
        return XML_NO_TEXT_NODE;
    }


    XMLError XMLElement::QueryFloatText( float* fval ) const
    {
        if ( FirstChild() && FirstChild()->ToText() ) {
            const char* t = FirstChild()->Value();
            if ( XMLUtil::ToFloat( t, fval ) ) {
                return XML_SUCCESS;
            }
            return XML_CAN_NOT_CONVERT_TEXT;
        }
        return XML_NO_TEXT_NODE;
    }

    int XMLElement::IntText(int defaultValue) const
    {
        int i = defaultValue;
        QueryIntText(&i);
        return i;
    }

    unsigned XMLElement::UnsignedText(unsigned defaultValue) const
    {
        unsigned i = defaultValue;
        QueryUnsignedText(&i);
        return i;
    }

    int64_t XMLElement::Int64Text(int64_t defaultValue) const
    {
        int64_t i = defaultValue;
        QueryInt64Text(&i);
        return i;
    }

    bool XMLElement::BoolText(bool defaultValue) const
    {
        bool b = defaultValue;
        QueryBoolText(&b);
        return b;
    }

    double XMLElement::DoubleText(double defaultValue) const
    {
        double d = defaultValue;
        QueryDoubleText(&d);
        return d;
    }

    float XMLElement::FloatText(float defaultValue) const
    {
        float f = defaultValue;
        QueryFloatText(&f);
        return f;
    }

    XMLNode* XMLElement::ShallowClone( XMLDocument* doc ) const
    {
        if ( !doc ) {
            doc = _document;
        }
        //新建元素列表
        XMLElement* element = doc->NewElement( Value() );
        //获取名称和值
        for( const XMLAttribute* a=FirstAttribute(); a; a=a->Next() ) {
            element->SetAttribute( a->Name(), a->Value() );
        }
        return element;
    }

    bool XMLElement::ShallowEqual( const XMLNode* compare ) const
    {
        TIXMLASSERT( compare );
        const XMLElement* other = compare->ToElement();
        //如果compare不为空，比较名称
        if ( other && XMLUtil::StringEqual( other->Name(), Name() )) {
            const XMLAttribute* a=FirstAttribute();
            const XMLAttribute* b=other->FirstAttribute();
            //如果a和b都不为空
            while ( a && b ) {
                //比较属性值
                if ( !XMLUtil::StringEqual( a->Value(), b->Value() ) ) {
                    return false;
                }
                a = a->Next();
                b = b->Next();
            }
            if ( a || b ) {
                return false;
            }
            return true;
        }
        return false;
    }

    //列表内容必须与'enum XMLError'匹配
    const char* XMLDocument::_errorNames[XML_ERROR_COUNT] = {
        "XML_SUCCESS",
        "XML_NO_ATTRIBUTE",
        "XML_WRONG_ATTRIBUTE_TYPE",
        "XML_ERROR_FILE_NOT_FOUND",
        "XML_ERROR_FILE_COULD_NOT_BE_OPENED",
        "XML_ERROR_FILE_READ_ERROR",
        "XML_ERROR_PARSING_ELEMENT",
        "XML_ERROR_PARSING_ATTRIBUTE",
        "XML_ERROR_PARSING_TEXT",
        "XML_ERROR_PARSING_CDATA",
        "XML_ERROR_PARSING_COMMENT",
        "XML_ERROR_PARSING_DECLARATION",
        "XML_ERROR_PARSING_UNKNOWN",
        "XML_ERROR_EMPTY_DOCUMENT",
        "XML_ERROR_MISMATCHED_ELEMENT",
        "XML_ERROR_PARSING",
        "XML_CAN_NOT_CONVERT_TEXT",
        "XML_NO_TEXT_NODE",
        "XML_ELEMENT_DEPTH_EXCEEDED"
    };

    void XMLDocument::Parse()
    {
        //判断释放存在节点
        TIXMLASSERT( NoChildren() );
        TIXMLASSERT( _charBuffer );
        //从第一行开始
        _parseCurLineNum = 1;
        _parseLineNum = 1;
        char* p = _charBuffer;
        p = XMLUtil::SkipWhiteSpace( p, &_parseCurLineNum );
        p = const_cast<char*>( XMLUtil::ReadBOM( p, &_writeBOM ) );
        //判断解析内容是否为空
        if ( !*p ) {
            SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
            return;
        }
        //深度解析
        ParseDeep(p, 0, &_parseCurLineNum );
    }


    void XMLDocument::SetError( XMLError error, int lineNum, const char* format, ... )
    {
        TIXMLASSERT( error >= 0 && error < XML_ERROR_COUNT );
        //获取错误ID和行号
        _errorID = error;
        _errorLineNum = lineNum;
        _errorStr.Reset();

        //默认缓存空间
        size_t BUFFER_SIZE = 1000;
        char* buffer = new char[BUFFER_SIZE];

        //打印错误
        TIXMLASSERT(sizeof(error) <= sizeof(int));
        TIXML_SNPRINTF(buffer, BUFFER_SIZE, "Error=%s ErrorID=%d (0x%x) Line number=%d", ErrorIDToName(error), int(error), int(error), lineNum);

        //如果传入另外的参数，则将其转换为字符串黏贴在buffer后面
        if (format) {
          size_t len = strlen(buffer);
          TIXML_SNPRINTF(buffer + len, BUFFER_SIZE - len, ": ");
          len = strlen(buffer);

          va_list va;
          va_start(va, format);
          TIXML_VSNPRINTF(buffer + len, BUFFER_SIZE - len, format, va);
          va_end(va);
      }
      _errorStr.SetStr(buffer);
      //释放
      delete[] buffer;
    }

    void XMLDocument::PushDepth()
    {
        _parsingDepth++;
        //如果等于最大深度，则警告
        if (_parsingDepth == TINYXML2_MAX_ELEMENT_DEPTH) {
            SetError(XML_ELEMENT_DEPTH_EXCEEDED, _parseCurLineNum, "Element nesting is too deep." );
        }
    }

    void XMLDocument::PopDepth()
    {
        TIXMLASSERT(_parsingDepth > 0);
        --_parsingDepth;
    }

    XMLDocument::XMLDocument( bool processEntities, Whitespace whitespaceMode ) :
    XMLNode( 0 ),
    _writeBOM( false ),
    _processEntities( processEntities ),
    _errorID(XML_SUCCESS),
    _whitespaceMode( whitespaceMode ),
    _errorStr(),
    _errorLineNum( 0 ),
    _charBuffer( 0 ),
    _parseCurLineNum( 0 ),
    _parsingDepth(0),
    _unlinked(),
    _elementPool(),
    _attributePool(),
    _textPool(),
    _commentPool()
    {
        _document = this;
    }


    XMLDocument::~XMLDocument()
    {
        Clear();
    }

    XMLError XMLDocument::Parse( const char* p, size_t len )
    {
        Clear();
        //判断字符串长度
        if ( len == 0 || !p || !*p ) {
            SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
            return _errorID;
        }
        if ( len == (size_t)(-1) ) {
            len = strlen( p );
        }
        //初始化_charBuffer
        TIXMLASSERT( _charBuffer == 0 );
        _charBuffer = new char[ len+1 ];
        //把字符串拷贝到_charBuffer
        memcpy( _charBuffer, p, len );
        _charBuffer[len] = 0;
        //深度解析
        Parse();
        //如果报警，则清空内存池
        if ( Error() ) {
            DeleteChildren();
            _elementPool.Clear();
            _attributePool.Clear();
            _textPool.Clear();
            _commentPool.Clear();
        }
        return _errorID;
    }   

    //辅助打开文件函数
    static FILE* callfopen( const char* filepath, const char* mode )
    {
        TIXMLASSERT( filepath );
        TIXMLASSERT( mode );
        FILE* fp = fopen( filepath, mode );
        return fp;
    }

    XMLError XMLDocument::LoadFile( const char* filename )
    {
        //检查文件名
        if ( !filename ) {
            TIXMLASSERT( false );
            SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=<null>" );
            return _errorID;
        }

        Clear();
        //打开二进制文件
        FILE* fp = callfopen( filename, "rb" );
        if ( !fp ) {
            SetError( XML_ERROR_FILE_NOT_FOUND, 0, "filename=%s", filename );
            return _errorID;
        }
        //加载文件2
        LoadFile( fp );
        //关闭文件
        fclose( fp );
        return _errorID;
    }

    /*这两个模板是用来检测文件长度，如果size_t类型大于无符号长类型，则检查是多余的，gcc和clang会发出Wtype-limits警告。*/
    template<bool = (sizeof(unsigned long) >= sizeof(size_t))>
    struct LongFitsIntoSizeTMinusOne {
        static bool Fits( unsigned long value )
        {
            return value < (size_t)-1;
        }
    };

    template <>
    struct LongFitsIntoSizeTMinusOne<false> {
        static bool Fits( unsigned long )
        {
            return true;
        }
    };

    XMLError XMLDocument::LoadFile( FILE* fp )
    {
        Clear();
        //定位到文件开头
        fseek( fp, 0, SEEK_SET );
        //文件读取错误
        if ( fgetc( fp ) == EOF && ferror( fp ) != 0 ) {
            SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
            return _errorID;
        }
        //定位到文件结尾
        fseek( fp, 0, SEEK_END );
        //获取当前偏移字节数（在这里值文件总长度）
        const long filelength = ftell( fp );
        //重新定位到开头
        fseek( fp, 0, SEEK_SET );
        
        //文件长度过大警告
        if ( filelength == -1L ) {
            SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
            return _errorID;
        }
        TIXMLASSERT( filelength >= 0 );

        //检查文件长度，无法处理与空终止符一起放入缓冲区中的文件
        if ( !LongFitsIntoSizeTMinusOne<>::Fits( filelength ) ) {
            SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
            return _errorID;
        }

        if ( filelength == 0 ) {
            SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
            return _errorID;
        }

        const size_t size = filelength;
        TIXMLASSERT( _charBuffer == 0 );
        //初始化_charBuffer
        _charBuffer = new char[size+1];
        //将文件读到_charBuffer
        size_t read = fread( _charBuffer, 1, size, fp );
        if ( read != size ) {
            SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
            return _errorID;
        }
        //添加终止符
        _charBuffer[size] = 0;

        Parse();
        return _errorID;
    }

    XMLError XMLDocument::SaveFile( const char* filename, bool compact )
    {
        //检查文件名
        if ( !filename ) {
            TIXMLASSERT( false );
            SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=<null>" );
            return _errorID;
        }
        
        //以写模式打开文件
        FILE* fp = callfopen( filename, "w" );
        if ( !fp ) {
            SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=%s", filename );
            return _errorID;
        }
        //保存文件2
        SaveFile(fp, compact);
        fclose( fp );
        return _errorID;
    }

    XMLError XMLDocument::SaveFile( FILE* fp, bool compact )
    {
        //清除上次保存中的所有错误
        ClearError();
        //写入文件
        XMLPrinter stream( fp, compact );
        Print( &stream );
        return _errorID;
    }

    void XMLDocument::Print( XMLPrinter* streamer ) const
    {
        if ( streamer ) {
            Accept( streamer );
        }
        else {
            XMLPrinter stdoutStreamer( stdout );
            Accept( &stdoutStreamer );
        }
    }

    bool XMLDocument::Accept( XMLVisitor* visitor ) const
    {
        TIXMLASSERT( visitor );
        if ( visitor->VisitEnter( *this ) ) {
            for ( const XMLNode* node=FirstChild(); node; node=node->NextSibling() ) {
                if ( !node->Accept( visitor ) ) {
                    break;
                }
            }
        }
        return visitor->VisitExit( *this );
    }

    XMLElement* XMLDocument::NewElement( const char* name )
    {
        XMLElement* ele = CreateUnlinkedNode<XMLElement>( _elementPool );
        ele->SetName( name );
        return ele;
    }

    XMLComment* XMLDocument::NewComment( const char* str )
    {
        XMLComment* comment = CreateUnlinkedNode<XMLComment>( _commentPool );
        comment->SetValue( str );
        return comment;
    }

    XMLText* XMLDocument::NewText( const char* str )
    {
        XMLText* text = CreateUnlinkedNode<XMLText>( _textPool );
        text->SetValue( str );
        return text;
    }

    XMLDeclaration* XMLDocument::NewDeclaration( const char* str )
    {
        XMLDeclaration* dec = CreateUnlinkedNode<XMLDeclaration>( _commentPool );
        dec->SetValue( str ? str : "xml version=\"1.0\" encoding=\"UTF-8\"" );
        return dec;
    }

    XMLUnknown* XMLDocument::NewUnknown( const char* str )
    {
        XMLUnknown* unk = CreateUnlinkedNode<XMLUnknown>( _commentPool );
        unk->SetValue( str );
        return unk;
    }

    void XMLDocument::DeleteNode( XMLNode* node )   {
        TIXMLASSERT( node );
        TIXMLASSERT(node->_document == this );
        if (node->_parent) {
            node->_parent->DeleteChild( node );
        }
        else {    
            //如果不在树上，则直接删除
            node->_memPool->SetTracked();
            XMLNode::DeleteNode(node);
        }
    }

    const char* XMLDocument::ErrorIDToName(XMLError errorID)
    {
        TIXMLASSERT( errorID >= 0 && errorID < XML_ERROR_COUNT );
        const char* errorName = _errorNames[errorID];
        TIXMLASSERT( errorName && errorName[0] );
        return errorName;
    }

    const char* XMLDocument::ErrorName() const
    {
        return ErrorIDToName(_errorID);
    }


    const char* XMLDocument::ErrorStr() const
    {
        //如果为空则返回空，构造返回错误描述
        return _errorStr.Empty() ? "" : _errorStr.GetStr();
    }

    void XMLDocument::PrintError() const
    {
        printf("%s\n", ErrorStr());
    }

    void Clear();

    void XMLDocument::Clear()
    {
        //删除孩子节点
        DeleteChildren();
        //删除未链接节点
        while( _unlinked.Size()) {
            DeleteNode(_unlinked[0]);
        }

    //DEBUG调试错误
    #ifdef TINYXML2_DEBUG
        const bool hadError = Error();
    #endif
        //清空错误
        ClearError();
        //释放缓存
        delete [] _charBuffer;
        _charBuffer = 0;
        _parsingDepth = 0;

    //跟踪
    #if 0
        _textPool.Trace( "text" );
        _elementPool.Trace( "element" );
        _commentPool.Trace( "comment" );
        _attributePool.Trace( "attribute" );
    #endif

    //调试未知错误
    #ifdef TINYXML2_DEBUG
        if ( !hadError ) {
            TIXMLASSERT( _elementPool.CurrentAllocs()   == _elementPool.Untracked() );
            TIXMLASSERT( _attributePool.CurrentAllocs() == _attributePool.Untracked() );
            TIXMLASSERT( _textPool.CurrentAllocs()      == _textPool.Untracked() );
            TIXMLASSERT( _commentPool.CurrentAllocs()   == _commentPool.Untracked() );
        }
    #endif
    }

    void XMLDocument::DeepCopy(XMLDocument* target) const
    {
        TIXMLASSERT(target);
        //如果两者相同
        if (target == this) {
            return; 
        }

        //清空目标
        target->Clear();
        //复制到目标
        for (const XMLNode* node = this->FirstChild(); node; node = node->NextSibling()) {
          target->InsertEndChild(node->DeepClone(target));
      }
    }

    char* XMLDocument::Identify( char* p, XMLNode** node )
    {
        TIXMLASSERT( node );
        TIXMLASSERT( p );
        //从文档开头开始解析
        char* const start = p;
        int const startLine = _parseCurLineNum;
        p = XMLUtil::SkipWhiteSpace( p, &_parseCurLineNum );
        if( !*p ) {
            *node = 0;
            TIXMLASSERT( p );
            return p;
        }

        //这些字符串定义匹配模式
        static const char* xmlHeader        = { "<?" };
        static const char* commentHeader    = { "<!--" };
        static const char* cdataHeader      = { "<![CDATA[" };
        static const char* dtdHeader        = { "<!" };
        static const char* elementHeader    = { "<" };  
        static const int xmlHeaderLen       = 2;
        static const int commentHeaderLen   = 4;
        static const int cdataHeaderLen     = 9;
        static const int dtdHeaderLen       = 2;
        static const int elementHeaderLen   = 1;

        TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLUnknown ) );        
        TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLDeclaration ) );    
        XMLNode* returnNode = 0;
        
        //匹配开始标记
        if ( XMLUtil::StringEqual( p, xmlHeader, xmlHeaderLen ) ) {
            returnNode = CreateUnlinkedNode<XMLDeclaration>( _commentPool );
            returnNode->_parseLineNum = _parseCurLineNum;
            p += xmlHeaderLen;
        }
        //匹配注释标记
        else if ( XMLUtil::StringEqual( p, commentHeader, commentHeaderLen ) ) {
            returnNode = CreateUnlinkedNode<XMLComment>( _commentPool );
            returnNode->_parseLineNum = _parseCurLineNum;
            p += commentHeaderLen;
        }
        //匹配cdata标记
        else if ( XMLUtil::StringEqual( p, cdataHeader, cdataHeaderLen ) ) {
            XMLText* text = CreateUnlinkedNode<XMLText>( _textPool );
            returnNode = text;
            returnNode->_parseLineNum = _parseCurLineNum;
            p += cdataHeaderLen;
            text->SetCData( true );
        }
        //匹配dtd标记
        else if ( XMLUtil::StringEqual( p, dtdHeader, dtdHeaderLen ) ) {
            returnNode = CreateUnlinkedNode<XMLUnknown>( _commentPool );
            returnNode->_parseLineNum = _parseCurLineNum;
            p += dtdHeaderLen;
        }
        //匹配元素标记
        else if ( XMLUtil::StringEqual( p, elementHeader, elementHeaderLen ) ) {
            returnNode =  CreateUnlinkedNode<XMLElement>( _elementPool );
            returnNode->_parseLineNum = _parseCurLineNum;
            p += elementHeaderLen;
        }
        //其他清空按文本内容处理
        else {
            returnNode = CreateUnlinkedNode<XMLText>( _textPool );
            //报告第一个非空白字符的行
            returnNode->_parseLineNum = _parseCurLineNum; 
            p = start;  // 备份
            _parseCurLineNum = startLine;
        }

        TIXMLASSERT( returnNode );
        TIXMLASSERT( p );
        *node = returnNode;
        return p;
    }

    void XMLDocument::MarkInUse(XMLNode* node)
    {
        TIXMLASSERT(node);
        TIXMLASSERT(node->_parent == 0);
        //如果没有父节点则删除
        for (int i = 0; i < _unlinked.Size(); ++i) {
            if (node == _unlinked[i]) {
                _unlinked.SwapRemove(i);
                break;
            }
        }
    }

    void XMLPrinter::PrintString( const char* p, bool restricted )
    {
        //查找要打印的实体之间的字节。
        const char* q = p;
        //处理实体
        if ( _processEntities ) {
            const bool* flag = restricted ? _restrictedEntityFlag : _entityFlag;
            while ( *q ) {
                TIXMLASSERT( p <= q );
                if ( *q > 0 && *q < ENTITY_RANGE ) {
                    // 如果检测到实体，则写入实体并继续查找
                    if ( flag[(unsigned char)(*q)] ) {
                        while ( p < q ) {
                            const size_t delta = q - p;     //增量
                            const int toPrint = ( INT_MAX < delta ) ? INT_MAX : (int)delta; //打印字节长度
                            Write( p, toPrint );
                            p += toPrint;
                        }
                        
                        //打印实体模型（可以查看实验一实体原理）
                        bool entityPatternPrinted = false;
                        for( int i=0; i<NUM_ENTITIES; ++i ) {
                            if ( entities[i].value == *q ) {
                                Putc( '&' );
                                Write( entities[i].pattern, entities[i].length );
                                Putc( ';' );
                                entityPatternPrinted = true;
                                break;
                            }
                        }
                        //找不到实体，报错
                        if ( !entityPatternPrinted ) {
                            TIXMLASSERT( false );
                        }
                        ++p;
                    }
                }
                ++q;
                TIXMLASSERT( p <= q );
            }
            
            //写入剩余的字符串，如果找不到实体，则写入整个字符串
            if ( p < q ) {
                const size_t delta = q - p;
                const int toPrint = ( INT_MAX < delta ) ? INT_MAX : (int)delta;
                Write( p, toPrint );
            }
        }
        else {
            Write( p );
        }
    }

    void XMLPrinter::PrintSpace( int depth )
    {
        for( int i=0; i<depth; ++i ) {
            Write( "    " );
        }
    }

    void XMLPrinter::Print( const char* format, ... )
    {
        va_list     va;                     //参数获取列表
        va_start( va, format );             //指向第一个参数

        //如果打开文件，直接写入
        if ( _fp ) {
            vfprintf( _fp, format, va );
        }
        
        //否则写入缓存
        else {
            const int len = TIXML_VSCPRINTF( format, va );
            //关闭并重新启动va 
            va_end( va );
            TIXMLASSERT( len >= 0 );
            va_start( va, format );
            TIXMLASSERT( _buffer.Size() > 0 && _buffer[_buffer.Size() - 1] == 0 );
            //增加终止符
            char* p = _buffer.PushArr( len ) - 1;
            //写入p
            TIXML_VSNPRINTF( p, len+1, format, va );
        }
        va_end( va );
    }

    void XMLPrinter::Write( const char* data, size_t size )
    {
        //如果已打开文件，直接写入
        if ( _fp ) {
            fwrite ( data , sizeof(char), size, _fp);
        }
        //否则，先存入缓存
        else {
            //最后一位是空终止符
            char* p = _buffer.PushArr( static_cast<int>(size) ) - 1; 
            memcpy( p, data, size );
            p[size] = 0;
        }
    }

    void XMLPrinter::Putc( char ch )
    {
        if ( _fp ) {
            fputc ( ch, _fp);
        }
        else {
            char* p = _buffer.PushArr( sizeof(char) ) - 1;
            p[0] = ch;
            p[1] = 0;
        }
    }

    void XMLPrinter::SealElementIfJustOpened()
    {
        if ( !_elementJustOpened ) {
            return;
        }
        _elementJustOpened = false;
        Putc( '>' );
    }

    XMLPrinter::XMLPrinter( FILE* file, bool compact, int depth ) :
    _elementJustOpened( false ),
    _stack(),
    _firstElement( true ),
    _fp( file ),
    _depth( depth ),
    _textDepth( -1 ),
    _processEntities( true ),
    _compactMode( compact ),
    _buffer()
    {
        //初始化标记
        for( int i=0; i<ENTITY_RANGE; ++i ) {
            _entityFlag[i] = false;
            _restrictedEntityFlag[i] = false;
        }
        
        //初始化实体
        for( int i=0; i<NUM_ENTITIES; ++i ) {
            const char entityValue = entities[i].value;
            const unsigned char flagIndex = (unsigned char)entityValue;
            TIXMLASSERT( flagIndex < ENTITY_RANGE );
            _entityFlag[flagIndex] = true;
        }
        
        //特定实体标记
        _restrictedEntityFlag[(unsigned char)'&'] = true;
        _restrictedEntityFlag[(unsigned char)'<'] = true;
        _restrictedEntityFlag[(unsigned char)'>'] = true;
        
        //初始化缓存
        _buffer.Push( 0 );
    }

    void XMLPrinter::PushHeader( bool writeBOM, bool writeDec )
    {
        //写入BOM，utf-8
        if ( writeBOM ) {
            static const unsigned char bom[] = { TIXML_UTF_LEAD_0, TIXML_UTF_LEAD_1, TIXML_UTF_LEAD_2, 0 };
            Write( reinterpret_cast< const char* >( bom ) );
        }
        //写入声明
        if ( writeDec ) {
            PushDeclaration( "xml version=\"1.0\"" );
        }
    }

    //注释是以"<?"开始，"?>"结尾
    void XMLPrinter::PushDeclaration( const char* value )
    {
        SealElementIfJustOpened();
        //检查解析方式
        if ( _textDepth < 0 && !_firstElement && !_compactMode) {
            Putc( '\n' );
            PrintSpace( _depth );
        }
        _firstElement = false;

        Write( "<?" );
        Write( value );
        Write( "?>" );
    }

    void XMLPrinter::OpenElement( const char* name, bool compactMode )
    {
        //检查是否结尾
        SealElementIfJustOpened();
        _stack.Push( name );

        //如果不是第一个元素，换行
        if ( _textDepth < 0 && !_firstElement && !compactMode ) {
            Putc( '\n' );
        }
        
        //非紧凑模式，添加空格
        if ( !compactMode ) {
            PrintSpace( _depth );
        }

        //开始编写
        Write ( "<" );
        Write ( name );

        _elementJustOpened = true;
        _firstElement = false;
        ++_depth;
    }

    void XMLPrinter::CloseElement( bool compactMode )
    {
        --_depth;
        const char* name = _stack.Pop();

        //元素已有开始标记
        if ( _elementJustOpened ) {
            Write( "/>" );
        }
        //重新写入元素
        else {
            if ( _textDepth < 0 && !compactMode) {
                Putc( '\n' );
                PrintSpace( _depth );
            }
            Write ( "</" );
            Write ( name );
            Write ( ">" );
        }
        
        //文本深度--
        if ( _textDepth == _depth ) {
            _textDepth = -1;
        }
        
        //解析升读为0则换行
        if ( _depth == 0 && !compactMode) {
            Putc( '\n' );
        }
        _elementJustOpened = false;
    }

    void XMLPrinter::PushAttribute( const char* name, const char* value )
    {
        TIXMLASSERT( _elementJustOpened );
        //添加空格
        Putc ( ' ' );
        //写入名称
        Write( name );
        //写入” =" “
        Write( "=\"" );
        //写入值
        PrintString( value, false );
        //写入” " “
        Putc ( '\"' );
    }


    void XMLPrinter::PushAttribute( const char* name, int v )
    {
        char buf[BUF_SIZE];
        //转换为字符串，下同
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        //调用重载函数，下同
        PushAttribute( name, buf );
    }


    void XMLPrinter::PushAttribute( const char* name, unsigned v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        PushAttribute( name, buf );
    }


    void XMLPrinter::PushAttribute(const char* name, int64_t v)
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr(v, buf, BUF_SIZE);
        PushAttribute(name, buf);
    }


    void XMLPrinter::PushAttribute( const char* name, bool v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        PushAttribute( name, buf );
    }


    void XMLPrinter::PushAttribute( const char* name, double v )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( v, buf, BUF_SIZE );
        PushAttribute( name, buf );
    }

    void XMLPrinter::PushText( const char* text, bool cdata )
    {
        _textDepth = _depth-1;
        SealElementIfJustOpened();
        //cdata格式
        if ( cdata ) {
            Write( "<![CDATA[" );
            Write( text );
            Write( "]]>" );
        }
        //普通格式
        else {
            PrintString( text, true );
        }
    }

    void XMLPrinter::PushText( int64_t value )
    {
        char buf[BUF_SIZE];
        //转换为字符串，下同
        XMLUtil::ToStr( value, buf, BUF_SIZE );
        //调用重载函数，下同
        PushText( buf, false );
    }

    void XMLPrinter::PushText( int value )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( value, buf, BUF_SIZE );
        PushText( buf, false );
    }


    void XMLPrinter::PushText( unsigned value )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( value, buf, BUF_SIZE );
        PushText( buf, false );
    }


    void XMLPrinter::PushText( bool value )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( value, buf, BUF_SIZE );
        PushText( buf, false );
    }


    void XMLPrinter::PushText( float value )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( value, buf, BUF_SIZE );
        PushText( buf, false );
    }


    void XMLPrinter::PushText( double value )
    {
        char buf[BUF_SIZE];
        XMLUtil::ToStr( value, buf, BUF_SIZE );
        PushText( buf, false );
    }

    void XMLPrinter::PushComment( const char* comment )
    {
        SealElementIfJustOpened();
        //检查解析方式
        if ( _textDepth < 0 && !_firstElement && !_compactMode) {
            Putc( '\n' );
            PrintSpace( _depth );
        }
        _firstElement = false;

        Write( "<!--" );
        Write( comment );
        Write( "-->" );
    }

    void XMLPrinter::PushUnknown( const char* value )
    {
        SealElementIfJustOpened();
        if ( _textDepth < 0 && !_firstElement && !_compactMode) {
            Putc( '\n' );
            PrintSpace( _depth );
        }
        _firstElement = false;

        Write( "<!" );
        Write( value );
        Putc( '>' );
    }

    bool XMLPrinter::VisitEnter( const XMLDocument& doc )
    {
        _processEntities = doc.ProcessEntities();
        //如果存在utf-8格式
        if ( doc.HasBOM() ) {
            PushHeader( true, false );
        }
        return true;
    }

    bool XMLPrinter::VisitEnter( const XMLElement& element, const XMLAttribute* attribute )
    {
        //初始化元素
        const XMLElement* parentElem = 0;
        //如果存在元素，获取
        if ( element.Parent() ) {
            parentElem = element.Parent()->ToElement();
        }
        
        //写入模式
        const bool compactMode = parentElem ? CompactMode( *parentElem ) : _compactMode;
        
        //添加元素
        OpenElement( element.Name(), compactMode );
        
        //添加元素
        while ( attribute ) {
            PushAttribute( attribute->Name(), attribute->Value() );
            attribute = attribute->Next();
        }
        return true;
    }

    //退出并封闭元素
    bool XMLPrinter::VisitExit( const XMLElement& element )
    {
        CloseElement( CompactMode(element) );
        return true;
    }    

    bool XMLPrinter::Visit( const XMLText& text )
    {
        //调用添加接口，下同
        PushText( text.Value(), text.CData() );
        return true;
    }


    bool XMLPrinter::Visit( const XMLComment& comment )
    {
        PushComment( comment.Value() );
        return true;
    }

    bool XMLPrinter::Visit( const XMLDeclaration& declaration )
    {
        PushDeclaration( declaration.Value() );
        return true;
    }


    bool XMLPrinter::Visit( const XMLUnknown& unknown )
    {
        PushUnknown( unknown.Value() );
        return true;
    }


} 