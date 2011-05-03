// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include "AsyncSocket.h"
#include <sstream>
#include <memory>

enum HTTPMethod {
    InvalidMethod,
    GET,
    POST,
    HEAD,
    PUT,
    DELETE
};

enum HTTPVersion {
    InvalidVersion,
    HTTP_1_0,
    HTTP_1_1
};

class HTTPHeadersParser
{
public:
    enum Type { REQUEST, RESPONSE };
    
	HTTPHeadersParser(Type tp, long lim = 4*1024)
    : type(tp)
    , ready(false)
    , limit(lim)
    , count(0)
	, code(0)
    , lineNum(0)
    , curLine("")
	{
    }
	
    void write(Data & data);
	
    bool isReady()
	{
        return ready;
	}
		
    std::string method, version, action, status;
    int code;
    
    std::map<std::string, std::string> headers;
    
private:
    Type type;
    bool ready;
    long limit, count;
    int lineNum;
    
    void parseLine(const std::string& line);        
    std::stringstream buf;
    std::string curLine;
};

class HTTPHeadersWriter
{
public:
	void addHeader(const std::string & name, const std::string & value) {
        headers.insert( std::make_pair(name, value) );
    }
    
    void setResponse(HTTPVersion version, int code, const std::string& status);
    void setRequest(HTTPMethod method, const std::string& query, HTTPVersion version);
    
    static std::string methodToString(HTTPMethod method) 
    {
        switch((int)method) {
            case GET:
                return "GET";
                break;
            case POST:
                return "POST";
                break;
            case PUT:
                return "PUT";
                break;
            case DELETE:
                return "DELETE";
                break;
            case HEAD:
                return "HEAD";
                break;
        }
        
        return "UNKNOWN";
    }
    
    static std::string versionToString(HTTPVersion ver) 
    {
        switch((int)ver) {
            case HTTP_1_0:
                return "HTTP/1.0";
                break;
            case HTTP_1_1:
                return "HTTP/1.1";
                break;
        }
        
        return "HTTP/INVALID";
    }
    
    Data getData();

private:
    std::string first_line;
    std::map<std::string, std::string> headers;
};


class HTTPRequest 
: public RWSocket::Delegate
{    
public:
    struct Delegate 
    {
        virtual ~Delegate() {};
        virtual void onHeaders(const HTTPRequest&, const HTTPHeadersParser& headers) = 0;
        virtual void onCanRead(const HTTPRequest&) = 0;
        virtual void onCanWrite(const HTTPRequest&) = 0;
        virtual void onDisconnect(const HTTPRequest&) = 0;
    };
    
    HTTPRequest( Delegate* del, Socket& sock ) 
    : delegate(del)
    , sock(this, sock)
    , parser(new HTTPHeadersParser( HTTPHeadersParser::REQUEST ))
    , state(PARSING_HEADERS)
    {        
    }
    
    virtual void onDisconnect(const RWSocket&);
    virtual void onCanRead(const RWSocket&);
    virtual void onCanWrite(const RWSocket&);
    
    inline int write(Data& d) 
    {
        return sock.write(d); 
    }
    
    inline int read(Data& d) 
    {
        return sock.read(d); 
    }
    
    void reset() {
        state = PARSING_HEADERS;       
    }
    
    inline int getSock() {
        return sock.getSock();
    }
    
private:
    Delegate *delegate;
    RWSocket sock;
    Data     body;
    std::auto_ptr<HTTPHeadersParser> parser;
        
    enum { PARSING_HEADERS = 0, READING_BODY = 10, DEAD = 20 } state;
};

#endif // __HTTP_REQUEST_H__