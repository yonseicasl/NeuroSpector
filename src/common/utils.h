#ifndef __UTILS_H__
#define __UTILS_H__

#include <iostream>
#include <string>
#include <sys/stat.h> 
#include <sys/types.h> 

enum class err_type_t {
    GENERAL, INVAILD, OPENFAIL, SIZE
};

class handler_t {
public:
    handler_t();
    ~handler_t();

    void print_err(err_type_t type_, std::string msg_); 
    void print_line(unsigned num_, std::string str_);
    void make_dir(std::string name_);
};

#endif
