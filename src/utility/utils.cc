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

bool is_number(std::string& str_) {
    for(unsigned i = 0; i < str_.size(); i++) {
        if(!std::isdigit(str_[i])) {
            return false;
        }
    }
    return true;
}

bool has_txt_extension(const std::string& path_) {
    size_t last_dot = path_.find_last_of('.');
    if(last_dot == std::string::npos) {
        return false;
    }
    return path_.substr(last_dot + 1) == "txt";
}