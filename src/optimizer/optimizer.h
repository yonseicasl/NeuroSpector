#ifndef __OPTIMIZER_H__
#define __OPTIMIZER_H__

#include <cfloat>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "accelerator.h"
#include "configs.h"
#include "enums.h"
#include "mapping_space.h"
#include "mapping_table.h"
#include "stats.h"

/* Optimizer */
class optimizer_t {
public:
    optimizer_t(const std::string& acc_cfg_path_, 
                const std::string& net_cfg_path_, 
                const bool is_fixed_,
                const unsigned num_threads_);                  
    ~optimizer_t();
    // Optimizer APIs
    void run_brute_force();                         // Run brute-force optimizing of all layers
    void run_brute_force(const unsigned idx_);      // Run brute-force optimizing of the target layer
    void run_2level_by_2level(const unsigned idx_);

private:
    // Mapping worker
    void brute_force_worker(const unsigned idx_, 
                            const unsigned tid_, 
                            const mapping_space_t& mapping_space_,
                            const component_t start_,
                            const component_t end_,
                            std::mutex& m_); 
    // Check each mapping table with the accelerator
    bool check_validity(const mapping_table_t& mapping_table_) const; 
    bool mac_validity(const mapping_table_t& mapping_table_) const;    
    bool s0_validity(const mapping_table_t& mapping_table_) const; 
    bool l1_validity(const mapping_table_t& mapping_table_) const;
    bool s1_x_validity(const mapping_table_t& mapping_table_) const; 
    bool s1_y_validity(const mapping_table_t& mapping_table_) const;
    bool l2_validity(const mapping_table_t& mapping_table_) const;
    bool s2_validity(const mapping_table_t& mapping_table_) const;
    // Variables & containers
    bool is_fixed;                                  // TODO: Dataflow: fixed (true) or flexible (false)
    unsigned D_size;                                // Mapping table column size
    unsigned U_size;                                // Mapping table row size
    unsigned num_levels;                            // # of existent levels
    accelerator_t *accelerator;                     // Target accelerator 
    std::string network_name;                       // DNN name
    std::vector<bool> exists;                       // Component exist bits from MAC to DRAM
    std::vector<mapping_table_t> mapping_tables;    // Mapping tables from the mapping configuration
    // For multi-threading
    unsigned num_threads;
    uint64_t global_total_cnt;
    uint64_t global_valid_cnt;
    std::vector<size_t> global_min_energy;
    std::vector<mapping_table_t> global_best_mapping_table;
    std::vector<std::vector<mapping_table_t>> global_similar_mapping_tables;
};

#endif
