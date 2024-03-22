#include "Buffer.h"

#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
/**
 *  从socket读到缓冲区的方法是使用readv先读至buffer_，
 * buffer_空间如果不够会读入到栈上65536个字节大小的空间，然后以append的
 * 方式追加入buffer_。既考虑了避免系统调用带来开销，又不影响数据的接收(理由如下)。
 **/
/**
 ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
 使用read()将数据读到不连续的内存、使用write将不连续的内存发送出去，需要多次调用read和write，
 如果要从文件中读一片连续的数据到进程的不同区域，有两种方案，要么使用read一次将它们读到
 一个较大的缓冲区中，然后将他们分成若干部分复制到不同的区域，要么调用read若干次分批把他们读至
 不同的区域，这样，如果想将程序中不同区域的连续数据块写到文件，也必须进行类似的处理。
 频繁系统调用和拷贝开销比较大，所以Unix提供了另外两个函数readv()和writev()，它们只需要
 一次系统调用就可以实现在文件和进程的多个缓冲区之间传送数据，免除多次系统调用或复制数据的开销。
 readv叫散布读，即把若干连续的数据块读入内存分散的缓冲区中，
 writev叫聚集写，把内存中分散的若干缓冲区写到文件的连续区域中
 */
 const char Buffer::CRLF[] = "\r\n";
 ssize_t Buffer::readFd(int fd, int *saveError) {
     char buf[65536] = {0};
     struct iovec vec[2];
     const size_t writable = writableByte();
     vec[0].iov_base = beginWrite();
     vec[0].iov_len = writable;
     vec[1].iov_base = buf;
     vec[1].iov_len = sizeof(buf);
     // 如果Buffer缓冲区大小比extrabuf(64k)还小，那就Buffer和extrabuf都用上
     // 如果Buffer缓冲区大小比64k还大或等于，那么就只用Buffer。
//     const int iovcnt = (writable < sizeof(buf)) ? 2 : 1;//感觉没有必要写这个，写入本身就是从vec[0]顺序写

     const ssize_t n = ::readv(fd,vec,2);
     if(n < 0){
         *saveError = errno;
     }else if(n <= writable){
         writerIndex_ += n;
     }else {
         writerIndex_ = buffer_.size();
         append(buf,n - writable);
     }
     return n;
 }
// 注意：
// inputBuffer_.readFd表示将对端数据读到inputBuffer_中，移动writerIndex_指针
// outputBuffer_.writeFd表示将数据写入到outputBuffer_中，从readerIndex_开始，可以写readableBytes()个字节
 ssize_t Buffer::writeFd(int fd, int *saveError) {
     ssize_t n = write(fd,peek(),readableBytes());
     if(n < 0){
         *saveError = errno;
     }
     return n;
 }