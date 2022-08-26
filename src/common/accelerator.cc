#include <cassert>

#include "accelerator.h"

#define BYTE 8

accelerator_t::accelerator_t(const std::string &cfg_path_) 
    : acc_cfg_path(cfg_path_),
      precision(8),
      mac_operation_energy(1.0) {
        // Initialize component list
        for(unsigned i = 0; i < (unsigned)ComponentType::SIZE; i++) {
            component_list[i] = nullptr;
        }
        // Parse the configuration file
        parser_t parser;
        parser.cfgparse(acc_cfg_path);

        // Generate components
        generate_components(parser);
}
// Copy constructor 
accelerator_t::accelerator_t(const accelerator_t &accelerator_) 
    : acc_cfg_path(accelerator_.acc_cfg_path),
      precision(accelerator_.precision),
      mac_operation_energy(accelerator_.mac_operation_energy) {
        // Parse the configuration file
        parser_t parser;
        parser.cfgparse(acc_cfg_path);

        // Generate components
        generate_components(parser);
}
accelerator_t::~accelerator_t() {
    delete register_file;
    delete mac_array;
    delete local_buffer;
    delete pe_array;
    delete global_buffer;
    delete multi_chips;
    delete dram;
};

// Generate components that make up accelerator
void accelerator_t::generate_components(const parser_t parser) {
    for(unsigned i = 0; i < parser.sections.size(); i++) {
        section_config_t section_config = parser.sections[i];
        if(section_config.name == "ACCELERATOR") {
            std::string precision_str;
            section_config.get_setting("name", &name);
            section_config.get_setting("precision", &precision_str);
            // Convert string value to int
            if(precision_str.length() != 0) precision = 0;
            for(unsigned i = 0; i < unsigned(precision_str.length()); i++) {
                if(precision_str[i] > 47 && precision_str[i] < 58)
                    precision = precision * 10 + precision_str[i] - 48;
            }
            section_config.get_setting("frequency", &clock_frequency);
            section_config.get_setting("unit_mac_energy", &mac_operation_energy);
            section_config.get_setting("unit_mac_static", &mac_static_power);
        }
        else {
            if(section_config.name == "REGISTER") {
                register_file = new temporal_component_t;
                init_temporal_component(register_file, section_config);
                component_list[(unsigned)ComponentType::REG] = register_file;
            }
            else if(section_config.name == "MAC_ARRAY") {
                mac_array = new spatial_component_t;
                init_spatial_component(mac_array, section_config);
                component_list[(unsigned)ComponentType::MAC_X] = mac_array;
                component_list[(unsigned)ComponentType::MAC_Y] = mac_array;
            }
            else if(section_config.name == "LOCAL_BUFFER") {
                local_buffer = new temporal_component_t;
                init_temporal_component(local_buffer, section_config);
                component_list[(unsigned)ComponentType::LB] = local_buffer;
            }
            else if(section_config.name == "PE_ARRAY") {
                pe_array = new spatial_component_t;
                init_spatial_component(pe_array, section_config);
                component_list[(unsigned)ComponentType::PE_X] = pe_array;
                component_list[(unsigned)ComponentType::PE_Y] = pe_array;
            }
            else if(section_config.name == "GLOBAL_BUFFER") {
                global_buffer = new temporal_component_t;
                init_temporal_component(global_buffer, section_config);
                component_list[(unsigned)ComponentType::GB] = global_buffer;
            }
            else if(section_config.name == "MULTI_CHIPS") {
                multi_chips = new spatial_component_t;
                init_spatial_component(multi_chips, section_config);
                component_list[(unsigned)ComponentType::CHIP_X] = multi_chips;
                component_list[(unsigned)ComponentType::CHIP_Y] = multi_chips;
            }
            else if(section_config.name == "DRAM") {
                dram = new temporal_component_t;
                init_temporal_component(dram, section_config);
                component_list[(unsigned)ComponentType::DRAM] = dram;
            }
        }
    }
}
std::string accelerator_t::get_name(ComponentType comp_) {
    std::string rtn;
    if(component_list[(unsigned)comp_] != nullptr) {
        rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->name;
    }
    else {
        rtn = "virtual";
    }
    return rtn;
}
component_type_t accelerator_t::get_type(ComponentType comp_) {
    component_type_t rtn;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        rtn = component_type_t::TEMPORAL;
    }
    else {
        rtn = component_type_t::SPATIAL;
    }
    return rtn;
}
// Get component size
std::vector<float> accelerator_t::get_size(ComponentType comp_) {
    std::vector<float> rtn;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->size;
        }
        else {
            rtn.emplace_back(precision/BYTE);  // Input
            rtn.emplace_back(precision/BYTE);  // Weight
            rtn.emplace_back(precision/BYTE);  // Output
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component dataflow
dataflow_t accelerator_t::get_dataflow(ComponentType comp_) {
    dataflow_t rtn = dataflow_t::NONE;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->dataflow;
        }
    }
    return rtn;
}
// Get component bitwidth
float accelerator_t::get_bitwidth(ComponentType comp_) {
    float rtn = precision;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->bitwidth;
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component energy
float* accelerator_t::get_energy(ComponentType comp_) {
    float* rtn;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->unit_energy;
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component static power
float* accelerator_t::get_static(ComponentType comp_) {
    float* rtn;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->unit_static;
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component cycle
float* accelerator_t::get_cycle(ComponentType comp_) {
    float* rtn;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->unit_cycle;
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component bypass
bool* accelerator_t::get_bypass(ComponentType comp_) {
    bool* rtn;
    if(comp_ == ComponentType::REG || comp_ == ComponentType::LB
    || comp_ == ComponentType::GB  || comp_ == ComponentType::DRAM) {
        rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->bypass;
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get Total number of MAC components in accelerator
unsigned accelerator_t::get_total_num_MACs() {
    unsigned rtn = 1;
    if(component_list[(unsigned)ComponentType::MAC_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)ComponentType::MAC_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)ComponentType::MAC_Y])->dim_y;
    }
    if(component_list[(unsigned)ComponentType::PE_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)ComponentType::PE_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)ComponentType::PE_Y])->dim_y;

    }
    if(component_list[(unsigned)ComponentType::CHIP_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_Y])->dim_y;
    }
    return rtn;
}
// Get Total number of PE components in accelerator
unsigned accelerator_t::get_total_num_PEs() {
    unsigned rtn = 1;
    if(component_list[(unsigned)ComponentType::PE_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)ComponentType::PE_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)ComponentType::PE_Y])->dim_y;

    }
    if(component_list[(unsigned)ComponentType::CHIP_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_Y])->dim_y;
    }
    return 1;
}
// Get Total number of CHIP components in accelerator
unsigned accelerator_t::get_total_num_chips() {
    unsigned rtn = 1;
    if(component_list[(unsigned)ComponentType::CHIP_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_Y])->dim_y;
    }
    return 1;
}
// Get MAC array width (or height)
unsigned accelerator_t::get_mac_array_size(dimension_t dim_) {
    unsigned rtn = 1;
    if(component_list[(unsigned)ComponentType::MAC_X] == nullptr) {
        return 1;
    }
    if(dim_ == dimension_t::DIM_X) {
        rtn = ((spatial_component_t*)component_list[(unsigned)ComponentType::MAC_X])->dim_x;
    }
    else {
        rtn = ((spatial_component_t*)component_list[(unsigned)ComponentType::MAC_Y])->dim_y;
    }
    return rtn;
}
// Get PE array width (or height)
unsigned accelerator_t::get_pe_array_size(dimension_t dim_) {
    unsigned rtn = 1;
    if(component_list[(unsigned)ComponentType::PE_X] == nullptr) {
        return 1;
    }
    if(dim_ == dimension_t::DIM_X) {
        rtn = ((spatial_component_t*)component_list[(unsigned)ComponentType::PE_X])->dim_x;
    }
    else {
        rtn = ((spatial_component_t*)component_list[(unsigned)ComponentType::PE_Y])->dim_y;
    }
    return rtn;
}
// Get Multi chips width (or height)
unsigned accelerator_t::get_multi_chips_size(dimension_t dim_) {
    unsigned rtn = 1;
    if(component_list[(unsigned)ComponentType::CHIP_X] == nullptr) {
        return 1;
    }
    if(dim_ == dimension_t::DIM_X) {
        rtn = ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_X])->dim_x;
    }
    else {
        rtn = ((spatial_component_t*)component_list[(unsigned)ComponentType::CHIP_Y])->dim_y;
    }
    return rtn;
}
bool accelerator_t::is_unit_component(ComponentType comp_) {
    bool rtn = false;
    std::vector<float> buff_size;
    switch (comp_) {
        case ComponentType::REG:
        case ComponentType::LB:
        case ComponentType::GB:
            buff_size = get_size(comp_);
            if(buff_size.size() == 1) {
                rtn = (buff_size.back() == (precision * (unsigned)data_t::SIZE) / BYTE);
            }
            else {
                rtn = (buff_size[(unsigned)data_t::INPUT]  == (precision / BYTE));
                rtn = rtn && (buff_size[(unsigned)data_t::WEIGHT] == (precision / BYTE));
                rtn = rtn && (buff_size[(unsigned)data_t::OUTPUT] == (precision / BYTE));
            }
            break;
        case ComponentType::MAC_X:
            rtn = (get_mac_array_size(dimension_t::DIM_X) == 1); 
            break;
        case ComponentType::MAC_Y:
            rtn = (get_mac_array_size(dimension_t::DIM_Y) == 1); 
            break;
        case ComponentType::PE_X:
            rtn = (get_pe_array_size(dimension_t::DIM_X) == 1); 
            break;
        case ComponentType::PE_Y:
            rtn = (get_pe_array_size(dimension_t::DIM_Y) == 1);
            break;
        case ComponentType::CHIP_X:
            rtn = (get_multi_chips_size(dimension_t::DIM_X)== 1);
            break;
        case ComponentType::CHIP_Y:
            rtn = (get_multi_chips_size(dimension_t::DIM_Y) == 1);
            break;
        default:
            break;
    }
    return rtn;
};

// Get required time per 1 cycle
float accelerator_t::get_clock_time() {
    /** Convert frequency to time (ns)
     *  ! Suppose Unit of Clock frequency (MHz) 
     *  Return value; Latency (ms) is calculated by 
     *  'Latency(ns) = (clock_frequency)^(-1) (us) * 10^3(kilo)'
     */
    float rtn = 0.0;
    rtn = (1.0 / (float)clock_frequency) * 1000;
    return rtn;
}
// Get MAC operation unit energy
float accelerator_t::get_mac_energy() {
    return mac_operation_energy;
}
// Get MAC operation unit static power
float accelerator_t::get_mac_static_power() {
    return mac_static_power;
}
// Get data precision of MAC
unsigned accelerator_t::get_precision() {
    return precision;
}
// Print specifiations of accelerator
void accelerator_t::print_spec() {
    std::cout << "==== Accelerator Spec ====" << std::endl;
    std::cout << "[ acc. name] " << name << std::endl;
    std::cout << "[ precision] " << get_precision() << std::endl;
    std::cout << "[ frequency] " << clock_frequency << std::endl;
    std::cout << "[clock time] " << get_clock_time() << std::endl;
    std::cout << "[MAC energy] " << get_mac_energy() << std::endl;
    std::cout << "[MAC static] " << get_mac_static_power() << std::endl;
    for(unsigned i = 0; i < (unsigned)ComponentType::SIZE; i++) {
        if(component_list[i] == nullptr) { continue; }
        std::cout << std::endl;
        if(((temporal_component_t*)component_list[i])->type == component_type_t::TEMPORAL) {
            std::cout << "[ comp. name] " << ((temporal_component_t*)component_list[i])->name << std::endl;
            print_temporal_component((temporal_component_t*)component_list[i]);
        }
        else {
            std::cout << "[ comp. name] " << ((spatial_component_t*)component_list[i])->name << std::endl;
            print_spatial_component((spatial_component_t*)component_list[i]);
            i++;
        }
    }
    std::cout << "==========================" << std::endl;
}
// Print out temporal component
void accelerator_t::print_temporal_component(temporal_component_t* component_) {
    std::string type_str[2]  = {"TEMPORAL", "SPATIAL"};
    std::string data_type[4] = {"INPUT", "WEIGHT", "OUTPUT", "NONE"};
    std::string dataflow_type[4] = {"none", "Input Stationary", "Weight Stationary", "Output Stationary"};
    std::cout << "[ comp. type] " << type_str[(unsigned)component_->type] << "\n"
              << "[ comp. size] "; 
    for(unsigned i = 0; i < component_->size.size(); i++) {
        std::cout << component_->size.at(i) << "  ";
    }
    std::cout << std::endl;
    std::cout << "[     bypass] ";
    for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
        if(component_->bypass[i]) {
            std::cout << data_type[i] << " ";
        }
    }
    std::cout << std::endl;
    std::cout << "[   bitwidth] ";
    std::cout << component_->bitwidth << "  ";
    std::cout << std::endl;
    std::cout << "[   dataflow] " << dataflow_type[(unsigned)component_->dataflow] << std::endl;
    std::cout << "[unit energy] "; 
    for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
        std::cout << component_->unit_energy[i] << "  ";
    }
    std::cout << std::endl;
    std::cout << "[unit static] "; 
    for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
        std::cout << component_->unit_static[i] << "  ";
    }
    std::cout << std::endl;
    std::cout << "[ unit cycle] "; 
    for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
        std::cout << component_->unit_cycle[i] << "  ";
    }
    std::cout << std::endl;
    return;
}
// Print out spatial component
void accelerator_t::print_spatial_component(spatial_component_t* component_) {
    std::string type_str[2]  = {"TEMPORAL", "SPATIAL"};

    std::cout << "[ comp. type] " << type_str[(unsigned)component_->type] << "\n"
              << "[ comp. dimX] " << component_->dim_x << "\n"
              << "[ comp. dimY] " << component_->dim_y << std::endl; 
    return;
}

// Parsing temporal component
void accelerator_t::init_temporal_component(temporal_component_t* component_,
                                            section_config_t section_config_) {
    std::string unrefined_value;
    // Set component type
    component_->type = component_type_t::TEMPORAL;
    // Get component name
    component_->name = section_config_.name;
    // Set component size
    unrefined_value = "";
    section_config_.get_setting("buffer_size", &unrefined_value);
    if(!unrefined_value.empty()) { 
        component_->size = set_size(unrefined_value); 
    }
    // Set component bypass
    unrefined_value = "";
    section_config_.get_setting("bypass", &unrefined_value);
    if(!unrefined_value.empty()) { set_bypass(component_->bypass, unrefined_value); }
    // Set dataflow
    unrefined_value = "";
    section_config_.get_setting("dataflow", &unrefined_value);
    if(!unrefined_value.empty()) {
        component_->dataflow = (dataflow_t)get_enum_type(dataflow_str, unrefined_value);
    }
    // Set bitwidth
    unrefined_value = "";
    section_config_.get_setting("bandwidth", &unrefined_value);
    if(!unrefined_value.empty()) { component_->bitwidth = stoi(unrefined_value); }
    // Set unit access energy 
    unrefined_value = "";
    section_config_.get_setting("unit_access_energy", &unrefined_value);
    if(!unrefined_value.empty()) { set_cost(component_->unit_energy, unrefined_value); }
    // Set unit static power
    unrefined_value = "";
    section_config_.get_setting("unit_static_power", &unrefined_value);
    if(!unrefined_value.empty()) { set_cost(component_->unit_static, unrefined_value); }
    // Set unit cycle
    unrefined_value = "";
    section_config_.get_setting("unit_cycle", &unrefined_value);
    if(!unrefined_value.empty()) { set_cost(component_->unit_cycle, unrefined_value); }

    return;
}
// Parsing spatial component
void accelerator_t::init_spatial_component(spatial_component_t* component_,
                                           section_config_t section_config_) {
    // Get component type
    component_->type = component_type_t::SPATIAL;
    // Get component name
    component_->name = section_config_.name;
    // Get component size
    section_config_.get_setting("size_x", &component_->dim_x);
    section_config_.get_setting("size_y", &component_->dim_y);
    return;

}
// Set component size
std::vector<float> accelerator_t::set_size(std::string value_) const {
    std::vector<std::string> splited_value;
    std::vector<float> rtn;   
    // Get buffer size configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        rtn.push_back(std::stof(splited_value.at(i)));
    }
    splited_value.clear();
    return rtn;
}
// Set component cost
void accelerator_t::set_cost(float* cost_, std::string value_) {
    std::vector<std::string> splited_value;
    // Get buffer size configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        cost_[i] = std::stof(splited_value.at(i));
    }
    splited_value.clear();
    return;
}
// Set component bypass
void accelerator_t::set_bypass(bool* bypass_, std::string value_) {
    std::vector<std::string> splited_value;
    
    // Get bypass configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        if(splited_value.at(i) == "input") { 
            bypass_[(unsigned)data_t::INPUT] = true;
        }
        else if(splited_value.at(i) == "weight") { 
            bypass_[(unsigned)data_t::WEIGHT] = true;
        }
        else if(splited_value.at(i) == "output") { 
            bypass_[(unsigned)data_t::OUTPUT] = true;
        }
        else {
            std::cerr << "Error: invalid bypass configuration " 
                      << splited_value.at(i)<< std::endl;
            exit(0);
        }
    }
    return;
}