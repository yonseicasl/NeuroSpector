#ifndef __ACCELERATOR_H__
#define __ACCELERATOR_H__

// #include "component.h"
#include "enums.h"
#include "parser.h"

class accelerator_t {
public:
    struct temporal_component_t {
        component_type_t   type = component_type_t::TEMPORAL;
        std::string        name;
        std::vector<float> size;
        dataflow_t         dataflow;
        unsigned           bitwidth;
        float unit_energy[(unsigned)data_t::SIZE] = { };
        float unit_static[(unsigned)data_t::SIZE] = { };
        float unit_cycle[(unsigned)data_t::SIZE] = { };
        bool  bypass[(unsigned)data_t::SIZE] = { false };
    };

    struct spatial_component_t {
        component_type_t   type = component_type_t::SPATIAL;
        std::string name;
        unsigned    dim_x = 1;
        unsigned    dim_y = 1;
    };
    // Construct components in accelerator 
    accelerator_t(const std::string &cfg_path_); 
    accelerator_t(const accelerator_t &accelerator_); // Copy constructor
    ~accelerator_t();

    // Generate components which make up the accelerator
    void generate_components(const parser_t parser); 
    
    float               get_clock_time();
    float               get_mac_energy();
    float               get_mac_static_power();
    unsigned            get_precision();

    // Get dimension size of spatially arranged components 
    unsigned            get_total_num_MACs();
    unsigned            get_total_num_PEs();
    unsigned            get_total_num_chips();
    unsigned            get_mac_array_size(dimension_t dim_);
    unsigned            get_pe_array_size(dimension_t dim_);
    unsigned            get_multi_chips_size(dimension_t dim_);
    // Get variables in temporal component structure
    std::string         get_name(ComponentType comp_);
    component_type_t    get_type(ComponentType comp_);
    bool*               get_bypass(ComponentType comp_); 
    std::vector<float>  get_size(ComponentType comp_);         
    dataflow_t          get_dataflow(ComponentType comp_);
    float               get_bitwidth(ComponentType comp_);         
    float*              get_energy(ComponentType comp_);
    float*              get_static(ComponentType comp_);
    float*              get_cycle(ComponentType comp_);
    // Check the component sizes is 1
    bool                is_unit_component(ComponentType comp_);
    
    // Print specifiations of accelerator
    void print_spec();
    void print_temporal_component(temporal_component_t* component_); 
    void print_spatial_component(spatial_component_t* component_); 
    
    
private:
    std::vector<float>  set_size(std::string value_) const;
    void                set_cost(float* cost_, std::string value_);
    void                set_bypass(bool* bypass, std::string value_);
    void init_temporal_component(temporal_component_t* component_,
                                 section_config_t section_config_); 
    void init_spatial_component(spatial_component_t* component_,
                                section_config_t section_config_); 

    std::string               acc_cfg_path;
    std::string               name;
    unsigned                  precision;
    unsigned                  clock_frequency;
    float                     mac_static_power;
    float                     mac_operation_energy;

    std::vector<float>        energy;
    std::vector<float>        cycle;

    void *component_list[(unsigned)ComponentType::SIZE];
    temporal_component_t *register_file;
    spatial_component_t  *mac_array;
    temporal_component_t *local_buffer;
    spatial_component_t  *pe_array;
    temporal_component_t *global_buffer;
    spatial_component_t  *multi_chips;
    temporal_component_t *dram;
};

#endif
