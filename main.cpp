#include <stdio.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <set>
#include <thread>
#include "http/HttpServer.h"
#include "net/EventLoop.h"

using namespace std;
int Connection(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        cout << "Error: Failed to create socket\n";
        return 1;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cout<<"ERROR"<<endl;
        close(sockfd);
        return 1;
    }

    this_thread::sleep_for(chrono::seconds(10));
    char buf[] = "GET /index.html?id=acer HTTP/2.1\r\nHost: 127.0.0.1:8002\r\nConnection: keep-alive\r\n";//2.1解析会出错，返回错误的解析
    ssize_t n = write(sockfd,buf,sizeof(buf));
    cout<<"HTTP report sizeof : "<<sizeof(buf)<<endl;
    cout<<"has writen size: "<<n<<endl;

    char buff[1024];
    memset(buff,0, sizeof(buff));
    ssize_t m = read(sockfd,buff,sizeof(buff));
    if(m == -1){
        cout<<"error"<<endl;
    }else {
        cout<<"read size "<<m<<endl;
        cout<<buff<<endl;
    }

    close(sockfd);
    return 0;
}
int main() {
    EventLoop* p = new EventLoop();
    InetAddress localAddress(12345);
    HttpServer* httpServer = new HttpServer(p,localAddress,"LIBO");
    thread t1([&](){
        httpServer->setThreadNum(5);
        httpServer->start();
        p->loop();
    });
    this_thread::sleep_for(chrono::seconds(2));
    thread t2([](){

        while(1){
            for(int i = 0; i < 10; i++){
                cout<<"new connection to local "<<endl;
                thread t(Connection);
                t.detach();
            }
            this_thread::sleep_for(chrono::seconds(3));
        }
    });
    t2.detach();
    t1.join();
    return 0;

}