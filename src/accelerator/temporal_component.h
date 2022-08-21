#ifndef __TEMPORAL_COMPONENT_H__
#define __TEMPORAL_COMPONENT_H__

#include "component.h"

class temporal_component_t : public component_t {
public:
    temporal_component_t();
    virtual ~temporal_component_t();
    void init(section_config_t section_config_);
    std::string        get_name() const;
    std::vector<data_t> get_bypass() const; 
    unsigned           get_bandwidth() const; 
    component_type_t   get_type() const;
    std::vector<float> get_size() const;
    std::vector<unsigned> get_allocated_size(direction_t direction_) const;
    std::vector<unsigned> get_active_components() const;
    std::vector<unsigned> get_tile_access_count(operation_t operation_,
                                                direction_t direction_) const;
    dataflow_t         get_dataflow() const;
    
    std::vector<float> get_unit_energy() const;           // Get component access energy
    std::vector<float> get_unit_static_power() const;     // Get component static power
    std::vector<float> get_unit_cycle() const;            // Get component access energy

    void update_dataflow(dataflow_t dataflow_); 
    void update_allocated_tile_size(unsigned size_, data_t data_type_,
                                    direction_t direciton_);
    void update_active_components(std::vector<unsigned> size_); 
    void update_tile_access_count(unsigned size_, data_t data_type_,
                                  operation_t operation_,
                                  direction_t direction_);
    
    void print_spec();
    void print_stats();
    void reset();
private:
    // Post-processing to separate multiple values which exist in a same line.
    std::vector<data_t> set_bypass(std::string value_) const;   // Set bypass 
    std::vector<float>       set_size(std::string value_) const;     // Set size

    unsigned bandwidth;                                              // Bandwidth
    dataflow_t dataflow_origin;
};

#endif
