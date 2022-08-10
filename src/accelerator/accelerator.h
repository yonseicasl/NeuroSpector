#ifndef __ACCELERATOR_H__
#define __ACCELERATOR_H__

#include "component.h"

class accelerator_t {
public:
    accelerator_t(const std::string &cfg_path_); 
    accelerator_t(const accelerator_t &accelerator_); // Copy constructor
    ~accelerator_t();
    void init_accelerator();
    void generate_components(const parser_t parser); 
    unsigned get_num_components(); 

    std::string get_name(const unsigned idx_);
    unsigned get_bandwidth(const unsigned idx_);
    std::vector<data_type_t> get_bypass(const unsigned idx_); 
    dataflow_t get_dataflow(const unsigned idx_);
    component_type_t get_type(const unsigned idx_);
    unsigned get_precision();

    std::vector<float>    get_size(const unsigned idx_);
    std::vector<unsigned> get_allocated_size(const unsigned idx_, 
                                             direction_type_t direction_);
    std::vector<unsigned> get_tile_access_count(const unsigned idx_, 
                                                operation_type_t operation,
                                                direction_type_t direction_);
    unsigned              get_active_MACs(); 
    std::vector<unsigned> get_active_components(const unsigned idx_);
    unsigned              get_total_num_MACs();
    unsigned              get_total_num_chips();
    unsigned              get_total_num_components(const unsigned idx_);

    float get_clock_time();
    float get_mac_energy();
    float get_mac_static_power();
    std::vector<float> get_component_energy(const unsigned idx_);
    float              get_component_static_power(const unsigned idx_);
    std::vector<float> get_component_cycle(const unsigned idx_);

    void update_dataflow(unsigned idx_, dataflow_t dataflow_);
    void update_allocated_tile_size(unsigned dst_, unsigned size_, 
                                    data_type_t data_type_,
                                    direction_type_t direction_);
    void update_tile_access_count(unsigned dst_, unsigned size_, 
                                  data_type_t data_type,
                                  operation_type_t operation_,
                                  direction_type_t direction_);
    void update_active_components(unsigned dst_, 
                                  std::vector<unsigned> size_); 
    void update_accelerator_energy(float energy_);
    void update_accelerator_cycle(float cycle_);

    void print_spec();
    void print_stats();
    void reset();
private:
    std::string               acc_cfg_path;
    std::string               name;
    unsigned                  precision;
    unsigned                  clock_frequency;
    float                     mac_static_power;
    float                     mac_operation_energy;

    std::vector<float>        energy;
    std::vector<float>        cycle;
    std::vector<component_t*> components;
};

#endif
