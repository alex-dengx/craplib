// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "HTTPRequest.h"

void HTTPHeadersParser::parseLine(const std::string& line) 
{
    std::stringstream ss;
    ss << line;
    
    if(++lineNum == 1) {
        if( type == REQUEST ) {
            
            ss >> method;
            ss >> action;
            ss >> version;
            
        } else {

            ss >> version;
            ss >> code;
            ss >> status;        
        }
    } else {
        
        // Header line
        std::string key, value;        
        
        ss >> key;
        std::getline(ss, value, '\n');

		if( key.empty() || value.empty() ) // Hrissan: your server will crash without it, guy!
			return;
        // *key.rbegin() = '\0'; // remove ':'  Hrissan: WTF??? Forget this stupidness forever!
		key.resize( key.size() - 1 );

        // FIXME: this is probably slow with this substr Hrissan: HaHaHa
        headers.insert( std::make_pair(key, value.substr(1, value.length())) );
    }
}

void HTTPHeadersParser::write(Data& data)
{
    if(ready)
        return;
    
    while(!data.empty()) {
        unsigned char p = data.getByte();
        if(++count >= limit) {
            wLog("Header exceeds given limit");

            // TODO: Throw some error
            return;                    
        }
        
        if(p == '\r') {
            
            if(state == FIRST_N)
                state = SECOND_R;
            else
                state = FIRST_R;
            
        } else if(p == '\n') {
            
            if ( state == FIRST_R ) {
                // Line done
                state = FIRST_N;            
                parseLine(buf.str());
                buf.str("");
                
            } else if( state == SECOND_R ) {
                
                // HTTP headers done
                buf.str("");            
                ready = true;
                return;
            }
            
        } else {
            if(state == FIRST_N)
                state = HEADER;
            
            buf << p;
        }
    }
}

Data HTTPHeadersWriter::getData()
{
    if(first_line.empty())
        throw std::exception(); // You should set response or request first
    
    std::stringstream ss;
    ss << first_line;
    
    for(std::map<std::string, std::string>::const_iterator it=headers.begin();
            it!=headers.end(); ++it) 
    {
        ss << it->first << ": ";
        ss << it->second << "\r\n"; 
    }
    
    ss << "\r\n";
	first_line.clear();
	headers.clear();
    return Data((int)ss.str().length(), ss.str().c_str());
}

void HTTPHeadersWriter::setResponse(HTTPVersion version, int code, const std::string& status) 
{
    std::stringstream ss;
    ss << versionToString(version) << " " << code << " " << status << "\r\n";
    first_line = ss.str();
}

void HTTPHeadersWriter::setRequest(HTTPMethod method, const std::string& query, HTTPVersion version)
{
    std::stringstream ss;
    ss << methodToString(method) << " " << query << " " << versionToString(version) << "\r\n";
    first_line = ss.str();
}


void HTTPRequest::onDisconnect(const RWSocket& s) 
{
    wLog("Underlying sock disconnected in HTTPRequest");
    delegate->onDisconnect(*this);
}

void HTTPRequest::onCanRead(const RWSocket& s) 
{
    wLog("[0x%04X :: %d] HTTPRequest::onCanRead", this, getSock());
    
    if(state != READING_BODY) {
        
        if(body.empty()) {
            sock.read(body);
        }
        
        parser->write(body);
        if (parser->isReady()) {
            
            wLog("Method: %s", parser->method.c_str());
            wLog("Action: %s", parser->action.c_str());
            
            state = READING_BODY;
            delegate->onHeaders(*this, *parser);
            
            // Prepare for the next request
            parser = std::auto_ptr<HTTPHeadersParser>( new HTTPHeadersParser( HTTPHeadersParser::REQUEST ) );
        }
        
    } else if(state == READING_BODY) {
        
        delegate->onCanRead(*this);
    }
}

void HTTPRequest::onCanWrite(const RWSocket& s) 
{
    wLog("[0x%04X :: %d] HTTPRequest::onCanWrite", this, getSock());
    
    if( state == READING_BODY ) {
        delegate->onCanWrite(*this);
    }
}
