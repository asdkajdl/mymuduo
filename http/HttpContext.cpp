#include "../net/Buffer.h"
#include "HttpContext.h"
#include "../logger/Logging.h"

// GET /index.html?id=acer HTTP/1.1
// Host: 127.0.0.1:8002
// User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:102.0) Gecko/20100101 Firefox/102.0
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8
// Accept-Language: en-US,en;q=0.5
// Accept-Encoding: gzip, deflate, br
// Connection: keep-alive

bool HttpContext::processRequestLine(const char *begin, const char *end) {
    bool succeed = false;
    const char* start = begin;

    const char* space = std::find(start,end,' ');
    if(space != end && request_.setMethod(start,space)){
        start = space + 1;
        space = std::find(start,end,' ');
        if(space != end){
            const char* question = std::find(start,space,'?');
            if(question!=space){
                request_.setPath(start,question);
                request_.setQuery(question,space);
            }else {
                request_.setPath(start,question);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start,end-1,"HTTP/1.");
            if(succeed){
                if(*(end - 1) == '1'){
                    request_.setVersion(HttpRequest::Http11);
                }else if(*(end - 1) == '0'){
                    request_.setVersion(HttpRequest::Http10);
                }else {
                    succeed = false;
                }
            }
        }

    }
    return succeed;
}
bool HttpContext::paraseRequest(Buffer *buf, Timestamp receiveTime) {
    bool ok = true;//解析是否成功
    bool hasMore = true;//是否还有更多信息需要解析
    while(hasMore){
        if(state_ == RequestLine){
            const char* crlf = buf->findCRLF();
            if(crlf){
                ok = processRequestLine(buf->peek(),crlf);
                if(ok){
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf+2);//crlf+2而非crlf
                    state_ = Headers;
                }else {
                    hasMore = false;
                }
            }else {
                hasMore = false;
            }
        }else if(state_ == Headers){
            const char* crlf = buf->findCRLF();
            if(crlf){
                const char* colon = std::find(buf->peek(),crlf,':');
                if(colon != crlf){
                    request_.addHeader(buf->peek(),colon,crlf);
                }else {
                    // 头部结束后的空行
                    state_ = All;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);
            }else {
                hasMore = false;
            }
        }else if(state_ == Body){

        }
    }
    return ok;
}