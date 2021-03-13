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
    stats_t(const accelerator_t *accelerator_, 
            const mapping_table_t *mapping_table_);
    ~stats_t();

    void print_stats() const;
    void print_csv() const;
    void update_stats();
    size_t get_total_energy() const { return total_energy; }
    size_t get_dram_energy() const { return dram_energy; }
    size_t get_l2_energy() const { return l2_energy; }
    size_t get_l1_energy() const { return l1_energy; }
    size_t get_total_cycle() const { return total_cycle; }
    double get_total_edp() const { return total_edp; }
    
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
    size_t mac_filter_tile_size;
    size_t mac_output_tile_size; 
    size_t l1_input_tile_size; 
    size_t l1_filter_tile_size;
    size_t l1_output_tile_size; 
    size_t l2_input_tile_size; 
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
    double l2_energy;
    double dram_energy;
    double total_energy;
    // Utilization
    float s0_utilization;
    float l1_utilization;
    float s1_utilization;
    float l2_utilization;
    float s2_utilization;
    // Cycle
    size_t mac_cycle;
    size_t l1_cycle;
    size_t l2_cycle;
    size_t dram_cycle;
    size_t total_cycle;
    // EDP
    double total_edp;
    // Requisites 
    const accelerator_t *accelerator;
    const mapping_table_t *mapping_table;
};

/* Energy reference */
class energy_ref_t {
public:
    energy_ref_t();
    ~energy_ref_t();
    // Eyeriss 45nm (pJ)
    float mac_operation = 0.075;

    // L1 Separated (24/448/48)
//    float l1_input_ingress = 0.05;
//    float l1_input_egress = 0.05;
//    float l1_filter_ingress = 0.94;
//    float l1_filter_egress = 0.94;
//    float l1_output_ingress = 0.10;
//    float l1_output_egress = 0.10;
    // L1 Shared (520B)
    float l1_input_ingress = 0.96;
    float l1_input_egress = 0.96;
    float l1_filter_ingress = 0.96;
    float l1_filter_egress = 0.96;
    float l1_output_ingress = 0.96;
    float l1_output_egress = 0.96;
    // L2 128 KB
    float l2_input_ingress = 13.5;
    float l2_input_egress = 13.5;
    float l2_filter_ingress = 13.5;
    float l2_filter_egress = 13.5;
    float l2_output_ingress = 13.5;
    float l2_output_egress = 13.5;

    float dram_ingress = 200;
    float dram_egress = 200;
    // Simba 45nm (pJ)
    //float mac_energy = 2.2;
    //float l1_input_energy = 0.81;
    //float l1_filter_energy = 1.45;
    //float l1_output_energy = 0.58;
    //float l2_shared_energy = 7.71;
    //float dram_energy = 440;
};

/* Cycle reference */
class cycle_ref_t {
public: 
    cycle_ref_t(); 
    ~cycle_ref_t();

    size_t mac_operation = 1;
    size_t l1_access = 1;
    size_t l2_access = 2;
    size_t dram_access = 30;
};

#endif
