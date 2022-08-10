#ifndef __SPATIAL_COMPONENT_H__
#define __SPATIAL_COMPONENT_H__

#include "component.h"

class spatial_component_t : public component_t {
public:
    spatial_component_t();
    virtual ~spatial_component_t();
    void init(section_config_t section_config_);
    std::string        get_name() const;
    component_type_t   get_type() const;
    std::vector<float> get_size() const;
    std::vector<unsigned> get_allocated_size(direction_type_t direction_) const;
    std::vector<unsigned> get_tile_access_count(operation_type_t operation_,
                                                direction_type_t direction_) const;
    std::vector<unsigned> get_active_components() const;
    dataflow_t         get_dataflow() const;
    
    std::vector<float> get_unit_energy() const;           // Get component access energy
    std::vector<float> get_unit_cycle() const;            // Get component access energy
    
    void update_allocated_tile_size(unsigned size_, data_type_t data_type_,
                                    direction_type_t direciton_);
    void update_active_components(std::vector<unsigned> size_);
    void update_tile_access_count(unsigned size_, data_type_t data_type_,
                                  operation_type_t operation_,
                                  direction_type_t direction_);
    
    void print_spec();
    void print_stats();
private:
    float size_x;                        // horizontally arranged size
    float size_y;                        // vertically arranged size
};

#endif
