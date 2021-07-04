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

#define L1_THRESHOLD 0
#define S1_THRESHOLD 0
#define L2_THRESHOLD 0

/* Optimizer */
class optimizer_t {
public:
    optimizer_t(const std::string& acc_cfg_path_, 
                const std::string& net_cfg_path_, 
                const bool is_fixed_);                  
    virtual ~optimizer_t();
    // Optimizer APIs
    virtual void run() = 0;                     // Run optimizing of all layers
    virtual void run(const unsigned idx_) = 0;  // Run optimizing of the target layer
    virtual void print_stats() = 0;             // Print stats
    virtual void print_csv() = 0;               // Print csv

protected:
    // Initialze dataflows
    void init_dataflows();
    // Check each mapping table with the accelerator
    bool check_all_validity(const mapping_table_t& mapping_table_) const; 
    bool check_validity(const component_t U, const mapping_table_t& mapping_table_) const; 
    bool mac_validity(const mapping_table_t& mapping_table_) const;    
    bool s0_validity(const mapping_table_t& mapping_table_) const; 
    bool l1_validity(const mapping_table_t& mapping_table_) const;
    bool s1_x_validity(const mapping_table_t& mapping_table_) const; 
    bool s1_y_validity(const mapping_table_t& mapping_table_) const;
    bool l2_validity(const mapping_table_t& mapping_table_) const;
    bool s2_validity(const mapping_table_t& mapping_table_) const;
    // Variables & containers
    bool is_fixed;                                      // Dataflow: fixed (true) or flexible (false)
    unsigned D_size;                                    // Mapping table column size
    unsigned U_size;                                    // Mapping table row size
    unsigned num_levels;                                // # of existent levels
    accelerator_t *accelerator;                         // Target accelerator 
    std::string network_name;                           // DNN name
    std::vector<bool> exists;                           // Component exist bits from MAC to DRAM
    std::vector<mapping_table_t> mappings;              // Mapping tables from the mapping configuration
    std::vector<unsigned> l1_dataflows;                 // L1 dataflow(s)
    std::vector<unsigned> l2_dataflows;                 // L2 dataflow(s)
};

/* Brute-force */
class brute_force_t : public optimizer_t {
public:
    brute_force_t(const std::string& acc_cfg_path_, 
                  const std::string& net_cfg_path_,
                  const opt_type_t opt_type_, 
                  const unsigned num_threads_,
                  const bool is_fixed_);                
    ~brute_force_t();
    // Optimizer APIs
    void run();
    void run(const unsigned idx_);
    void print_stats();
    void print_csv();                   

private:
    // Optimizer private functions
    void reset(const unsigned idx_);                                // Reset for the next dataflow or layer
    void engine(const unsigned idx_,
                const dataflow_t l1_dataflow_, 
                const dataflow_t l2_dataflow_);                     // Brute-force engine for multi-threading
    void sync_and_update(const dataflow_t l1_dataflow_,
                         const dataflow_t l2_dataflow_);            // Sync and update
    void energy_worker(const unsigned tid_, 
                       const mapping_table_t& init_mapping_,
                       const mapping_space_t& mapping_space_,
                       const dataflow_t l1_dataflow_,
                       const dataflow_t l2_dataflow_,
                       std::mutex& m_);                             // Energy worker
    void cycle_worker(const unsigned tid_, 
                      const mapping_table_t& init_mapping_,
                      const mapping_space_t& mapping_space_,
                      const dataflow_t l1_dataflow_,
                      const dataflow_t l2_dataflow_,
                      std::mutex& m_);                              // Cycle worker
    void edp_worker(const unsigned tid_, 
                    const mapping_table_t& init_mapping_,
                    const mapping_space_t& mapping_space_,
                    const dataflow_t l1_dataflow_,
                    const dataflow_t l2_dataflow_,
                    std::mutex& m_);                                // EDP worker
    // Variables & containers
    const opt_type_t opt_type;                                      // Opt type: b-f-energy / b-f-cycle / b-f-edp
    const unsigned num_threads;                                     // # of threads
    uint64_t total_cnt;                                             // Total # of mapping space
    uint64_t valid_cnt;                                             // Total valid counts (global)
    std::vector<std::pair<double, mapping_table_t>> best_mappings;  // Best mapping (with stat) per thread (global)
    std::vector<std::vector<mapping_table_t>> similar_mappings;     // Similar mappings per thread (global)
    // Final
    double final_best_stat;                                         // Final best stat (energy, cycle, or edp)
    std::vector<mapping_table_t> final_best_mappings;               // Final best mappings 
    std::vector<dataflow_t> final_best_dataflows;                   // Final best dataflows (L1 and L2)
};

/* T-S Bottom-up */
class bottom_up_t : public optimizer_t {
public:
    bottom_up_t(const std::string& acc_cfg_path_, 
                const std::string& net_cfg_path_, 
                const bool is_fixed_);                
    ~bottom_up_t();
    // Optimizer APIs
    void run();
    void run(const unsigned idx_);
    void print_stats();
    void print_csv();

private:
    // Optimizer private functions
    void reset(const unsigned idx_);                         // Reset for the next dataflow or layer
    void engine(const unsigned idx_,
                const dataflow_t l1_dataflow_, 
                const dataflow_t l2_dataflow_);              // bottom_up engine
    void update(const dataflow_t l1_dataflow_,
                const dataflow_t l2_dataflow_);              // Update final things
    void worker(const unsigned seq_,
                const mapping_table_t& init_mapping_,
                const mapping_space_t& mapping_space_,
                const dataflow_t l1_dataflow_,
                const dataflow_t l2_dataflow_,
                std::vector<mapping_table_t>& rtn_);        // bottom_up worker (seq_: 0 (L2-DRAM) - 1 (L1-L2) - 2 (MAC-L1))
    // Variables & containers
    component_t start_component;                            // Current start component
    component_t end_component;                              // Current end component
    std::vector<unsigned> used_levels;                      // # of the existent levels (not skipped levels)
    std::vector<unsigned> top_k;                            // Top k for each t-s level
    std::vector<uint64_t> total_cnt;                        // Total # of mapping space: first (1) - second (top_k[0]) - third (top_k[0] * top_k[1])
    std::vector<uint64_t> valid_cnt;                        // Total valid counts      : first (1) - second (top_k[0]) - third (top_k[0] * top_k[1])
    std::vector<mapping_table_t> best_mappings;
    // Final
    unsigned seq_0_top_k;
    unsigned seq_1_top_k;
    unsigned seq_2_top_k;
    unsigned final_best_idx;                                // Final best mapping's index
    double final_best_energy;                               // Final best mapping's energy
    std::vector<mapping_table_t> final_best_mappings;       // Final best mappings
    std::vector<dataflow_t> final_best_dataflows;           // Final best dataflows (L1 and L2)
};

#endif
