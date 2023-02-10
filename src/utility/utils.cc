#include "utils.h" 

std::string& lowercase(std::string &str_) {
    transform(str_.begin(), str_.end(), str_.begin(), ::tolower);
    return str_;
}

std::string& uppercase(std::string &str_) {
    transform(str_.begin(), str_.end(), str_.begin(), ::toupper);
    return str_;
}

std::vector<std::string> split(std::string str_, char divisor_) {
    std::istringstream ss(str_);
    std::string buffer;

    std::vector<std::string> rtn;

    while(getline(ss, buffer, divisor_)) {
        rtn.push_back(buffer);
    }
    return rtn;
}

std::vector<unsigned> comma_to_vector(std::string value_) {
    std::vector<std::string> splited_value;
    std::vector<unsigned> rtn;   
    // Get buffer size configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        rtn.push_back(std::stoi(splited_value.at(i)));
    }
    splited_value.clear();
    return rtn;
}