#include <cassert>

#include "accelerator.h"
#include "temporal_component.h"
#include "spatial_component.h"

accelerator_t::accelerator_t(const std::string &cfg_path_) { 
    acc_cfg_path = cfg_path_;
    precision = 8; 
    mac_operation_energy = 1.0; 
}
accelerator_t::accelerator_t(const accelerator_t &accelerator_) 
    : acc_cfg_path(accelerator_.acc_cfg_path),
      precision(accelerator_.precision),
      mac_operation_energy(accelerator_.mac_operation_energy) {
        init_accelerator();
}
accelerator_t::~accelerator_t() {
    for(auto it = components.begin(); it != components.end(); ++it) {
        delete *it;
    }
};
void accelerator_t::init_accelerator() {
    // Parse the configuration file
    parser_t parser;
    parser.cfgparse(acc_cfg_path);

    // Generate components
    generate_components(parser);
}

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
            component_t *component = NULL;
            if(section_config.get_type() == "temporal") {
                component = new temporal_component_t();
            }
            else if(section_config.get_type() == "spatial") {
                component = new spatial_component_t();
            }
            else {
                std::cerr << "Error: unknown component type " 
                          << section_config.get_type() << std::endl;
                exit(1);
            }
            component->init(section_config);
            components.push_back(component);
        }
    }
}

unsigned accelerator_t::get_num_components() {
    return components.size();
}
std::string accelerator_t::get_name(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_name();
}
std::vector<data_t> accelerator_t::get_bypass(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_bypass();
}
unsigned accelerator_t::get_bandwidth(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_bandwidth();
}
dataflow_t accelerator_t::get_dataflow(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_dataflow();
}
component_type_t accelerator_t::get_type(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_type();
}
unsigned accelerator_t::get_precision() {
    return precision;
}
std::vector<float> accelerator_t::get_size(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_size();
}
std::vector<unsigned> accelerator_t::get_allocated_size(const unsigned idx_, direction_t direction_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_allocated_size(direction_);
}
std::vector<unsigned> accelerator_t::get_tile_access_count(const unsigned idx_, 
                                                           operation_t operation_, direction_t direction_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_tile_access_count(operation_, direction_);
}
unsigned accelerator_t::get_active_MACs() {
    unsigned rtn = 1;
    for(unsigned idx = 0; idx < get_num_components(); idx++) {
        if(get_type(idx) == component_type_t::SPATIAL) {
            rtn *= get_active_components(idx).at((unsigned)dimension_t::DIM_X)
                 * get_active_components(idx).at((unsigned)dimension_t::DIM_Y);
        }
    }
    return rtn;
}
std::vector<unsigned> accelerator_t::get_active_components(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_active_components();
}
unsigned accelerator_t::get_total_num_MACs() {
    unsigned rtn = 1;
    for(unsigned idx = 0; idx < get_num_components(); idx++) {
        if(get_type(idx) == component_type_t::SPATIAL) {
            rtn *= get_size(idx).at((unsigned)dimension_t::DIM_X)
                 * get_size(idx).at((unsigned)dimension_t::DIM_Y);
        }
    }
    return rtn;
}
unsigned accelerator_t::get_total_num_chips() {
    unsigned num_chips = 1;
    unsigned dram_idx = get_num_components() - 1;
    unsigned multi_chip_idx = dram_idx - 1; // Multi chip level exists above DRAM
    const unsigned dim_x = (unsigned)dimension_t::DIM_X;
    const unsigned dim_y = (unsigned)dimension_t::DIM_Y;
    if(components.at(multi_chip_idx)->get_type() == component_type_t::SPATIAL) {
        num_chips *= components.at(multi_chip_idx)->get_size().at(dim_x);
        num_chips *= components.at(multi_chip_idx)->get_size().at(dim_y);
    }
    return num_chips;
}
unsigned accelerator_t::get_total_num_components(const unsigned idx_) {
    unsigned rtn = 1;
    for(unsigned idx = idx_; idx < get_num_components(); idx++) {
        if(get_type(idx) == component_type_t::SPATIAL) {
            rtn *= get_size(idx).at((unsigned)dimension_t::DIM_X)
                 * get_size(idx).at((unsigned)dimension_t::DIM_Y);
        }
    }
    return rtn;
}
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
float accelerator_t::get_mac_static_power() {
    return mac_static_power;
}
float accelerator_t::get_mac_energy() {
    return mac_operation_energy;
}
std::vector<float> accelerator_t::get_component_energy(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_unit_energy();
}
float accelerator_t::get_component_static_power(const unsigned idx_) {
    assert(idx_ < components.size());
    float sum_static_power = 0;
    std::vector<float> static_power;
    static_power = components.at(idx_)->get_unit_static_power();
    for(auto it = static_power.begin(); it != static_power.end(); ++it) {
        sum_static_power += *it;
    }
    return sum_static_power;
}
std::vector<float> accelerator_t::get_component_cycle(const unsigned idx_) {
    assert(idx_ < components.size());
    return components.at(idx_)->get_unit_cycle();
}
// Update components dataflow
void accelerator_t::update_dataflow(unsigned idx_, dataflow_t dataflow_) {
    assert(idx_ < components.size());
    return components.at(idx_)->update_dataflow(dataflow_);
}
// Update allocated tile size
void accelerator_t::update_allocated_tile_size(unsigned dst_, unsigned size_, 
                                               data_t data_type_, 
                                               direction_t direction_) {
    assert(dst_ < components.size());
    components.at(dst_)->update_allocated_tile_size(size_, data_type_, direction_);
    return;
}
// Update tile access count
void accelerator_t::update_tile_access_count(unsigned dst_, unsigned size_, 
                                             data_t data_type_,
                                             operation_t operation_,
                                             direction_t direction_) {
    assert(dst_ < components.size());
    components.at(dst_)->update_tile_access_count(size_, data_type_, operation_, direction_);
    return;
}
// Update active components of spatially arranged components
void accelerator_t::update_active_components(unsigned dst_, 
                                             std::vector<unsigned> size_) {
    assert(dst_ < components.size());
    components.at(dst_)->update_active_components(size_);
    return;
}
void accelerator_t::update_accelerator_energy(float energy_) {
    energy.push_back(energy_);
    return;
}
void accelerator_t::update_accelerator_cycle(float cycle_) {
    cycle.push_back(cycle_);
    return;
}
void accelerator_t::print_spec() {
    std::cout << "==== Accelerator Spec ====" << std::endl;
    std::cout << "[ acc. name] " << name << std::endl;
    std::cout << "[ precision] " << get_precision() << std::endl;
    std::cout << "[ frequency] " << clock_frequency << std::endl;
    std::cout << "[clock time] " << get_clock_time() << std::endl;
    std::cout << "[MAC energy] " << get_mac_energy() << std::endl;
    std::cout << "[MAC static] " << get_mac_static_power() << "\n" << std::endl;
    for(unsigned idx = 0; idx < get_num_components(); idx++) {
            std::cout << "[ comp. name] "
                      << components.at(idx)->get_name() << std::endl;
            components.at(idx)->print_spec();
            std::cout << std::endl;
    }
    std::cout << "==========================" << std::endl;
}
void accelerator_t::print_stats() {
    std::cout << "==== Component stats  ====" << std::endl;
    for(unsigned idx = 0; idx < get_num_components(); idx++) {
            std::cout << "[["
                      << components.at(idx)->get_name() 
                      << "]]"
                      << std::endl;
            components.at(idx)->print_stats();
            std::cout << std::endl;
    }
    return;
}
void accelerator_t::reset() {
    for(unsigned idx = 0; idx < get_num_components(); idx++) {
        components.at(idx)->reset();
    }
    return;
}
