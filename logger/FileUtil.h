#pragma once

#include<stdio.h>
#include<string>
/**
* 工具类，打开一个文件fp_,向fp_指定buffer_中写入数据，把buffer_中的数据写入文件中，在LogFile中用到
*/
class FileUtil{
public:
    explicit FileUtil(std::string &fileName);
    ~FileUtil();
    void append(const char *data, size_t len);
    void flush();
    off_t writtenBytes() const {return writtenBytres_;}
private:
    size_t write(const char *data, size_t len);

    FILE *fp_;
    char buffer_[64*1024];
    off_t writtenBytres_;//指示当前已经文件(注意不是buffer_)写入多少字节
};