#include "configs.h"
// TODO: if -> switch, line overflow assertion in get functions

static handler_t handler;
static unsigned D_SIZE = static_cast<unsigned>(parameter_t::SIZE);
static unsigned U_SIZE = static_cast<unsigned>(component_t::SIZE);

/* Configuration */
configs_t::configs_t(const std::string& cfg_path_) {
    config_file.open(cfg_path_.c_str());
    if(!config_file.is_open()) 
        handler.print_err(err_type_t::OPENFAIL, cfg_path_);
}

unsigned configs_t::get_line_uint(const std::string &line_, const unsigned bias_) {
    size_t strpos = 0;
    size_t endpos = line_.find(',');
    unsigned rtn;
    for(size_t b = 0; b < bias_; b++) {
        strpos = endpos + 1;
        endpos = line_.find(',', strpos);
    }
    if(endpos == std::string::npos)
        rtn = std::stoi(line_.substr(strpos, line_.length() - strpos));
    else 
        rtn = std::stoi(line_.substr(strpos, endpos - strpos));
    if(rtn < 0)
        handler.print_err(err_type_t::INVAILD, "the value must be positive");
    return rtn;
}

std::string configs_t::get_line_string(const std::string &line_, const unsigned bias_) {
    size_t strpos = 0;
    size_t endpos = line_.find(',');
    for(size_t b = 0; b < bias_; b++) {
        strpos = endpos + 1;
        endpos = line_.find(',', strpos);
    }
    if(endpos == std::string::npos)
        return line_.substr(strpos, line_.length() - strpos - 1);
    else 
        return line_.substr(strpos, endpos - strpos);
}

void configs_t::get_line_vals(const std::string &line_, const unsigned bias_, const unsigned num_, std::vector<unsigned>& dst_) {
    size_t strpos = 0;
    size_t endpos = line_.find(',');
    for(size_t b = 0; b < bias_; b++) {
        strpos = endpos + 1;
        endpos = line_.find(',', strpos);
    }
    for(size_t n = 0; n < num_; n++) {
        std::string src;
        if(endpos == std::string::npos)
            src = line_.substr(strpos, line_.length() - strpos - 1);
        else 
            src = line_.substr(strpos, endpos - strpos);
        if(std::stoi(src) < 0)
            handler.print_err(err_type_t::INVAILD, "the value must be positive");
        dst_.push_back(std::stoi(src));
        strpos = endpos + 1;
        endpos = line_.find(',', strpos);
    }
    return;
}

/* Accelerator configuration */
acc_cfg_t::acc_cfg_t(const std::string& cfg_path_) 
    : configs_t(cfg_path_), 
      name("NO NAME"), 
      precision(precision_t::FP32), 
      mac_dataflow(dataflow_t::SIZE), 
      macs_per_pe(1), 
      mac_width(1), 
      l1_input_bypass(false), 
      l1_filter_bypass(false), 
      l1_output_bypass(false),
      l1_input_size(0), 
      l1_filter_size(0), 
      l1_output_size(0), 
      l1_shared_size(0), 
      l1_type(buffer_type_t::SIZE), 
      l1_dataflow(dataflow_t::SIZE), 
      s1_noc_exists(false), 
      s1_size_x(1), 
      s1_size_y(1), 
      l2_input_bypass(false), 
      l2_filter_bypass(false), 
      l2_output_bypass(false),
      l2_input_size(0), 
      l2_filter_size(0), 
      l2_output_size(0), 
      l2_shared_size(0), 
      l2_type(buffer_type_t::SIZE), 
      l2_dataflow(dataflow_t::SIZE), 
      s2_size(1) { 
    parse();
}

acc_cfg_t::~acc_cfg_t() {

}

void acc_cfg_t::parse() {
    std::string line;
    // Start parsing
    while(std::getline(config_file, line)) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Skip blank lines or comments
        if(!line.size() || line[0] == '#' || line[0] == ',') continue;
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
            // MAC [T]
            if(line.find("PRECISION") != std::string::npos) {
                std::string precision_str = get_line_string(line, 1); 
                if(precision_str.compare("FP8") == 0) precision = precision_t::FP8;
                else if(precision_str.compare("FP16") == 0) precision = precision_t::FP16;
                else if(precision_str.compare("FP32") == 0) precision = precision_t::FP32;
                else if(precision_str.compare("INT8") == 0) precision = precision_t::INT8;
                else if(precision_str.compare("INT16") == 0) precision = precision_t::INT16;
                else if(precision_str.compare("INT32") == 0) precision = precision_t::INT32;
                else handler.print_err(err_type_t::INVAILD, "PRECISION");
                continue;
            }
            else if(line.find("MAC_DATAFLOW") != std::string::npos) {
                std::string dataflow_str = get_line_string(line, 1); 
                if(dataflow_str.compare("IS") == 0) mac_dataflow = dataflow_t::IS;
                else if(dataflow_str.compare("WS") == 0) mac_dataflow = dataflow_t::WS;
                else if(dataflow_str.compare("NONE") == 0) mac_dataflow = dataflow_t::NONE;
                else handler.print_err(err_type_t::INVAILD, "MAC_DATAFLOW");
                continue;
            }
            // S0 [S]
            else if(line.find("MACS_PER_PE") != std::string::npos) {
                macs_per_pe = get_line_uint(line, 1); continue;
            }
            else if(line.find("MAC_WIDTH") != std::string::npos) {
                mac_width = get_line_uint(line, 1); continue;
            }
            // L1 [T]
            else if(line.find("L1_DATAFLOW") != std::string::npos) {
                std::string dataflow_str = get_line_string(line, 1); 
                if(dataflow_str.compare("IS") == 0) l1_dataflow = dataflow_t::IS;
                else if(dataflow_str.compare("WS") == 0) l1_dataflow = dataflow_t::WS;
                else if(dataflow_str.compare("OS") == 0) l1_dataflow = dataflow_t::OS;
                else handler.print_err(err_type_t::INVAILD, "L1_DATAFLOW");
                continue;
            }
            else if(line.find("L1_TYPE") != std::string::npos) {
                if(get_line_string(line, 1).compare("NONE") == 0) 
                    l1_type = buffer_type_t::NONE;
                else if(get_line_string(line, 1).compare("SEPARATED") == 0) {
                    l1_type = buffer_type_t::SEPARATED;
                    l1_input_size = get_line_uint(line, 2);
                    l1_filter_size = get_line_uint(line, 3);
                    l1_output_size = get_line_uint(line, 4);
                    if(l1_input_size == 0) {
                        l1_input_bypass = true;
                        if(l1_dataflow == dataflow_t::IS)
                            handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW");
                    }
                    if(l1_filter_size == 0) {
                        l1_filter_bypass = true;
                        if(l1_dataflow == dataflow_t::WS)
                            handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW");
                    } 
                    if(l1_output_size == 0) {
                        l1_output_bypass = true;
                        if(l1_dataflow == dataflow_t::OS)
                            handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW");
                    }
                    if(l1_input_bypass && l1_filter_bypass && l1_output_bypass)
                        handler.print_err(err_type_t::INVAILD, "L1 SEPARATED SIZE");
                }
                else if(get_line_string(line, 1).compare("SHARED") == 0) {
                    l1_type = buffer_type_t::SHARED;
                    l1_shared_size = get_line_uint(line, 2);
                }
                else if(get_line_string(line, 1).compare("SHARED_IF") == 0) {
                    l1_type = buffer_type_t::SHARED;
                    l1_shared_size = get_line_uint(line, 2);
                    l1_output_size = get_line_uint(line, 3);
                    if(l1_output_size == 0) {
                        l1_output_bypass = true;
                        if(l1_dataflow == dataflow_t::OS)
                            handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW");
                    }
                }
                else if(get_line_string(line, 1).compare("SHARED_FO") == 0) {
                    l1_type = buffer_type_t::SHARED;
                    l1_shared_size = get_line_uint(line, 2);
                    l1_input_size = get_line_uint(line, 3);
                    if(l1_input_size == 0) {
                        l1_input_bypass = true;
                        if(l1_dataflow == dataflow_t::IS)
                            handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW");
                    }
                }
                else if(get_line_string(line, 1).compare("SHARED_OI") == 0) {
                    l1_type = buffer_type_t::SHARED;
                    l1_shared_size = get_line_uint(line, 2);
                    l1_filter_size = get_line_uint(line, 3);
                    if(l1_filter_size == 0) {
                        l1_filter_bypass = true;
                        if(l1_dataflow == dataflow_t::WS)
                            handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW");
                    } 
                }
                else handler.print_err(err_type_t::INVAILD, "L1 TYPE");
            }
            // S1_X & S1_Y [S]
            else if(line.find("S1_NOC_EXISTS") != std::string::npos) {
                unsigned exists_u = get_line_uint(line, 1);
                if(exists_u == 1) s1_noc_exists = true;
                else if(exists_u == 0) s1_noc_exists = false;
                else handler.print_err(err_type_t::INVAILD, "L1_NOC_EXISTS");
                continue;
            }
            else if(line.find("S1_X") != std::string::npos) {
                s1_size_x = get_line_uint(line, 1); continue;
            }
            else if(line.find("S1_Y") != std::string::npos) {
                s1_size_y = get_line_uint(line, 1); continue;
            }
            // L2 [T]
            else if(line.find("L2_DATAFLOW") != std::string::npos) {
                std::string dataflow_str = get_line_string(line, 1); 
                if(dataflow_str.compare("IS") == 0) l2_dataflow = dataflow_t::IS;
                else if(dataflow_str.compare("WS") == 0) l2_dataflow = dataflow_t::WS;
                else if(dataflow_str.compare("OS") == 0) l2_dataflow = dataflow_t::OS;
                else handler.print_err(err_type_t::INVAILD, "L2_DATAFLOW");
                continue;
            }
            else if(line.find("L2_TYPE") != std::string::npos) {
                if(get_line_string(line, 1).compare("NONE") == 0) 
                    l2_type = buffer_type_t::NONE;
                else if(get_line_string(line, 1).compare("SEPARATED") == 0) {
                    l2_type = buffer_type_t::SEPARATED;
                    l2_input_size = get_line_uint(line, 2);
                    l2_filter_size = get_line_uint(line, 3);
                    l2_output_size = get_line_uint(line, 4);
                    if(l2_input_size == 0) {
                        l2_input_bypass = true;
                        if(l2_dataflow == dataflow_t::IS)
                            handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW");
                    }
                    if(l2_filter_size == 0) {
                        l2_filter_bypass = true;
                        if(l2_dataflow == dataflow_t::WS)
                            handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW");
                    } 
                    if(l2_output_size == 0) {
                        l2_output_bypass = true;
                        if(l2_dataflow == dataflow_t::OS)
                            handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW");
                    }
                    if(l2_input_bypass && l2_filter_bypass && l2_output_bypass)
                        handler.print_err(err_type_t::INVAILD, "L2 SEPARATED SIZE");
                }
                else if(get_line_string(line, 1).compare("SHARED") == 0) {
                    l2_type = buffer_type_t::SHARED;
                    l2_shared_size = get_line_uint(line, 2);
                }
                else if(get_line_string(line, 1).compare("SHARED_IF") == 0) {
                    l2_type = buffer_type_t::SHARED_OI;
                    l2_shared_size = get_line_uint(line, 2);
                    l2_output_size = get_line_uint(line, 3);
                    if(l2_output_size == 0) {
                        l2_output_bypass = true;
                        if(l2_dataflow == dataflow_t::OS)
                            handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW");
                    }
                }
                else if(get_line_string(line, 1).compare("SHARED_FO") == 0) {
                    l2_type = buffer_type_t::SHARED_OI;
                    l2_shared_size = get_line_uint(line, 2);
                    l2_input_size = get_line_uint(line, 3);
                    if(l2_input_size == 0) {
                        l2_input_bypass = true;
                        if(l2_dataflow == dataflow_t::IS)
                            handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW");
                    }
                }
                else if(get_line_string(line, 1).compare("SHARED_OI") == 0) {
                    l2_type = buffer_type_t::SHARED_OI;
                    l2_shared_size = get_line_uint(line, 2);
                    l2_filter_size = get_line_uint(line, 3);
                    if(l2_filter_size == 0) {
                        l2_filter_bypass = true;
                        if(l2_dataflow == dataflow_t::WS)
                            handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW");
                    } 
                }
                else handler.print_err(err_type_t::INVAILD, "L2 TYPE");
            }
            // S2 [S]
            else if(line.find("S2") != std::string::npos) {
                s2_size = get_line_uint(line, 1); continue;
            }
            else 
                handler.print_err(err_type_t::INVAILD, "ACC parsing");
        }
    }
    return;
}

/* Mapping table configuration */
map_cfg_t::map_cfg_t(const std::string& cfg_path_) 
    : configs_t(cfg_path_), network_name("NO NAME") { 
    parse();
}

map_cfg_t::~map_cfg_t() {

}

void map_cfg_t::parse() {
    std::string line;
    size_t strpos = 0; 
    size_t endpos = 0; 
    size_t layer_cnts = 0;
    // Parse [NET:NETWORK_NAME]
    while(std::getline(config_file, line)) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Skip blank lines or comments
        if(!line.size() || line[0] == '#' || line[0] == ',') continue;
        // Parse layer parameter values
        else if(line[0] == '[') {
            if(line.find("NET") != std::string::npos) {
                strpos = line.find(':') + 1; 
                endpos = line.find(']'); 
                network_name = line.substr(strpos, endpos - strpos);
                std::getline(config_file, line);
                while(line[0] != '[') {
                    // Erase all spaces
                    line.erase(remove(line.begin(), line.end(), ' '), line.end());
                    // Skip blank lines or comments
                    if(!line.size() || line[0] == '#' || line[0] == ',') 
                        std::getline(config_file, line);
                    else {
                        bool is_layer = line.find("CONV") != std::string::npos 
                                        || line.find("FC") != std::string::npos 
                                        || line.find("PW") != std::string::npos
                                        || line.find("GROUP") != std::string::npos
                                        || line.find("DEPTH") != std::string::npos;
                        if(!is_layer)
                            handler.print_err(err_type_t::INVAILD, "NET parsing");
                        // Parse layer values
                        mapping_table_t mapping;
                        mapping.is_grouped = line.find("GROUP") != std::string::npos
                                          || line.find("DEPTH") != std::string::npos;
                        mapping.stride = get_line_uint(line, D_SIZE + 1);
                        mapping.name = get_line_string(line, 0);
                        get_line_vals(line, 1, D_SIZE, mapping.values);
                        mappings.push_back(mapping);
                        layer_cnts++;
                        std::getline(config_file, line);
                    }
                }
                break;
            }
            else 
                handler.print_err(err_type_t::INVAILD, "Cannot find [NET]'");
        }
    }
    // Parse [MAP:LAYER_NAME]s 
    for(size_t idx = 0; idx < layer_cnts; idx++) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Skip blank lines or comments
        if(!line.size() || line[0] == '#' || line[0] == ',') continue;
        // Parse mapping table degrees 
        else if(line.find("MAP") != std::string::npos) {
            strpos = line.find(':') + 1; 
            endpos = line.find(']'); 
            std::string layer_name = line.substr(strpos, endpos - strpos);
            // Check LAYER-MAP pair
            if(mappings.at(idx).name.compare(layer_name)) 
                handler.print_err(err_type_t::INVAILD, "LAYER-MAP are not matched");
            std::getline(config_file, line);
            // Start parsing
            bool is_eof = false;
            for(unsigned row = 0; row < U_SIZE - 1; row++) {
                // Erase all spaces
                line.erase(remove(line.begin(), line.end(), ' '), line.end());
                // Skip blank lines or comments
                if(!line.size() || line[0] == '#' || line[0] == ',')  {
                    std::getline(config_file, line); row--;
                }
                else {
                    // row == 0: MAC level degrees are all 1
                    if(row == 1 && line.find("S0") != std::string::npos) {
                        get_line_vals(line, 1, D_SIZE, mappings.at(idx).degrees);
                        if(!std::getline(config_file, line)) is_eof = true;
                    }
                    else if(row == 2 && line.find("L1") != std::string::npos) {
                        get_line_vals(line, 1, D_SIZE, mappings.at(idx).degrees);
                        if(!std::getline(config_file, line)) is_eof = true;
                    }
                    else if(row == 3 && line.find("S1_X") != std::string::npos) {
                        get_line_vals(line, 1, D_SIZE, mappings.at(idx).degrees);
                        if(!std::getline(config_file, line)) is_eof = true;
                    }
                    else if(row == 4 && line.find("S1_Y") != std::string::npos) {
                        get_line_vals(line, 1, D_SIZE, mappings.at(idx).degrees);
                        if(!std::getline(config_file, line)) is_eof = true;
                    }
                    else if(row == 5 && line.find("L2") != std::string::npos) {
                        get_line_vals(line, 1, D_SIZE, mappings.at(idx).degrees);
                        if(!std::getline(config_file, line)) is_eof = true;
                    }
                    else if(row == 6 && line.find("S2") != std::string::npos) {
                        get_line_vals(line, 1, D_SIZE, mappings.at(idx).degrees);
                        if(!std::getline(config_file, line)) is_eof = true;
                    }
                    else {
                        for(unsigned column = 0; column < D_SIZE; column++)
                            mappings.at(idx).degrees.push_back(1);
                    }
                    if(is_eof) {
                        for(unsigned r = row + 1; r < U_SIZE - 1; r++) {
                            for(unsigned column = 0; column < D_SIZE; column++)
                                mappings.at(idx).degrees.push_back(1);
                        }
                        break;
                    } 
                }
            }
            if(line[0] != '[') {
                while(std::getline(config_file, line)) { 
                    // Erase all spaces
                    line.erase(remove(line.begin(), line.end(), ' '), line.end());
                    // Skip blank lines or comments
                    if(!line.size() || line[0] == '#' || line[0] == ',') continue;
                    // Next mapping table
                    else if(line[0] == '[') break;
                    else handler.print_err(err_type_t::INVAILD, "Line typo");
                }
            }
        }
        else handler.print_err(err_type_t::INVAILD, "Line typo");
    }
    return;
}

/* Network configuration */
net_cfg_t::net_cfg_t(const std::string& cfg_path_) 
    : configs_t(cfg_path_),
    network_name("NO NAME") { 
    parse();
}

net_cfg_t::~net_cfg_t() {

}

void net_cfg_t::parse() {
    std::string line;
    // Start parsing
    while(std::getline(config_file, line)) {
        // Erase all spaces
        line.erase(remove(line.begin(), line.end(), ' '), line.end());
        // Skip blank lines or comments
        if(!line.size() || line[0] == '#' || line[0] == ',') continue;
        // Parse [NET:NETWORK_NAME]
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
                            || line.find("GROUP") != std::string::npos
                            || line.find("DEPTH") != std::string::npos;
            if(!is_layer)
                handler.print_err(err_type_t::INVAILD, "NET parsing");
            // Parse layer values
            layer_t layer;
            layer.is_grouped = line.find("GROUP") != std::string::npos
                            || line.find("DEPTH") != std::string::npos;
            layer.stride = get_line_uint(line, D_SIZE + 1);
            layer.name = get_line_string(line, 0);
            get_line_vals(line, 1, D_SIZE, layer.values);
            layers.push_back(layer);
        }
    }
    return;
}
