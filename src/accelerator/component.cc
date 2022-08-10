#include "component.h"

// Component Class
component_t::component_t()
    :name(""),
     type(component_type_t::SIZE),
     size(1,0),
     active_components(2,1),
     dataflow(dataflow_t::NONE),
     unit_energy(3, 0.0),
     unit_cycle(3, 0.0) {
        std::vector<unsigned> tile_size((unsigned)data_type_t::SIZE, 0);
        std::vector<unsigned> access_count((unsigned)data_type_t::SIZE, 0);
        allocated_size.insert(std::pair<direction_type_t, std::vector<unsigned>>(direction_type_t::UPPER, tile_size));
        allocated_size.insert(std::pair<direction_type_t, std::vector<unsigned>>(direction_type_t::LOWER, tile_size));
        access_count_to_upper.insert(std::pair<operation_type_t, std::vector<unsigned>>(operation_type_t::READ, access_count));
        access_count_to_upper.insert(std::pair<operation_type_t, std::vector<unsigned>>(operation_type_t::WRITE, access_count));
        access_count_to_lower.insert(std::pair<operation_type_t, std::vector<unsigned>>(operation_type_t::READ, access_count));
        access_count_to_lower.insert(std::pair<operation_type_t, std::vector<unsigned>>(operation_type_t::WRITE, access_count));
        
     }
component_t::~component_t() {}
// Init component 
void component_t::init(section_config_t section_config_) { 
    return; 
}
// Get component name
std::string component_t::get_name() const { 
    return name; 
}
// Get component type (temporal or spatial)
component_type_t component_t::get_type() const { 
    return type; 
}
// Get component size (buffer size or # spatially arranged)
std::vector<float> component_t::get_size() const { 
    return size; 
}
// Get allocated size (buffer size or # spatially arranged)
std::vector<unsigned> component_t::get_allocated_size(direction_type_t direction_) const { 
    return allocated_size.at(direction_); 
}
// Get allocated size (buffer size or # spatially arranged)
std::vector<unsigned> component_t::get_tile_access_count(operation_type_t operation_, 
                                                         direction_type_t direction_) const { 
    std::vector<unsigned> rtn;
    if(direction_ == direction_type_t::UPPER) { rtn = access_count_to_upper.at(operation_); }
    else if(direction_ == direction_type_t::LOWER) { rtn = access_count_to_lower.at(operation_); }
    else { std::cerr << "Error invalid direction type" << std::endl; exit(0); }
    return rtn; 
}
std::vector<unsigned> component_t::get_active_components() const { 
    return active_components; 
}
// Get bypass
std::vector<data_type_t> component_t::get_bypass() const {
    return bypass;
}
// Get bypass
unsigned component_t::get_bandwidth() const {
    return 1;
}
// Get dataflow
dataflow_t component_t::get_dataflow() const {
    return dataflow;
}
// Get component access energy
std::vector<float> component_t::get_unit_energy() const {
    return unit_energy;
}
// Get component static power
std::vector<float> component_t::get_unit_static_power() const {
    return unit_static_power;
}
// Get component access energy
std::vector<float> component_t::get_unit_cycle() const {
    return unit_cycle;
}
// Update dataflow
void component_t::update_dataflow(dataflow_t dataflow_) { 
    return;
}
// Update allocated tile size of component
void component_t::update_allocated_tile_size(unsigned size_,
                                             data_type_t data_type_,
                                             direction_type_t direction_) {
    return;
}
// Update tile access count of component
void component_t::update_tile_access_count(unsigned size_, 
                                           data_type_t data_type,
                                           operation_type_t operation_,
                                           direction_type_t direction_) {
    return;
}
// Update total number of active components
void component_t::update_active_components(std::vector<unsigned> size_) {
    return;
}
// Print component hardware information
void component_t::print_spec() { 
    return; 
}
// Print component stats
void component_t::print_stats() { 
    return; 
}
// Reset component configurations
void component_t::reset() {
    return;
}