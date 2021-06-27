#ifndef __STATS_H__
#define __STATS_H__

#include <iomanip>
#include <string>
#include <vector>

#include "accelerator.h"
#include "enums.h"
#include "mapping_table.h"
#include "utils.h"

/* Stats */
class stats_t {
public:
    // For fixed datatflows
    stats_t(const accelerator_t *accelerator_, 
            const mapping_table_t& mapping_table_);
    // For flexible datatflows
    stats_t(const accelerator_t *accelerator_, 
            const mapping_table_t& mapping_table_,
            const dataflow_t l1_dataflow_, 
            const dataflow_t l2_dataflow_);
    ~stats_t();
    // stats_t's APIs
    double get_total_energy() const;
    double get_energy(component_t U) const;
    double get_total_cycle() const;
    double get_total_edp() const;
    void print_stats() const;
    void print_csv() const;
    void update_stats();
    
    struct iteration_t {
        size_t input_rd_it;
        size_t filter_rd_it;
        size_t output_rd_it;
        size_t output_wt_it;
    };

private:
    // Update stats
    void update_tile_size();
    void update_iteration();
    void update_active_components();
    void update_noc();
    void update_energy();
    void update_utilization();
    void update_cycle();
    void update_edp();
    // Tile size
    size_t mac_input_tile_size; 
    size_t mac_input_tile_size_spatial;
    size_t mac_filter_tile_size;
    size_t mac_filter_tile_size_spatial;
    size_t mac_output_tile_size; 
    size_t mac_output_tile_size_spatial;
    size_t l1_input_tile_size; 
    size_t l1_input_tile_size_spatial; 
    size_t l1_filter_tile_size;
    size_t l1_filter_tile_size_spatial;
    size_t l1_output_tile_size; 
    size_t l1_output_tile_size_spatial; 
    size_t l2_input_tile_size; 
    size_t l2_input_tile_size_spatial;
    size_t l2_filter_tile_size;
    size_t l2_output_tile_size;
    // Iteration 
    iteration_t l1_iteration;
    iteration_t l2_iteration;
    iteration_t dram_iteration;
    // NoC
    size_t num_s0_input_hosts;
    size_t num_s0_filter_hosts;
    size_t num_s0_output_hosts;
    size_t num_s1_input_hosts;
    size_t num_s1_filter_hosts;
    size_t num_s1_output_hosts;
    size_t num_s2_input_hosts;
    size_t num_s2_filter_hosts;
    size_t num_s2_output_hosts;
    // Active components
    size_t num_active_macs;
    size_t num_active_pes;
    size_t num_active_accs;
    // Energy
    double mac_energy;
    double l1_energy;
    double l1_energy_upper;
    double l1_energy_lower;
    double l2_energy;
    double l2_energy_upper;
    double l2_energy_lower;
    double dram_energy;
    double total_energy;
    // Utilization
    float s0_utilization;
    float l1_utilization;
    float s1_utilization;
    float l2_utilization;
    float s2_utilization;
    // Cycle
    double mac_cycle;
    double l1_cycle;
    double l2_cycle;
    double dram_cycle;
    double total_cycle;
    // EDP
    double total_edp;
    // Requisites 
    const dataflow_t l1_dataflow;
    const dataflow_t l2_dataflow;
    const accelerator_t *accelerator;
    const mapping_table_t mapping_table;
};

#endif
