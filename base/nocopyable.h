#pragma once


//删除拷贝构造和赋值构造
class nocopyable{
public:
    nocopyable(const nocopyable &) = delete;
    nocopyable &operator=(const nocopyable &) = delete;
protected:
    nocopyable() = default;
    ~nocopyable() = default;
};