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
    // Constructor for optimizer
    analyzer_t(const std::string& accelerator_pth_,
               const std::string& network_pth_,
               const std::string& scheduling_table_pth_);
    // Constructor for analyzer
    analyzer_t(accelerator_t *accelerator_,
               network_t    *network_);
    ~analyzer_t();
    // Run analyzer
    void run();
    // Init analyzer
    void init(scheduling_table_t scheduling_table_);
    // Check scheduling table's validity
    bool check_validity() const;
    // Estimate accelerator's cost for a given scheduling table
    void estimate_cost();
    // Estimate the amount of data reuse between adjacent layers
    void estimate_cross_layer_reuse(scheduling_table_t prev_table_,
                                    metric_t      metric_);
    // Print out analysis result
    void print_results();
    // Reset analyzer variables
    void reset();
    // Get accelerator's target cost (Energy or Cycle)
    float get_total_cost(metric_t metric_);
    // Get the cost of targeted level 
    float get_target_level_cost(unsigned idx_, metric_t metric_);
    // Get factorzation degrees of targeted level
    unsigned get_target_level_factorization(unsigned idx_);
    // Get Num. active chips
    unsigned get_num_active_chips();
    // Get tile size of the row of scheduling table 
    unsigned get_tile_size(unsigned idx_, data_t data_type_);
    // Get tile-granular access count of the row of scheduling table 
    unsigned get_access_count(unsigned idx_, data_t data_type_);

private:
    // Check hardware constraints
    bool check_hardware_constraints() const;
    // Check active components doesn't exceed physical # components or not
    bool check_spatial_validity(unsigned idx_) const;
    // Check allocated tile size fit in buffer capacity or not
    bool check_temporal_validity(unsigned idx_) const;
    // Check parameter constraints
    bool check_network_constraints() const;
    // Update tile size to receive from adjacent buffer (or memory)
    void update_tile_size_to_receive();
    // Update tile size to send to adjacent buffer (or memory)
    void update_tile_size_to_send();
    // Update tile-granular access count
    void update_access_count();
    // Update num. activated components
    void update_active_components();
    // Update accelerator's cycle
    void update_cycle();
    // Update accelerator's energy (static + dynamic)
    void update_energy();
    // Update accelerator's static energy
    void update_static_energy();

    // Update input tile size to receive (or send)
    void update_component_input_tile_size(unsigned idx_, 
                                          std::vector<unsigned> values_,
                                          direction_t direction_);
    // Update weight tile size to receive (or send)
    void update_component_weight_tile_size(unsigned idx_, 
                                          std::vector<unsigned> values_,
                                          direction_t direction_);
    // Update output tile size to receive (or send)
    void update_component_output_tile_size(unsigned idx_, 
                                          std::vector<unsigned> values_,
                                          direction_t direction_);
    // Update tile-granular input access count
    void update_component_input_access_count(unsigned idx_, 
                                             unsigned value_);
    // Update tile-granular weight access count
    void update_component_weight_access_count(unsigned idx_, 
                                              unsigned value_);
    // Update tile-granular output access count
    void update_component_output_access_count(unsigned idx_, 
                                              unsigned value_);
    // Get energy of component level
    float get_energy_consumption(unsigned component_idx_,
                                 direction_t direction_);
    // Get cycle of target component level
    float get_cycle_consumption(unsigned component_idx_,
                                direction_t direction_);
    // Get factorziation degrees in target component level
    unsigned get_factorization_degrees(unsigned idx_);
    
    unsigned update_irrelevant_mapping_value(unsigned row_idx_,
                                             data_t stationary_data_);
    
    unsigned compute_overlapped_size(scheduling_table_t prev_table_); 

    accelerator_t      *accelerator;
    network_t          *network;
    scheduling_table_t scheduling_table;

    std::vector<float> components_energy;
    std::vector<float> components_static_energy;
    std::vector<float> components_cycle;

    unsigned access_count = 0;
    unsigned write_access_count = 0;
    unsigned read_access_count  = 0;
    
    unsigned component_idx_curr = 0;
    unsigned component_idx_lower= 0;
    unsigned component_idx_upper= 0;

    float    total_energy = 0;
    float    total_static_energy = 0;
    float    total_cycle  = 0;
};
#endif
