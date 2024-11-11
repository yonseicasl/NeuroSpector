#include "parser.h"

// Configuration section
section_config_t::section_config_t(std::string name_):
    name(name_) {
}

section_config_t::~section_config_t() {
    settings.clear();
}

// Show all setting values (key, value) pairs
void section_config_t::show_setting() {
    for(auto it = settings.begin(); it != settings.end(); it++) {
        std::cerr << it->first << "=" << it->second << std::endl;
    }
}
// Get total number of settings
unsigned section_config_t::get_num_settings() {
    return settings.size();
}
// Get setting value with settings order
std::string section_config_t::get_value(unsigned idx_) {
    assert(idx_ < settings_order.size());
    return settings_order.at(idx_);
}
// Get setting value with key value
std::string section_config_t::get_value(std::string key_) {
    // Find position of key
    auto it = settings.find(key_);
    if(it != settings.end()) {
        return it->second;
    }
    else {
        std::cerr << "[Error] key(" << key_ << ") does not exist" << std::endl;
        return "False";
    }
}
// Get setting value
std::string section_config_t::get_value(std::string value_, unsigned idx_) {
    assert(idx_ < settings.size());
    unsigned cnt = 0;
    for(auto it = settings.begin(); it != settings.end(); it++) {
        if(cnt == idx_) { 
            if(value_ == "key") { return it->first; }
            else if(value_ == "value") { return it->second; }
            else { std::cerr << "Error" << std::endl; }
            break; 
        }
        cnt++;
        
    }
    return "False";
}
// Add (key, value) pair to the section settings
void section_config_t::add_setting(std::string key_, std::string value_) {
    settings.insert(std::pair<std::string,std::string>(lowercase(key_), lowercase(value_)));
    settings_order.push_back(lowercase(key_));
}
// Check if a setting exists.
bool section_config_t::exist(std::string key_) {
    return settings.find(lowercase(key_)) != settings.end();
} 

// Parser for config files or input arguments
parser_t::parser_t() { }
parser_t::~parser_t() { 
    sections.clear();
}

void parser_t::argparse(int argc_, char** argv_,
                        std::map<std::string, std::string>& str_argv_) {
    for(int i = 0; i < argc_; i++) {
        std::string str(argv_[i]);
        std::string delimiter = "=";
        if(str.find("--help") != std::string::npos) {
            print_help_argparser(argv_);
        }
        else if(str.find("--scheduling_table") != std::string::npos
             || str.find("-s") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"scheduling_table",str});
        }
        else if(str.find("--run_type") != std::string::npos 
             || str.find("-r") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"run_type",lowercase(str)});
        }
        else if(str.find("--accelerator") != std::string::npos
             || str.find("-a") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"accelerator",str});
        }
        else if(str.find("--network") != std::string::npos
             || str.find("-n") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"network",str});
        }
        else if(str.find("--dataflow") != std::string::npos
             || str.find("-d") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"dataflow",lowercase(str)});
        }
        else if(str.find("--optimizer") != std::string::npos
             || str.find("-o") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"optimizer",lowercase(str)});
        }
        else if(str.find("--layer") != std::string::npos
             || str.find("-l") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            // Check the targeted layer is valid 
            for(char const &c : str) {
                if(std::isdigit(c) == 0) { 
                    std::cerr << "Error: invalid target layer; " << str << std::endl;
                    exit(0);
                }
            }
            str_argv_.insert({"layer",lowercase(str)});
        }
        else if(str.find("--batch_size") != std::string::npos
             || str.find("--batch") != std::string::npos
             || str.find("-b") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            // Check the targeted layer is valid 
            for(char const &c : str) {
                if(std::isdigit(c) == 0) { 
                    std::cerr << "Error: invalid batch size; " << str << std::endl;
                    exit(0);
                }
            }
            str_argv_.insert({"batch_size",lowercase(str)});
        }
        else if(str.find("--thread") != std::string::npos
             || str.find("-t") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"thread",lowercase(str)});
        }
        else if(str.find("--multi_chip_partitioning") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"mc_partitioning",lowercase(str)});
        }
        else if(str.find("--cross_layer_optimization") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"cl_optimization",lowercase(str)});
        }
        else if(str.find("--metric") != std::string::npos
             || str.find("-m") != std::string::npos) {
            str = str.erase(0, str.find(delimiter) + delimiter.length());
            str_argv_.insert({"metric",lowercase(str)});
        }
    }
}

void parser_t::cfgparse(const std::string cfg_path_) {
    std::fstream file_stream;
    file_stream.open(cfg_path_.c_str(), std::fstream::in);
    if(!file_stream.is_open()) {
        std::cerr << "Error: failed to open " << cfg_path_ << std::endl;
        exit(1);
    }

    std::string line;
    while(getline(file_stream, line)) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Erase carriage return
        line.erase(remove(line.begin(), line.end(), '\r'), line.end()); 
        // Skip blank lines or comments
        if(!line.size() || (line[0] == '#')) continue;
        // Beginning of [component]
        if(line[0] == '[') {
            std::string section_name = line.substr(1, line.size()-2);
            sections.push_back(section_config_t(section_name));
        }
        else {
            size_t eq = line.find('=');
            if(eq == std::string::npos) {
                std::cerr << "Error: invalid config" << std::endl << line << std::endl;
                exit(1);
            }
            // Save (key, value) pair in the latest section setting
            std::string key   = line.substr(0, eq);
            std::string value = line.substr(eq+1, line.size()-1);
            sections[sections.size()-1].add_setting(key, value);
        }
    }
}

void parser_t::txtparse(const std::string cfg_path_) {
    std::fstream file_stream;
    file_stream.open(cfg_path_.c_str(), std::fstream::in);
    if(!file_stream.is_open()) {
        std::cerr << "Error: failed to open " << cfg_path_ << std::endl;
        exit(1);
    }

    bool sig_pass = true;
    bool tab_pass = false;
    std::string line;
    while(getline(file_stream, line)) {
        // Skip all lines that are not related with scheduling table
        if(line[0] == '[') {
            std::string section_name = line.substr(1, line.size()-2);
            // ! Only the section name `scheduling table` is allowed'
            if(section_name.compare("Scheduling Table") == 0) {
                sections.push_back(section_config_t("SCHEDULING_TABLE"));
                sig_pass = false;
            }
            if(section_name.compare("Analyze Results") == 0) {
                sig_pass = true;
            }
        }
        if(!sig_pass) {
            // Get layer info
            if(line[0] == 'L') {
                sections.push_back(section_config_t("Layer"));
                for(int i = 0; i < int(line.length()); i++) {
                    if(line[i] == ' ') {
                        line[i] = '_';
                        break;
                    }
                }
                // Convert : to = 
                for(int i = 0; i < int(line.length()); i++) {
                    if(line[i] == ':') {
                        line[i] = '=';
                    }
                }
                // Erase all spaces
                line.erase(remove(line.begin(), line.end(), ' '), line.end());
                // Erase carriage return
                line.erase(remove(line.begin(), line.end(), '\r'), line.end()); 
                size_t eq = line.find('=');
                if(eq == std::string::npos) {
                    std::cerr << "Error: invalid config" << std::endl << line << std::endl;
                    exit(1);
                }
                // Save (key, value) pair in the latest section setting
                std::string key   = line.substr(0, eq);
                key = lowercase(key);
                std::string value = line.substr(eq+1, line.size()-1);
                sections[sections.size()-1].add_setting(key, value);
            }
            // Get table info
            else if(line[0] == '|') {
                // Convert flags to collect only table information
                if(!line.size() || (line[1] == '-')) {
                    tab_pass = !tab_pass;
                }
                // Skip blank lines or comments
                if(!line.size() || (line[0] == '#') || (line[1] == '-')) continue;
                if(tab_pass) continue;
                // Convert scheduling table
                unsigned cnt = 0;
                for(int i = 0; i < int(line.length()); i++) {
                    if(line[i] == '|' && (cnt == 0 || cnt == 10)) {
                    line[i] = ' ';
                    cnt++; 
                    }
                    else if(line[i] == '|' && cnt == 1) {
                    line[i] = '=';
                    cnt++; 
                    }
                    else if(line[i] == '|' && cnt > 1) {
                    line[i] = ',';
                    cnt++; 
                    }
                }
                // Erase all spaces
                line.erase(remove(line.begin(), line.end(), ' '), line.end());
                // Erase carriage return
                line.erase(remove(line.begin(), line.end(), '\r'), line.end()); 
                size_t eq = line.find('=');
                if(eq == std::string::npos) {
                    std::cerr << "Error: invalid config" << std::endl << line << std::endl;
                    exit(1);
                }
                // Save (key, value) pair in the latest section setting
                std::string key   = line.substr(0, eq);
                // ! Temp code; line.substr(eq+1, ed) didn't work
                std::string tmp   = line.substr(eq+1, line.size()-1);
                size_t ed = tmp.find_last_of(',');
                // ! ----
                std::string value = tmp.substr(0, ed);
                sections[0].add_setting(key, value);
            }
        }
    }
}

void parser_t::print_help_argparser(char** argv_) {
    std::cout << "Usage: " << argv_[0]  
              << "--run_type= default is optimizer\n"
              << "--accelerator=<DNN accelerator input>\n"
              << "--dataflow=<fixed or flexible>"
              << "--network=<DNN workload input>"
              << "--optimizer=<bottom-up or brute-force>"
              << "--metric=<target metric; energy or cycle>"
              << "--layer=<target layer; layer #>"
              << std::endl; 
    exit(1);
}
