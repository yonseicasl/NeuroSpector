#include <algorithm>
#include <vector>

#include "configs.h"
#include "utils.h"

static handler_t handler;

/* Configuration */
configs_t::configs_t(std::string config_file_) {
    config_file.open(config_file_.c_str());
    if(!config_file.is_open()) 
        handler.print_err(err_type_t::OPENFAIL, config_file_);
}

template<typename T>
T configs_t::get_line_val(std::string &line_) {
    double val;
    size_t strpos = line_.find(',') + 1;
    size_t endpos = line_.find(',', strpos);
    val = std::stod(line_.substr(strpos, endpos - strpos));
    return val;
}

void configs_t::get_line_vals(std::string &line_, size_t bias_, std::vector<unsigned> &container) {
    size_t strpos = 0;
    size_t endpos = line_.find(','); 
    for(size_t b = 0; b < bias_; b++) {
        strpos = endpos + 1;
        endpos = line_.find(',', strpos);
    }
    while (endpos < line_.length()) {  
        container.push_back(std::stoi(line_.substr(strpos, endpos - strpos)));
        strpos = endpos + 1;
        endpos = line_.find(',', strpos);
        if(endpos == std::string::npos) {
            container.push_back(std::stoi(line_.substr(strpos, line_.length() - strpos)));
        }
    }
    return;
}

/* Accelerator configuration */
accelerator_configs_t::accelerator_configs_t(std::string config_file_) 
    : configs_t(config_file_) { 
    parse();
}

accelerator_configs_t::~accelerator_configs_t() {
}

void accelerator_configs_t::parse() {
    std::string line;
    // Start parsing
    while(std::getline(config_file, line)) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Skip blank lines or comments
        if(!line.size() || (line[0] == '#')) continue;
        // Parse [SECTION:NAME]
        else if(line[0] == '[') {
            size_t strpos = line.find(':') + 1; 
            size_t endpos = line.find(']'); 
            if(line.find("ACC") != std::string::npos) 
                name = line.substr(strpos, endpos - strpos);
            else 
                handler.print_err(err_type_t::INVAILD, "[SECTION:NAME]");
            continue;
        }
        else {
            size_t strpos = line.find(',') + 1;
            size_t endpos = line.find(',', strpos);
            // PRECISION
            if(line.find("PRECISION") != std::string::npos) {
                get_line_vals(line, 1, precision); continue;
            }
            // MAC
            else if(line.find("MAC_PER_PE") != std::string::npos) {
                mac_per_pe = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("MAC_WIDTH") != std::string::npos) {
                mac_width = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("L0_DATAFLOW") != std::string::npos) {
                L0_dataflow = line.substr(strpos, endpos - strpos); 
                if(L0_dataflow.size() != 2)
                    handler.print_err(err_type_t::INVAILD, "L0_DATAFLOW parsing error");
                continue;
            }
            // L1
            else if(line.find("L1_EXISTS") != std::string::npos) {
                L1_mode = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("L1_MODE") != std::string::npos) {
                L1_mode = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("L1_SEPARATE_SIZE") != std::string::npos) {
                get_line_vals(line, 1, L1_separate_size); continue;
            }
            else if(line.find("L1_SHARED_SIZE") != std::string::npos) {
                L1_shared_size = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("L1_DATAFLOW") != std::string::npos) {
                L1_dataflow = line.substr(strpos, endpos - strpos); 
                if(L1_dataflow.size() != 2)
                    handler.print_err(err_type_t::INVAILD, "L1_DATAFLOW parsing error");
                continue;
            }
            // X, Y
            else if(line.find("ARRAY_MODE") != std::string::npos) {
                array_mode = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("ARRAY_SIZE_X") != std::string::npos) {
                array_size_x = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("ARRAY_SIZE_Y") != std::string::npos) {
                array_size_y = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("ARRAY_MAP_X") != std::string::npos) {
                array_map_x = line.substr(strpos, endpos - strpos);
                if(array_map_x.size() != 1)
                    handler.print_err(err_type_t::INVAILD, "ARRAY_UNROLL_X parsing error");
                continue;
            }
            else if(line.find("ARRAY_MAP_Y") != std::string::npos) {
                array_map_y = line.substr(strpos, endpos - strpos);
                if(array_map_y.size() != 1)
                    handler.print_err(err_type_t::INVAILD, "ARRAY_UNROLL_Y parsing error");
                continue;
            }
            // L2
            else if(line.find("L2_EXISTS") != std::string::npos) {
                L2_exists = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("L2_SHARED_SIZE") != std::string::npos) {
                L2_shared_size = get_line_val<unsigned>(line); continue;
            }
            else if(line.find("L2_DATAFLOW") != std::string::npos) {
                L2_dataflow = line.substr(strpos, endpos - strpos);
                if(L2_dataflow.size() != 2)
                    handler.print_err(err_type_t::INVAILD, "L2_DATAFLOW parsing error");
                continue;
            }
            else 
                handler.print_err(err_type_t::INVAILD, "ACC parsing");
        }
    }
    return;
}

/* Network configuration */
network_configs_t::network_configs_t(std::string config_file_) 
    : configs_t(config_file_) { 
    parse();
}

network_configs_t::~network_configs_t() {
}

void network_configs_t::parse() {
    std::string line;
    // Start parsing
    while(std::getline(config_file, line)) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Skip blank lines or comments
        if(!line.size() || (line[0] == '#')) continue;
        // Parse [SECTION:NAME]
        else if(line[0] == '[') {
            size_t strpos = line.find(':') + 1; 
            size_t endpos = line.find(']'); 
            if(line.find("NET") != std::string::npos) 
                network_name = line.substr(strpos, endpos - strpos);
            else 
                handler.print_err(err_type_t::INVAILD, "[SECTION:NAME]");
            continue;
        }
        else {
            bool is_layer = line.find("CONV") != std::string::npos 
                            || line.find("FC") != std::string::npos 
                            || line.find("PW") != std::string::npos 
                            || line.find("DW") != std::string::npos;
            if(is_layer) {
                layer_t layer;
                layer.name = line.substr(0, line.find(','));
                get_line_vals(line, 1, layer.layer_vals);
                layers.push_back(layer);
                continue;
            }
            else
                handler.print_err(err_type_t::INVAILD, "NET parsing");
        }
    }
    return;
}

/* Mapping configuration */
mapping_configs_t::mapping_configs_t(std::string config_file_) 
    : configs_t(config_file_) { 
    parse();
}

mapping_configs_t::~mapping_configs_t() {
}

void mapping_configs_t::parse() {
    std::string line;
    bool is_mapping_parsed = true;
    unsigned layer_cnts = 0;
    // Start parsing
    while(std::getline(config_file, line)) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Skip blank lines or comments
        if(!line.size() || (line[0] == '#')) continue;
        // Parse [SECTION:NAME]
        else if(line[0] == '[') {
            size_t strpos = line.find(':') + 1; 
            size_t endpos = line.find(']'); 
            if(line.find("NET") != std::string::npos) 
                network_name = line.substr(strpos, endpos - strpos);
            else if(line.find("MAP") != std::string::npos) {
                std::string layer_name = line.substr(strpos, endpos - strpos);
                if(mappings.at(layer_cnts).name.compare(layer_name)) 
                    handler.print_err(err_type_t::INVAILD, "LAYER-MAP are not matched");
                is_mapping_parsed = false;
                layer_cnts++;
            }
            else 
                handler.print_err(err_type_t::INVAILD, "[SECTION:NAME]");
            continue;
        }
        else {
            // Mapping parsing
            if(!is_mapping_parsed) {
                for(size_t row = 0; row < 5; row++) {
                    if(line[0] == '#') {
                        row--; continue;
                    }
                    get_line_vals(line, 1, mappings.at(layer_cnts - 1).map_vals);
                    std::getline(config_file, line);
                }
            }
            else {
                bool is_layer = line.find("CONV") != std::string::npos 
                                || line.find("FC") != std::string::npos 
                                || line.find("PW") != std::string::npos 
                                || line.find("DW") != std::string::npos;
                if(is_layer) {
                    mapping_t mapping;
                    mapping.name = line.substr(0, line.find(','));
                    get_line_vals(line, 1, mapping.layer_vals);
                    mappings.push_back(mapping);
                    continue;
                }
                else 
                    handler.print_err(err_type_t::INVAILD, "NET parsing");
            }
        }
    }
    return;
}
