#ifndef AY_JSON_H
#define AY_JSON_H

#include <stack>
#include <vector>
#include <locale>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

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
public:
    uint8_t     d_type;
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
    void setDouble( double i )                    { d_type=VT_NUMBER_REAL; d_dta.r = i; }

    void setNull() { d_type = VT_NULL; }
    void setUndefined() { d_type = VT_UNDEFINED; }
    void setBool( bool i ) { d_type = VT_BOOL; d_dta.b = i; }

    void setString( const char* s, size_t s_len ) {
        d_type = VT_STRING;
        new (d_dta.buf) JSON_string(s,s_len);
    }

    // TODO: slow, for debugging only
    std::string getString() {
        JSON_string& s = (JSON_string&) d_dta.buf;
        return std::string(s.str, s.str_len);
    }

    JSON_value(): d_type(VT_UNDEFINED) {}
};

class JSON_SAX_parser {
public:
    typedef std::vector< char > char_vec;
    typedef enum {
        ERR_OK,
        ERR_TERMINATE,
        ERR_BAD_UNQUOTED, // bad un-quoted constant
        ERR_BAD_NUMBER, // bad un-quoted constant
        ERR_BAD_COMMA, // value expected after comma
        ERR_SEMICOLON, // semicolon expected
        ERR_UNICODE, // invalid \uXXXX unicode char
        ERR_ESCAPE // bad escape sequence
    } ErrStatus;
    typedef enum {
        EVENT_NULL,
        EVENT_OBJECT_START,
        EVENT_OBJECT_END,

        EVENT_ARRAY_START,
        EVENT_ARRAY_END,

        EVENT_NVPAIR_NAME,

        EVENT_NVPAIR_VALUE_START,
        EVENT_NVPAIR_VALUE_END,

        EVENT_VALUE_ATOMIC
    } Event;
    Event d_event;

    ErrStatus d_err;
    size_t d_pos; // position
    char_vec d_curStr;
    JSON_value d_val;

    JSON_SAX_parser() : d_event(EVENT_NULL), d_err(ERR_OK) {}

    bool is_hex_digit( char c ) { return( ( c>='0' && c<='9') || (c>='A' && c<='F') || (c>='a' && c<='f') ); }
    bool push_unicode( char_vec& v, const char* s )
        {
            if( is_hex_digit(s[0]) && is_hex_digit(s[1])&& is_hex_digit(s[2]) && is_hex_digit(s[3])) {
                v.push_back( (char)(s[1]) + ((char)(s[0]))<< 4 );
                v.push_back( (char)(s[3]) + ((char)(s[2]))<< 4 );
                return true;
            } else
                return false;
        }
    ErrStatus setValue( JSON_value& val, const char* t, size_t s_len, bool quoteInFront, bool isNumber )
    {
        if( isNumber ) {
            char numbuf[ 32 ];
            size_t l = (s_len<32?s_len:31);
            strncpy( numbuf, t, l );
            numbuf[l]=0;
            if( strchr(numbuf,'.') )
                val.setDouble( atof(numbuf) );
            else
                val.setInt( atoi(numbuf) );
            return ERR_OK;
        } else {
            if( !quoteInFront ) {
                switch( *t ) {
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
                    val.setString( t, s_len );
                    return ERR_OK;
                }
            } else {
                val.setString( t, s_len );
                return ERR_OK;
            }
        }
    }

    template <typename CB>
    int parse( CB& cb, const char* str, size_t s_len ) {
        const char* str_end  = str+s_len;
        const char*t= str, *t_end = str;
        bool isQuoted = false, isEscaped = false, isNumber = false;
        bool isObject = false, isArray  = false, lastNotSpace = false;

        for( ; t< str_end && *t; ++t ) {
            char tc = *t;
            if( isEscaped ) {
                char c;
                switch( tc ) {
                case '"': c='"'; break;
                case '\\':  c = '\\'; break;
                case '/':  c = '/'; break;
                case 'b':  c = '\b'; break;
                case 'f':  c = '\f'; break;
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                case 't':  c = '\t'; break;
                case 'u':  c = '\t';
                    if( t+3< str_end && push_unicode(d_curStr,t) )
                        t+= 3;
                    else
                        return ERR_UNICODE;
                    break;
                default:
                    return ERR_ESCAPE;
                }
                isEscaped = false;
                d_curStr.push_back(c);
            } else if( isQuoted ) { // quoted not escaped
                switch( tc ) {
                case '"':
                    isQuoted = false;
                    setValue( d_val, &d_curStr[0], d_curStr.size(), true, isNumber );
                    cb( (d_event = EVENT_VALUE_ATOMIC,*this) );
                    break;
                case '\\':
                    isEscaped = true;
                    break;
                default:
                    d_curStr.push_back(tc);
                }
            } else { // not quoted
                switch( tc ) {
                case '[':
                    if( cb( ( d_event = EVENT_ARRAY_START, *this) ) )
                        return ERR_TERMINATE;
                    break;
                case ']':
                    if( cb( (d_event = EVENT_ARRAY_END,*this) ))
                        return ERR_TERMINATE;
                    break;
                case '{':
                    if( cb( ( d_event = EVENT_OBJECT_START, *this) ) )
                        return ERR_TERMINATE;
                    break;
                case '}':
                    if( cb( (d_event = EVENT_NVPAIR_VALUE_END,*this) ) || cb( (d_event = EVENT_OBJECT_END,*this)) )
                        return ERR_TERMINATE;

                    break;
                case ':':  cb( (d_event = EVENT_NVPAIR_VALUE_START,*this) ); break; break;
                case ',':
                    if( cb( (d_event = EVENT_VALUE_ATOMIC,*this)) || cb( (d_event = EVENT_NVPAIR_VALUE_END,*this) ) || cb( (d_event = EVENT_OBJECT_END,*this) ))
                        return ERR_TERMINATE;
                    break;
                case ' ':
                    if( lastNotSpace ) {
                        setValue( d_val, &d_curStr[0], d_curStr.size(), true, isNumber );
                        if( cb( (d_event = EVENT_VALUE_ATOMIC,*this) ) )
                            return ERR_TERMINATE;
                        lastNotSpace = false;
                    }
                    break;
                case '"':
                    isQuoted = true;
                    isNumber = false;
                    break;
                default:
                    if( std::isdigit(tc) ) {
                        if( !d_curStr.size() )
                            isNumber= true;
                    } else {
                        if( isNumber )
                            return ERR_BAD_UNQUOTED;
                        else
                            d_curStr.push_back(tc);
                    }
                    d_curStr.push_back(tc);
                    if( !lastNotSpace )
                        lastNotSpace = true;
                }
            }
        }
        return ERR_OK;
    }
};

} // ay namespace ends
#endif // AY_JSON_H
