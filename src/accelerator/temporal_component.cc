#include "temporal_component.h"

// Component Class
temporal_component_t::temporal_component_t()
    : bandwidth(1) {
}
temporal_component_t::~temporal_component_t() {}

void temporal_component_t::init(section_config_t section_config_) { 
    std::string unrefined_value;
    // Get component name
    name = section_config_.name;
    // Set component type
    type = component_type_t::TEMPORAL;
    // Set component bypass
    unrefined_value = "";
    section_config_.get_setting("bypass", &unrefined_value);
    bypass = set_bypass(unrefined_value);
    // Set component bypass
    unrefined_value = "";
    section_config_.get_setting("buffer_size", &unrefined_value);
    if(!unrefined_value.empty()) { size = set_size(unrefined_value); }
    // Set dataflow
    unrefined_value = "";
    section_config_.get_setting("dataflow", &unrefined_value);
    if(!unrefined_value.empty()) {
        dataflow = (dataflow_t)get_enum_type(dataflow_str, unrefined_value);
        dataflow_origin = dataflow;
    }
    // Set unit access energy 
    unrefined_value = "";
    section_config_.get_setting("unit_access_energy", &unrefined_value);
    if(!unrefined_value.empty()) { unit_energy = set_size(unrefined_value); }
    // Set unit static power
    unrefined_value = "";
    section_config_.get_setting("unit_static_power", &unrefined_value);
    if(!unrefined_value.empty()) { unit_static_power = set_size(unrefined_value); }
    // Set unit cycle
    unrefined_value = "";
    section_config_.get_setting("unit_cycle", &unrefined_value);
    if(!unrefined_value.empty()) { unit_cycle = set_size(unrefined_value); }
    // Set bandwidth
    unrefined_value = "";
    section_config_.get_setting("bandwidth", &unrefined_value);
    if(!unrefined_value.empty()) { bandwidth = stoi(unrefined_value); }

    return;
}
// Get component name
std::string temporal_component_t::get_name() const {
    return name;
}
std::vector<data_type_t> temporal_component_t::get_bypass() const {
    return bypass;
}
unsigned temporal_component_t::get_bandwidth() const {
    return bandwidth;
}
// Get component type
component_type_t temporal_component_t::get_type() const {
    return type;
}
// Get component size
std::vector<float> temporal_component_t::get_size() const {
    return size;
}
// Get allocated data size in buffer
std::vector<unsigned> temporal_component_t::get_allocated_size(direction_type_t direction_) const { 
    return allocated_size.at(direction_);
}
std::vector<unsigned> temporal_component_t::get_active_components() const { 
    return active_components;
}
std::vector<unsigned> temporal_component_t::get_tile_access_count(operation_type_t operation_,
                                                                  direction_type_t direction_) const {
    std::vector<unsigned> rtn;
    if(direction_ == direction_type_t::UPPER) { rtn = access_count_to_upper.at(operation_); }
    else if(direction_ == direction_type_t::LOWER) { rtn = access_count_to_lower.at(operation_); }
    else { std::cerr << "Error invalid direction type" << std::endl; exit(0); }
    return rtn; 
}
// Get dataflow
dataflow_t temporal_component_t::get_dataflow() const {
    return dataflow;
}
// Get component access energy
std::vector<float> temporal_component_t::get_unit_energy() const {
    return unit_energy;
}
// Get component static power
std::vector<float> temporal_component_t::get_unit_static_power() const {
    return unit_static_power;
}
// Get component access energy
std::vector<float> temporal_component_t::get_unit_cycle() const {
    return unit_cycle;
}
// Set component bypass
std::vector<data_type_t> temporal_component_t::set_bypass(std::string value_) const {
    std::vector<std::string> splited_value;
    std::vector<data_type_t> rtn;
    
    // Get bypass configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        if(splited_value.at(i) == "input") { 
            rtn.push_back(data_type_t::INPUT); 
        }
        else if(splited_value.at(i) == "weight") { 
            rtn.push_back(data_type_t::WEIGHT); 
        }
        else if(splited_value.at(i) == "output") { 
            rtn.push_back(data_type_t::OUTPUT); 
        }
        else {
            std::cerr << "Error: invalid bypass configuration " 
                      << name << std::endl;
            exit(0);
        }
    }
    if(rtn.size() == 0) rtn.push_back(data_type_t::SIZE);
    return rtn;
}
// Set component size
std::vector<float> temporal_component_t::set_size(std::string value_) const {
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
// Update component's dataflow
void temporal_component_t::update_dataflow(dataflow_t dataflow_) {
    dataflow = dataflow_;
    return;
}
// Update allocated tile size of component
void temporal_component_t::update_allocated_tile_size(unsigned size_, data_type_t data_type_,
                                                      direction_type_t direction_) {
    allocated_size.at(direction_).at((unsigned)data_type_) = size_;
    return;
}
// Update tile access count of component
void temporal_component_t::update_tile_access_count(unsigned size_, data_type_t data_type_,
                                                    operation_type_t operation_,
                                                    direction_type_t direction_) {

    if(direction_ == direction_type_t::UPPER) { 
        access_count_to_upper.at(operation_).at((unsigned)data_type_) = size_; 
    }
    else if(direction_ == direction_type_t::LOWER) { 
        access_count_to_lower.at(operation_).at((unsigned)data_type_) = size_; 
    }
    else { std::cerr << "Error invalid direction type" << std::endl; exit(0); }
    return;
}
// Update total number of active components
void temporal_component_t::update_active_components(std::vector<unsigned> size_) {
    // Nothing to do.
    return;
}
// Print component hardware spec.
void temporal_component_t::print_spec() {
    std::string type_str[2]  = {"TEMPORAL", "SPATIAL"};
    std::string data_type[4] = {"INPUT", "WEIGHT", "OUTPUT", "NONE"};
    std::string dataflow_type[4] = {"none", "Input Stationary", "Weight Stationary", "Output Stationary"};
    std::cout << "[ comp. type] " << type_str[(unsigned)type] << "\n"
              << "[ comp. size] "; 
    for(unsigned i = 0; i < size.size(); i++) {
        std::cout << size.at(i) << "  ";
    }
    std::cout << std::endl;
    std::cout << "[     bypass] ";
    for(unsigned i = 0; i < bypass.size(); i++) {
        std::cout << data_type[(unsigned)bypass.at(i)] << "  ";
    }
    std::cout << std::endl;
    std::cout << "[  bandwidth] ";
    std::cout << bandwidth << "  ";
    std::cout << std::endl;
    std::cout << "[   dataflow] " << dataflow_type[(unsigned)dataflow] << std::endl;
    std::cout << "[unit energy] "; 
    for(unsigned i = 0; i < unit_energy.size(); i++) {
        std::cout << unit_energy.at(i) << "  ";
    }
    std::cout << std::endl;
    std::cout << "[unit static] "; 
    for(unsigned i = 0; i < unit_energy.size(); i++) {
        std::cout << unit_static_power.at(i) << "  ";
    }
    std::cout << std::endl;
    std::cout << "[ unit cycle] "; 
    for(unsigned i = 0; i < unit_energy.size(); i++) {
        std::cout << unit_cycle.at(i) << "  ";
    }
    std::cout << std::endl;
}
// Print component status
void temporal_component_t::print_stats() {
    std::string data_type[4] = {"input", "weight", "output", "NONE"};
    std::cout << "- [[Tile Size]] (exchange with upper level)\n";
    for(unsigned i = 0; i < allocated_size.at(direction_type_t::UPPER).size(); i++) { 
        std::cout << "--- [" << data_type[i] << "]="
                  << allocated_size.at(direction_type_t::UPPER).at(i) << "\n";
    }
    std::cout << "  [[Access Counts]] (exchange with upper level)\n"; 
    for(unsigned i = 0; i < access_count_to_upper.at(operation_type_t::READ).size(); i++) { 
        std::cout << "--- [" << data_type[i] << "(read)]="
                  << access_count_to_upper.at(operation_type_t::READ).at(i) << "\n";
    }
    for(unsigned i = 0; i < access_count_to_upper.at(operation_type_t::WRITE).size(); i++) { 
        std:: cout << "--- [" << data_type[i] << "(write)]="
                   << access_count_to_upper.at(operation_type_t::WRITE).at(i) << "\n";
    }
    std::cout << "- [[Tile Size]] (exchange with lower level)\n";
    for(unsigned i = 0; i < allocated_size.at(direction_type_t::LOWER).size(); i++) { 
        std:: cout << "--- [" << data_type[i] << "]="
                   << allocated_size.at(direction_type_t::LOWER).at(i) << "\n";
    }
    std::cout << "- [[Access Counts]] (exchange with lower level)\n"; 
    for(unsigned i = 0; i < access_count_to_lower.at(operation_type_t::READ).size(); i++) { 
        std::cout << "--- [" << data_type[i] << "(read)]="
                  << access_count_to_lower.at(operation_type_t::READ).at(i) << "\n";
    }
    for(unsigned i = 0; i < access_count_to_lower.at(operation_type_t::WRITE).size(); i++) { 
        std:: cout << "--- [" << data_type[i] << "(write)]="
                   << access_count_to_lower.at(operation_type_t::WRITE).at(i) << "\n";
    }
    return;
}
void temporal_component_t::reset() {
    dataflow = dataflow_origin;
    return;
}