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

/* Analyzer configuration */
analyzer_configs_t::analyzer_configs_t(std::string config_file_) 
    : configs_t(config_file_), 
    is_hw_parsed(true), is_net_parsed(true), 
    is_map_parsed(true), layer_and_map_cnt(0) { 
    parse();
}

analyzer_configs_t::~analyzer_configs_t() {
}

void analyzer_configs_t::parse() {
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
            if(line.find("ACC") != std::string::npos) {
                accelerator.name = line.substr(strpos, endpos - strpos);
                is_hw_parsed = false; is_net_parsed = true; is_map_parsed = true;
            }
            else if(line.find("NET") != std::string::npos) {
                network_name = line.substr(strpos, endpos - strpos);
                is_hw_parsed = true; is_net_parsed = false; is_map_parsed = true;
            }
            else if(line.find("MAP") != std::string::npos) {
                std::string layer_name = line.substr(strpos, endpos - strpos);
                if(layers_and_maps.at(layer_and_map_cnt).name.compare(layer_name)) 
                    handler.print_err(err_type_t::INVAILD, "LAYER-MAP are not matched");
                is_hw_parsed = true; is_net_parsed = true; is_map_parsed = false;
                layer_and_map_cnt++;
            }
            else 
                handler.print_err(err_type_t::INVAILD, "[SECTION:NAME]");
            continue;
        }
        else {
            if(!is_hw_parsed) {
                size_t strpos = line.find(',') + 1;
                size_t endpos = line.find(',', strpos);
                // MAC
                if(line.find("MAC_PER_PE") != std::string::npos) {
                    accelerator.mac_per_pe = get_line_val<unsigned>(line); continue;
                }
                else if(line.find("MAC_WIDTH") != std::string::npos) {
                    accelerator.mac_width = get_line_val<unsigned>(line); continue;
                }
                else if(line.find("MAC_STATIONARY") != std::string::npos) {
                    accelerator.mac_stationary = line.substr(strpos, endpos - strpos); 
                    if(accelerator.mac_stationary.size() != 2)
                        handler.print_err(err_type_t::INVAILD, "MAC_STATIONARY parsing error");
                    continue;
                }
                // L1
                else if(line.find("L1_SIZES") != std::string::npos) {
                    get_line_vals(line, 1, accelerator.L1_sizes); continue;
                }
                else if(line.find("L1_BYPASS") != std::string::npos) {
                    get_line_vals(line, 1, accelerator.L1_bypass); continue;
                }
                else if(line.find("L1_STATIONARY") != std::string::npos) {
                    accelerator.L1_stationary = line.substr(strpos, endpos - strpos); 
                    if(accelerator.L1_stationary.size() != 2)
                        handler.print_err(err_type_t::INVAILD, "L1_STATIONARY parsing error");
                    continue;
                }
                // X, Y
                else if(line.find("ARRAY_SIZE_X") != std::string::npos) {
                    accelerator.array_size_x = get_line_val<unsigned>(line); continue;
                }
                else if(line.find("ARRAY_SIZE_Y") != std::string::npos) {
                    accelerator.array_size_y = get_line_val<unsigned>(line); continue;
                }
                else if(line.find("ARRAY_UNROLL_X") != std::string::npos) {
                    accelerator.array_unroll_x = line.substr(strpos, endpos - strpos);
                    if(accelerator.array_unroll_x.size() != 1)
                        handler.print_err(err_type_t::INVAILD, "ARRAY_UNROLL_X parsing error");
                    continue;
                }
                else if(line.find("ARRAY_UNROLL_Y") != std::string::npos) {
                    accelerator.array_unroll_y = line.substr(strpos, endpos - strpos);
                    if(accelerator.array_unroll_y.size() != 1)
                        handler.print_err(err_type_t::INVAILD, "ARRAY_UNROLL_Y parsing error");
                    continue;
                }
                else if(line.find("ARRAY_STATIONARY") != std::string::npos) {
                    accelerator.array_stationary = line.substr(strpos, endpos - strpos); 
                    if(accelerator.array_stationary.size() != 2)
                        handler.print_err(err_type_t::INVAILD, "ARRAY_STATIONARY parsing error");
                    continue;
                }
                // L2
                else if(line.find("L2_SIZE") != std::string::npos) {
                    accelerator.L2_size = get_line_val<unsigned>(line); continue;
                }
                else if(line.find("L2_BYPASS") != std::string::npos) {
                    get_line_vals(line, 1, accelerator.L2_bypass); continue;
                }
                else if(line.find("L2_STATIONARY") != std::string::npos) {
                    accelerator.L2_stationary = line.substr(strpos, endpos - strpos);
                    if(accelerator.L2_stationary.size() != 2)
                        handler.print_err(err_type_t::INVAILD, "L2_STATIONARY parsing error");
                    continue;
                }
                // PRECISION
                else if(line.find("PRECISION") != std::string::npos) {
                    get_line_vals(line, 1, accelerator.precision); continue;
                }
                else 
                    handler.print_err(err_type_t::INVAILD, "HW parsing");
            }
            else if(!is_net_parsed) {
                bool is_layer = line.find("CONV") != std::string::npos 
                                || line.find("FC") != std::string::npos 
                                || line.find("PW") != std::string::npos 
                                || line.find("DW") != std::string::npos;
                if(is_layer) {
                    layer_and_map_t layer_and_map;
                    layer_and_map.name = line.substr(0, line.find(','));
                    get_line_vals(line, 1, layer_and_map.layer_vals);
                    layers_and_maps.push_back(layer_and_map);
                    continue;
                }
                else
                    handler.print_err(err_type_t::INVAILD, "NET parsing");
            }
            else if(!is_map_parsed) {
                for(size_t row = 0; row < MAP_TABLE_ROWS; row++) {
                    if(line[0] == '#') {
                        row--; continue;
                    }
                    get_line_vals(line, 1, layers_and_maps.at(layer_and_map_cnt - 1).map_vals);
                    std::getline(config_file, line);
                }
                is_map_parsed = true;
            }
            else 
                handler.print_err(err_type_t::INVAILD, "line parsing error!");
        }
    }
    return;
}

/* Optimizer configuration */
optimizer_configs_t::optimizer_configs_t(std::string config_file_) 
    : configs_t(config_file_) { 
}

optimizer_configs_t::~optimizer_configs_t() {
}

void optimizer_configs_t::parse() {
    return;
}
