#include "utils.h"

handler_t::handler_t() {
}

handler_t::~handler_t() {
}

void handler_t::print_err(err_type_t type_, std::string msg_) {
    switch(type_) {
        case err_type_t::GENERAL:
            std::cerr << "Error: " << msg_ << std::endl; 
            exit(1); break;
        case err_type_t::INVAILD:
            std::cerr << "Error: Invalid " << msg_ << std::endl; 
            exit(1); break;
        case err_type_t::OPENFAIL:
            std::cerr << "Error: Fail to open " << msg_ << std::endl; 
            exit(1); break;
        default: break;
    }
}

void handler_t::print_line(unsigned num_, std::string str_) {
    for(unsigned num = 0; num < num_; num++) {
        std::cout << str_;
    }
    std::cout << std::endl;
}
void handler_t::make_dir(std::string name_) {
    if(mkdir(name_.c_str(), 0776) == -1 && errno != EEXIST) { 
        std::cerr << "Error: Fail to create " << name_ << std::endl; 
        exit(1);
    }
}
