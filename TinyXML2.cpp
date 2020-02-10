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
                    TIXMLASSERT( digit < 10 );
                    TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );

                    //转换为ucs
                    const unsigned int digitScaled = mult * digit;
                    TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
                    ucs += digitScaled;
                }
                else{
                    return 0;
                }
                TIXMLASSERT( mult <= UINT_MAX / 10 );
                mult *= 10;
                --q;
            }
        }
        //ucs 转换为 utf-8
        ConvertUTF32ToUTF8( ucs, value, length );
        return p + delta + 1;
    }
    return p+1;
}
//原始类型转换位字符串实现
void XMLUtil::ToStr( int v, void char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( bufffer, bufferSize, "%d", v );
}

void XMLUtil::ToStr( unsigned v, void char* buffer, int bufferSize )
{
    TIXML_SNPRITF( buffer, bufferSize, "%u", v );
}
void XMLUtil::ToStr( bool v, void char* buffer, int bufferSize )
{
    TIXML_SNPRINT( buffer, bufferSize, "%s", v );
}
void XMLUtil::ToStr( float v, void char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%.8g", v );
}
void XMLUtil::ToStr( double v, void char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%.17g", v );
}
void XMLUtil::ToStr( int64_t v, void char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%lld", (long long)v );
}


//字符串转换位基本类型实现
static bool XMLUtil:: ToInt( const char* str, int* value )
{
    if( TIXML_SSCANF( str, "%d", value ) == 1 ){
        return true;
    }
    return false;
}
static bool XMLUtil:: ToUnsigned( const char* str, unsigned* value ){
   if( TIXML_SSCANF( str, "%u", value ) == 1 ){
       return true;
   }
   return false;
}
static bool XMLUtil::ToBool( const char* str, bool* value ){
    int ival = 0;
    if( ToInt(str, &ival )){
        *value = (ival == 0) ? false : true;
        return true;
    }
    if( StringEqual( str, "true" ) ){
        *value = true;
        return true;
    }
    else if( StringEqual( str, "false" ) ){
        *value = false;
        return true;
    }
    return false;
}

static bool XMLUtil::ToFloat( const char* str, float* value ){
    if( TIXML_SSCANF( str, "%f", value ) == 1 ){
        return true;
    }
    return false;
}
static bool XMLUtil::ToDouble( const char* str, double* value ){
    if( TIXML_SSCANF( str, "%lf", value ) == 1){
        return true;
    }
    return false;
}
static bool XMLUtil::ToInt64( const char* str, int64_t* value ){
    long long v = 0;
    if( TIXML_SSCANF( str, "%lld", &v ) == 1 ){
        *value = (int64_t)v;
        return true;
    }
    return false;
}


//断开孩子节点实现
void XMLNode::Unlike( XMLNode* child )
{
    //判断节点是否存在
    TIXMLASSERT( child );
    TIXMLASSERT( child->_document == _document );
    TIXMLASSERT( child->_parent == this );

    //重新链接孩子节点, _firstChild指向下一个节点
    if( child == _firstChild ){
        _firstChild = _firstChild->_next;
    }

    //_firstChild指向上一个节点
    if( child == _lastChild ){
        _lastChild = _lastChild->prev;
    }

    //child指向下一个节点
    if( child->_next ){
        child->_next->_prev = child->_prev;
    }

    //清空节点
    child->_next = 0;
    child->_prev = 0;
    child->_parent = 0;
}

void XMLNode*::DeleteNode( XMLNode* node )
{
    //如果节点为空返回
    if( node == 0 ){
        return;
    }
    TIXMLASSERT( node->_document );
    //检查节点是否存在值
    if( !node->ToDocument() ){
        node->_document->MarkInUse(node);
    }

    //释放节点
    MemPool* pool = node->_memPool;
    node->~XMLNode();
    pool->Free( node );
}

//检查节点是否有链接及相关操作实现

void XMLNode::InsertChildPreamble( XMLNode* insertThis)
{
    TIXMLASSERT( insertThis );
    TIXMLASEERT( insertThis->_document == _document );
    //断开链接
    if(insertThis->_partent){
        insertThis->_parent->Unlink( insertThis );
    }
    //跟踪
    else{
        insertThis->_document->MarkInUse(insertThis);
        insertThis->_memPool->SetTracked();
    }
}

//判断元素名称是否存在实现
const XMLElement* XMLNode::ToElementWithName( const char* name ) const{
    const XMLElement* element = this->ToElement();

    if(element == 0){
        return 0;
    }

    if(name == 0){
        return element;
    }

    //比较元素名和名称
    if( XMLUtil::StringEqual( element->Name(), name  ) ){
        return element;
    }
    return 0;
}

//XMLNode 构造和析构函数
XMLNode::XMLNode( XMLDocument* doc):
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
    if( _parent ){
        _parent->Unlink( this );
    }
}

//深度解析实现
char* XMLNode::ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr )
{
    XMLDocument::DepthTracker tracker( _document );
    if( _document->Error() )
        return 0;

    //当p不为空
    while( p && *p ){
        XMLNode* node = 0;
        //识别 p 内容
        p = _document->Identify( p, &node );
        TIXMLASSERT( p );
        if( node == 0 ){
            break;
        }

        //当前解析行数
        int initialLineNum = node->_parseLineNum;
        //找到结尾标记
        StrPari endTag;
        p = node->ParseDeep( p, &endTag, curLineNumPtr );
        //如果p为空,则报错
        if( !p ){
            DeleteNode( node );
            if( !_document->Error() ){
                _document->SetError( XML_ERROR_PARSING, initialLineNum, 0 );
            }
            break;
        }

        //查找声明
        XMLDeclaration* decl = node->ToDeclaration();
        if( decl ){
            //如果第一个节点是声明,而最后一个节点也是声明,那么到目前为止只添加了声明
            bool wellLocated = false;

            //如果是文本内容则检查首尾是否为声明
            if(ToDocument()){
                if(FirstChild()){
                    wellLocated =
                        FirstChild() &&
                        FirstChild()->ToDeclaration() &&
                        Lastchild &&
                        LastChild()->ToDeclaration();
                }
                else{
                    wellLocated = true;
                }
            }
            //如果不是,则报错并且删除节点
            if( !wellLocated ){
                _document->SetError( XML_ERROR_PARSING_DECLARATION, initialLineNum, "XMLDeclaration value=%s", decl->Value());
                DeleteNode( node );
                break;
            }
        }

        //识别元素
        XMLElement* ele = node->ToElement();
        if( ele ){
            //读取结束标记,将其返回给父母
            if( ele->ClosingType() == XMLElement::CLOSING ){
                if( parentEndTag ){
                    ele->_value.TransferTo( parentEndTag );
                }
                //建立一个跟踪,然后立即删除
                node->_memPool->SetTracked();
                DeleteNode( node );
                return p;
            }
            //处理返回到该处的结束标记,初始为不匹配
            bool mismatch = false;
            if( endTag.Empty() ){
                //结束标志为 " < ", 不匹配
                if( ele->ClosingType() == XMLElement::OPEN ){
                    mismatch = false;
                }
            }
            else{
                if( ele->ClosingType() != XMLElement::OPEN ){
                    mismatch = true;
                }
            }
            if( mismatch ){
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
    //XMLDocuments 没有值,返回null
    if( this->ToDocument() )
        return 0;
    return _value.GetStr();
}
void XMLNode::SetValue( const char* str, bool staticMem )
{
    //以插入方式
    if( staticMem ){
        _value.SetInternedStr( str );
    }
    //直接赋值
    else{
        _value_SetStr( str );
    }
}


const XMLElement* XMLNode::PreviousSiblingElement( const char* name ) const
{
    for( const XMLNode* node = _prev; node; node = node->prev ){
        const XMLElement* element = node->ToElementWithName( name );
        if( element ){
            return element;
        }
    }
    return 0;
}

const XMLElement* XMLNode::FirstChildElement( const char* name ) const
{
    //遍历节点,找到第一个元素
    for( const XMLNode* node = _firstChild; node; node = node->_next ){
        const XMLElement* element = node->ToElementWithName( name );
        if( element ){
            return element;
        }
    }
    return 0;
}

const XMLElement* XMLNode::LastChildElement( const char* name ) const
{
    //遍历,从最后一个节点开始
    for( const XMLNode* node = _lastChild; node; node = node->_prev ){
        const XMLElement* element = node->ToElementWithName( name ) );
        if( element ){
            return element;
        }
    }
    return 0;
}

const XMLElement* XMLNode* XMLNode::NextSiblingElement( const char* name ) const
{
    for( const XMLNode* node = _next; node; node = node->_next ){
        const XMLElement* element = node->ToElementWithName( name );
        if( element ){
            return element;
        }
    }
    return 0;
}


XMLNode* XMLNode::InsertEndChild( XMLNode* addThis )
{
    TIXMLASSERT( addThis );
    if( addThis->_document != _document ){
        TIXMLASSERT( false );
        return 0;
    }

    //准备插入
    InsertChildPreamble( addThis );
    //添加
    if( _lastChild ){
        TIXMLASSERT( _firstChild );
        TIXMLASSERT( _lastChild->next == 0 );
        _lastChild->next = addThis;
        addThis->_prev = _lastChild;
        _lastChild = addThis;

        addThis->_next = 0;
    }

    //(右)子节点就是(左) 子节点
    else{
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
    if( addThis->_document != _document ){
        TIXMLASSERT( false );
        return 0;
    }

    InsertChildPreamble( addThis );

    if(_firstChild ){
        TIXMLASSERT( _lastChild );
        TIXMLASSERT( _firstChild->_prev == 0 );

        _firstChlid->_prev = addThis;
        addThis->_next = _firstChild;
        _firstChild = addThis;

        addThis->_prev = 0;
    }
    else{
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
    if( addThis->_document != _document ){
        TIXMLASSERT( false );
        return 0;
    }

    TIXMLASSERT( afterThis );

    if( afterThis->_parent != this ){
        TIXMLASSERT( false );
        return 0;
    }

    if( afterThis == addThis ){
        return addThis;
    }

    if( afterThis->_next == ){
        //最后一个节点或唯一节点
        return InsertEndChild( addThis );
    }

    //插入准备
    InsertChildPreamble( addThis );
    //插入
    addThis->_prev = afterThis;
    addThis->_next = afterThis->_next;
    afterThis->_next->prev = addThis;
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
    TIXMLASSERT( node->_prev == 0 );
    TIXMLASSERT( node->_next == 0 );
    TIXMLASSERT( node->_parent == 0 );
    DeleteNode( node );
}


void XMLNode::DeleteChildren()
{
    while( _firstChild ){
        TIXMLASSERT( _lastChild );
        DeleteChild( _firstChild );
    }
    _firstChild = _lastChild = 0;
}


XMLNode* XMLNode::DeepClone( XMLDocument* target ) const
{
    XMLNode* clone = this->ShallowClone(target);
    if(!clone) return 0;
    //利用递归遍历
    for( const XMLNode* child = this->FirstChild(); child; child = child->NextSibling()){
        //将所有孩子节点克隆到clone
        XMLNode* childClone = child->DeepClone(target);
        TIXMLASSERT(childClone);
        clone->InsertEndChild(childClone);
    }
    return clone;
}
