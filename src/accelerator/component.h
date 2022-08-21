#ifndef __COMPONENT_H__
#define __COMPONENT_H__

#include <iostream>
#include <string>
#include <cassert>

#include "enums.h"
#include "parser.h"

class component_t {
public:
    component_t();
    virtual ~component_t();
    virtual void init(section_config_t section_config_);
    virtual std::string        get_name() const;                  // Get component name
    virtual component_type_t   get_type() const;                  // Get component type
    virtual std::vector<float>    get_size() const;               // Get component size
    virtual std::vector<unsigned> get_allocated_size(direction_t direction_) const;     // Get allocated data size
    virtual std::vector<unsigned> get_tile_access_count(operation_t operation_, 
                                                        direction_t direction) const;  // Get tile access count
    virtual std::vector<unsigned> get_active_components() const;  // Get active components
    virtual std::vector<data_t> get_bypass() const;          // Get bypass 
    virtual unsigned           get_bandwidth() const;             // Get bandwidth
    virtual dataflow_t         get_dataflow() const;              // Get dataflow
    
    virtual std::vector<float> get_unit_energy() const;           // Get component access energy
    virtual std::vector<float> get_unit_static_power() const;     // Get component static power
    virtual std::vector<float> get_unit_cycle() const;            // Get component access energy

    virtual void update_dataflow(dataflow_t dataflow_); 
    virtual void update_allocated_tile_size(unsigned size_,
                                            data_t data_type_,
                                            direction_t direction_);
    virtual void update_tile_access_count(unsigned size_,
                                          data_t data_type_,
                                          operation_t operation_,
                                          direction_t direction_);
    virtual void update_active_components(std::vector<unsigned> size_);
    virtual void print_spec();
    virtual void print_stats();
    virtual void reset();
protected:
    std::string        name;                     // Component name
    component_type_t   type;                     // Component type
    std::vector<float> size;                     // Component size
    std::vector<data_t> bypass;             // Bypassed data types
    std::map<direction_t, std::vector<unsigned>> allocated_size;        // Allocated size
    std::vector<unsigned> active_components;     // Num. active components 
    // std::map<operation_t, std::vector<unsigned>> tile_access_count;        // Tile access count
    std::map<operation_t, std::vector<unsigned>> access_count_to_upper;        // Tile access count
    std::map<operation_t, std::vector<unsigned>> access_count_to_lower;        // Tile access count
    dataflow_t         dataflow;                 // Dataflow in component level

    std::vector<float> unit_energy;              // Unit access energy (dynamic)
    std::vector<float> unit_static_power;        // Unit static power
    std::vector<float> unit_cycle;               // Unit access cycle
    
    std::vector<float> static_power; // Static power 
};

#endif
