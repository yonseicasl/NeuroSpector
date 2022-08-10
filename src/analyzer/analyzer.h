#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <iostream>
#include <string>
#include <vector>

#include "accelerator.h"
#include "network.h"
#include "scheduling_table.h"

class analyzer_t{
public:
    analyzer_t(const std::string& accelerator_pth_,
               const std::string& network_pth_,
               const std::string& scheduling_table_pth_);
    analyzer_t(accelerator_t *accelerator_,
               network_t    *network_);
    ~analyzer_t();

    void run();
    void init(scheduling_table_t scheduling_table_);
    bool check_validity() const;
    void estimate_cost();
    void estimate_cross_layer_reuse(scheduling_table_t prev_table_,
                                    metric_type_t      metric_);
    void print_stats();
    void reset();

    float get_total_cost(metric_type_t metric_);
    float get_target_level_cost(unsigned idx_, metric_type_t metric_);
    unsigned get_target_level_factorization(unsigned idx_);
    unsigned get_active_chips();
    unsigned get_tile_size(unsigned idx_, data_type_t data_type_);
    unsigned get_access_count(unsigned idx_, data_type_t data_type_);

private:
    bool check_hardware_constraints() const;
    bool check_spatial_validity(unsigned idx_) const;
    bool check_temporal_validity(unsigned idx_) const;
    bool check_network_constraints() const;

    void update_tile_size_to_receive();
    void update_tile_size_to_send();
    void update_component_input_tile_size(unsigned idx_, 
                                          std::vector<unsigned> values_,
                                          direction_type_t direction_);
    void update_component_weight_tile_size(unsigned idx_, 
                                          std::vector<unsigned> values_,
                                          direction_type_t direction_);
    void update_component_output_tile_size(unsigned idx_, 
                                          std::vector<unsigned> values_,
                                          direction_type_t direction_);
    void update_access_count();
    void update_component_input_access_count(unsigned idx_, 
                                             unsigned value_);
    void update_component_weight_access_count(unsigned idx_, 
                                              unsigned value_);
    void update_component_output_access_count(unsigned idx_, 
                                              unsigned value_);
    unsigned update_irrelevant_mapping_value(unsigned row_idx_,
                                             data_type_t stationary_data_);
    void update_active_components();
    void update_cycle();
    void update_energy();
    void update_static_energy();
    
    unsigned compute_overlapped_size(scheduling_table_t prev_table_); 
    
    float get_energy_consumption(unsigned component_idx_,
                                 direction_type_t direction_);
    float get_cycle_consumption(unsigned component_idx_,
                                direction_type_t direction_);
    unsigned get_factorization_degrees(unsigned idx_);

    accelerator_t      *accelerator;
    network_t          *network;
    scheduling_table_t scheduling_table;

    unsigned access_count = 0;
    unsigned write_access_count = 0;
    unsigned read_access_count  = 0;
    
    unsigned component_idx_curr = 0;
    unsigned component_idx_lower= 0;
    unsigned component_idx_upper= 0;

    std::vector<float> components_energy;
    std::vector<float> components_static_energy;
    std::vector<float> components_cycle;

    float              total_energy = 0;
    float              total_static_energy = 0;
    float              total_cycle  = 0;
};
#endif
