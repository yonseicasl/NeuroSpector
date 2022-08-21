#include "spatial_component.h"

// Spatial component Class
spatial_component_t::spatial_component_t() 
    :size_x(1),
     size_y(1) { }
spatial_component_t::~spatial_component_t() {}

void spatial_component_t::init(section_config_t section_config_) { 
    // Get component name
    name = section_config_.name;
    // Get component type
    type = component_type_t::SPATIAL;
    // Get component size
    section_config_.get_setting("size_x", &size_x);
    section_config_.get_setting("size_y", &size_y);
    size.clear();
    size.push_back(size_x);
    size.push_back(size_y);
    return;
}
std::string spatial_component_t::get_name() const {
    return name;
}
component_type_t spatial_component_t::get_type() const {
    return type;
}
std::vector<float> spatial_component_t::get_size() const {
    return size;
}
std::vector<unsigned> spatial_component_t::get_allocated_size(direction_t direction_) const { 
    return allocated_size.at(direction_);
}
std::vector<unsigned> spatial_component_t::get_tile_access_count(operation_t operation_, direction_t direction_) const { 
    std::vector<unsigned> rtn;
    if(direction_ == direction_t::UPPER) { rtn = access_count_to_upper.at(operation_); }
    else if(direction_ == direction_t::LOWER) { rtn = access_count_to_lower.at(operation_); }
    else { std::cerr << "Error invalid direction type" << std::endl; exit(0); }
    return rtn; 
}
std::vector<unsigned> spatial_component_t::get_active_components() const { 
    return active_components;
}
// Get dataflow
dataflow_t spatial_component_t::get_dataflow() const {
    return dataflow;
}
// Get component access energy
std::vector<float> spatial_component_t::get_unit_energy() const {
    return unit_energy;
}
// Get component access energy
std::vector<float> spatial_component_t::get_unit_cycle() const {
    return unit_cycle;
}
// Update allocated tile size of component
void spatial_component_t::update_allocated_tile_size(unsigned size_, data_t data_type_,
                                                     direction_t direction_) {
    // Nothing to do.
    return;
}
// Update tile access count of component
void spatial_component_t::update_tile_access_count(unsigned size_, data_t data_type_,
                                                   operation_t operation_,
                                                   direction_t direction_) {
    // Nothing to do.
    return;
}
// Update total number of active components
void spatial_component_t::update_active_components(std::vector<unsigned> size_) {
    // Update allocated data in spatially arranged components
    assert(active_components.size() == 2);
    active_components.at(0) = size_.at(0); // Dimension X
    active_components.at(1) = size_.at(1); // Dimension Y
    return;
}
void spatial_component_t::print_spec() {
    std::string type_str[2]  = {"temporal", "spatial"};
    std::string dimension[2]  = {"(X)", "(Y)"};

    std::cout << "[ comp. type] " << type_str[(unsigned)type] << "\n"
              << "[ comp. size] "; 
    for(unsigned i = 0; i < size.size(); i++) {
        std::cout << size.at(i) << dimension[i] << "  ";
    }
    std::cout << std::endl;
}
void spatial_component_t::print_stats() {
    std::string dimension[2]  = {"DIM X", "DIM Y"};
    std::cout << "- [Activated Components]\n";
    for(unsigned i = 0; i < active_components.size(); i++) {
        std::cout << "-- " << dimension[i] << " = "
                  << active_components.at(i) << "\n";
    }
    float num_active_components = 1.0f;
    float num_total_components  = 1.0f;
    float utilization = 0.0f;
    for(unsigned i = 0; i < active_components.size(); i++) {
        num_active_components *= active_components.at(i); 
        num_total_components *= size.at(i); 
    }
    utilization = num_active_components/num_total_components;
    std::cout.precision(2); 
    std::cout << "- [Utilization (%)] = "
              << std::fixed 
              << utilization * 100
              << std::endl;
    return;
}
