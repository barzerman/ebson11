#ifdef AY_JSON_H
#define AY_JSON_H

#include <stack>
#include <vector>
namespace ay {

struct JSON_string {
    const char* str;
    size_t      str_len;

    JSON_string() : str(0), str_len(0) {}
    JSON_string(const char* s) : str(s), str_len((s? strlen(s):0)) {}
    JSON_string(const char* s, size_t s_len) : str(s), str_len(s_len) {}
};

class JSON_value {
    union {
        char    buf[ sizeof(JSON_string) ];
        double  r;
        int     i4;     
        bool    b;
    } d_dta;
    uint8_t     d_type;
public:
    enum {
        VT_STRING,
        VT_NUMBER_INT,
        VT_NUMBER_REAL,
        VT_OBJECT,
        VT_ARRAY,
        VT_BOOL,

        VT_NULL,
        VT_UNDEFINED
    };
    void setInt( int i )                       { d_type=VT_NUMBER_INT; d_dta.i4 = i; }
    void setDoube( double i )                    { d_type=VT_NUMBER_REAL; d_dta.r = i; }

    void setNull() { d_type = VT_NULL; }
    void setUndefined() { d_type = VT_UNDEFINED; }
    void setBool( bool i ) { d_type = VT_BOOL; d_dta.b = i; }

    void setString( const char* s, size_t s_len ) { new (d_dta.buf)( JSON_string(s,s_len) ); }
    JSON_value(): d_type(VT_UNDEFINED) {}
};

template <typename CB>
class JSON_SAX_parser {
public:
    CB* d_cb_obj_start, 
        d_cb_obj_end, 
        d_cb_arr_start, 
        d_cb_arr_end; 
    bool d_readyForValue;
public:
    
    typedef std::vector< char > char_vec;
    typedef enum {
        ERR_OK,
        ERR_BAD_UNQUOTED, // bad un-quoted constant
        ERR_BAD_NUMBER, // bad un-quoted constant
        ERR_BAD_COMMA, // value expected after comma
        ERR_SEMICOLON, // semicolon expected 
        ERR_UNICODE, // invalid \uXXXX unicode char
        ERR_ESCAPE // bad escape sequence
    } ErrStatus;
    typedef enum {
        EVENT_OBJECT_START,
        EVENT_OBJECT_END,

        EVENT_NVPAIR_NAME, // passes in the name 

        EVENT_NVPAIR_VALUE_START, 
        EVENT_NVPAIR_VALUE_END,

        EVENT_VALUE_ATOMIC
    } Event;
    Event d_event;

    ErrStatus d_err;
    size_t d_pos; // position 
    char_vec d_curStr; 
    JSON_value d_val;

    JSON_SAX_parser() : d_err(ERR_OK) {}

    bool is_hex_digit( char c ) { return( ( c>='0' && c<='9') || (c>='A' && c<='F') || (c>='a' && c<='f') ); }
    bool push_unicode( char_vec& s, const char* s )
        { 
            if( is_hex_digit(s[0]) && is_hex_digit(s[1])&& is_hex_digit(s[2]) && is_hex_digit(s[3])) {
                s.push_back( (char)(s[1]) + ((char)(s[0]))<< 4 );
                s.push_back( (char)(s[3]) + ((char)(s[2]))<< 4 );
                return true;
            } else
                return false;
        }
    void setValue( JSON_value& val, const char* t, const char* s_len, bool quoteInFront, bool isNumber )
    {
        if( isNumber ) {
            char numbuf[ 32 ]; 
            size_t l = (s_len<32?s_len:31);
            strncpy( numbuf, s, l );
            numbuf[l]=0;
            if( strchr(numbuf,'.') ) 
                val.setDouble( atof(numbuf) );
            else 
                val.setInt( atoi(numbuf) );
            return ERR_OK;
        } else {
            if( !quoteInFront ) {
                switch( *s ) {
                case 't':
                    if( t[1] == 'r' && t[2] == 'u' && t[3] == 'e' && !t[4] ) {
                        val.setBool( true );
                        return ERR_OK;
                    }
                    break;
                case 'f':
                    if( t[1] == 'a' && t[2] == 'l' && t[3] == 's' && t[4] == 's' && !t[5] ) {
                        val.setBool( false );
                        return ERR_OK;
                    }
                    break;
                case 'n':
                    if( t[1] == 'u' && t[2] == 'l' && t[3] == 'l' && !t[4] ){
                        val.setNull();
                        return ERR_OK;
                    }
                    break;
                case 'u':
                    if( t[1] == 'n' && t[2] == 'd' && t[3] == 'e' && t[4] == 'f' && t[5] == 'i' && t[6] == 'n' && t[7] == 'e' && t[8] == 'd' &&!t[9]  ) {
                        val.setUndefined( );
                        return ERR_OK;
                    }
                    break;
                default:
                    val.setString( s, s_len );
                    return ERR_OK;
                }
            } else {
                val.setString( s, s_len );
                return ERR_OK;
            }
        }
    }
    int parse( CB& cb, const char* str, size_t s_len ) {
        const char* str_end  = str+s_len;
        const char*t= str, *t_end = str;
        bool isQuoted = false, isEscaped = false, isNumber = false;
        bool isObject = false; isArray  = false;
        
        for( ; t< str_end && *t; ++t ) {
            char tc = *t;
            if( isEscaped ) {
                char c = 0;
                switch( tc ) {
                case '"': c='"'; break;
                case '\\':  c = '\\'; break;
                case '/':  c = '/'; break;
                case 'b':  c = '\b'; break;
                case 'f':  c = '\f'; break;
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                case 'r':  c = '\r'; break;
                case 't':  c = '\t'; break;
                case 'u':  c = '\t'; 
                    if( t+3< str_end && push_unicode(d_curStr,t) ) 
                        t+= 3;
                    else 
                        return ERR_UNICODE;
                    break;
                }
                if( c ) 
                    d_curStr.push_back(c);
            } else if( isQuoted ) { // quoted not escaped 
                if( tc == '"' ) {
                    isQuoted = false;
                    setValue( d_val, &(d_curStr), d_curStr.size(), true, isNumber );
                } else {
                }
                isEscaped = false;
                if( d_readyForValue ) {
                    if( cb( *this ) )
                        return ERR_OK;
                    d_readyForValue = false;
                } else {
                    isNumber = false;
                    isEscaped = false;
                    isQuoted = false;
                }
            } else { // not quoted 
                switch( tc ) {
                case '{': 
                    cb( ( d_event = EVENT_OBJECT_START, *this) ); break;
                case '}': 
                    cb( (d_event = EVENT_NVPAIR_VALUE_END,*this) ); 
                    cb( (d_event = EVENT_OBJECT_END,*this) ); 
                    break;
                case ':':  cb( (d_event = EVENT_NVPAIR_VALUE_START,*this) ); break; break;
                case ',': 
                    cb( (d_event = EVENT_NVPAIR_VALUE_END,*this) ); 
                    cb( (d_event = EVENT_OBJECT_END,*this) ); 
                    break;
                case ' ': 
                    break;
                case '"': 
                    isQuoted = true;
                    break;
                default:
                    if( isdigit(tc) ) {
                        if( !d_curStr.size() ) 
                            isNumber= true;
                    } else {
                        if( isNumber ) 
                            return ERR_UNQUOTED;
                        else
                            d_curStr.push_back(tc);
                    }
                    d_curStr.push_back(tc);
                }
            }
        }
    }
}; 

} // ay namespace ends 
#endif // AY_JSON_H
