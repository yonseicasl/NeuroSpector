#ifndef __PARSER_H__
#define __PARSER_H__

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "utils.h"

class section_config_t {
public:
    section_config_t(std::string name_);
    ~section_config_t();

    // Show all setting values (key, value) pairs in a section
    void show_setting();
    // Get total number of settings
    unsigned get_num_settings();
    // Get setting value
    std::string get_value(std::string value_, unsigned idx_); 
    // Add (key, value) pair to the latest section settings
    void add_setting(std::string key_, std::string value_);
    // Check if a setting exists
    bool exist(std::string key_);
    // Get the setting value
    template <typename T>
    bool get_setting(std::string key_, T *var_) {
        std::map<std::string, std::string>::iterator it = settings.find(lowercase(key_));
        if(it == settings.end()) return false;
        std::stringstream ss; ss.str(it->second);
        ss >> *var_; return true;
    }
    std::string get_type() {
        std::string rtn;
        std::string key = "type";
        std::map<std::string, std::string>::iterator it = settings.find(lowercase(key));
        if(it == settings.end()) return "NONE";
        rtn = it->second;
        return rtn;
    }

    // Section name
    std::string name;
    std::string type;

private:
    // Section settings
    std::map<std::string, std::string> settings;
};

class parser_t {
public:
    parser_t();
    ~parser_t();

    void argparse(int argc_, char** argv_,
                   std::map<std::string, std::string>& str_argv_);  // Parse input command 
    void cfgparse(const std::string cfg_path_);                     // Parse configuration file
    void print_help_argparser(char** argv_);                        // Print command options 
    
    std::vector<section_config_t> sections;
};

#endif
