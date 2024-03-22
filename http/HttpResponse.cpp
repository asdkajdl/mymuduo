#include "HttpResponse.h"
#include "../net/Buffer.h"

#include <stdio.h>
#include <string.h>
#include <iostream>

/*典型的响应消息：
 *   HTTP/1.1 200 OK
 *   Date:Mon,31Dec200104:25:57GMT
 *   Server:Apache/1.3.14(Unix)
 *   Content-type:text/html
 *   Last-modified:Tue,17Apr200106:46:28GMT
 *   Etag:"a030f020ac7c01:1e9f"
 *   Content-length:39725426
 *   Content-range:bytes554554-40279979/40279980
*/

void HttpResponse::appendToBuffer(Buffer *output) const {
    char buf[32];
    snprintf(buf,sizeof(buf),"HTTP/1.1 %d",statusCode_);
    output->append(buf);
    output->append(statusMsg_);
    output->append("\r\n");

    if(closeConnection_){
        output->append("\Connection: close\r\n");
    }else {
//        关于%zd是强制转化为整形的格式输出符，对应的是size-t类型的(c99)中规定
        snprintf(buf,sizeof(buf),"Content-Length: %zd\r\n",body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }
    for(const auto& header:headers_){
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }
    output->append("\r\n");
    output->append(body_);
}