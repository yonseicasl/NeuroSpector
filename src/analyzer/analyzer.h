#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <iostream>
#include <string>
#include <vector>

#include "hardware_types.h"
#include "accelerator.h"
#include "network.h"
#include "scheduling_table.h"
#include "version.h"

class analyzer_t{
public:
    // Construct an analyzer with file paths 
    analyzer_t(const std::string& accelerator_pth_,
               const std::string& dataflow_,
               const std::string& network_pth_,
               const std::string& batch_size_,
               const std::string& scheduling_table_pth_);
    
    // Construct an analyzer with pre-configured objects
    analyzer_t(const accelerator_t *accelerator_,
               const network_t    *network_);
    
    ~analyzer_t();

    /* Core Analysis Methods */
    // Execute a complete analysis including validity check, cost estimation, 
    // and result printing
    void run();
    // Initialize the analyzer state
    void init(); 
    // Initialize the analyzer state with a specific scheduling table
    void init(scheduling_table_t scheduling_table_);
    // Estimate accelerator's cost for a given scheduling table
    void estimate_cost();
    // Estimate the amount of data reuse between adjacent layers
    void estimate_cross_layer_reuse(scheduling_table_t prev_table_,
                                    metric_t      metric_);

    /*--- Validation Methods ---*/
    bool verify_constraints();
    bool verify_spatial_constraints();
    bool verify_temporal_constraints();

    /*--- Cost Analyzing Methods ---*/
    // Get accelerator's target cost (Energy or Cycle)
    float get_total_cost(metric_t metric_);
    // Get accelerator's target cost (Energy or Cycle) in a specific level
    float get_target_level_cost(unsigned idx_, metric_t metric_);

    /*--- Component-specific Query Methods ---*/
    // Get factorzation degrees of targeted level
    unsigned get_target_level_factorization(unsigned idx_);
    // Get Num. active chips
    unsigned get_num_active_chips();
    // Get tile size of the row of scheduling table 
    unsigned get_tile_size(component_t comp_, data_t data_type_);
    // Get tile-granular access count of the row of scheduling table 
    unsigned get_access_count(component_t comp_, data_t data_type_);
    
    /*--- Output Methods ---*/
    void print_results();
    void print_results(std::ofstream &output_file_);

private:
    /*--- Initialize Hardware Component Methods ---*/
    void init_mac_array();
    void init_local_buffer();
    void init_pe_array();
    void init_global_buffer();
    void init_multi_chips();
    void init_dram();

    /*--- Verification Methods ---*/
    bool check_hardware_constraints() ;
    bool check_network_constraints() ;
    bool check_user_constraint(component_t comp_, arr_cnst_t cnst_);
    bool check_user_constraint(component_t comp_, std::vector<unsigned> cnst_);

    /*--- Update Methods ---*/
    void update_tile_size();            // Update tile size  
    void update_access_count();         // Update access count
    void update_active_components();    // Update active components
    void update_cycle();                // Update accelerator's cycle 
    void update_energy();               // Update accelerator's energy
    void update_runtime_power();        // Update accelerator's power
    // Update input tile size to allocate (or send)
    unsigned update_input_tile_size(component_t buffer_,
                                    direction_t direction_);
    // Update weight tile size to allocate (or send)
    unsigned update_weight_tile_size(component_t buffer_,
                                     direction_t direction_);
    // Update output tile size to allocate (or send)
    unsigned update_output_tile_size(component_t buffer_,
                                     direction_t direction_);
    // Update tile-granular input access count
    unsigned update_input_access_count(unsigned idx_, 
                                       unsigned value_);
    // Update tile-granular weight access count
    unsigned update_weight_access_count(unsigned idx_, 
                                        unsigned value_);
    // Update tile-granular output access count
    unsigned update_output_access_count(unsigned idx_, 
                                        unsigned value_,
                                        operation_t oper_);

    /*--- Misc Methods ---*/
    // Update irrelevant mapping values with stationary data
    unsigned get_irrelevant_mapping_value(unsigned row_idx_,
                                          data_t stationary_data_);
    // Update relevant mapping values with stationary data
    unsigned get_relevant_mapping_value(unsigned row_idx_,
                                        data_t stationary_data_);
    // Get factorziation degrees in target component level
    unsigned get_factorization_degrees(unsigned idx_);
    // Compute overlapped data size between the previous and current tables
    unsigned compute_overlapped_size(scheduling_table_t prev_table_);
    // Handle exception case where given dataflow is ineffective 
    dataflow_t handle_dataflow_exception_case(unsigned idx_, 
                                              dataflow_t df_, 
                                              bypass_t bp_);
    // Clarify the bypassed effect on reported data
    void update_bypassed_data(); 

    accelerator_t      *accelerator;
    network_t          *network;
    scheduling_table_t scheduling_table;

    dataflow_t dataflow = dataflow_t::NONE;

    // Total number of active components
    unsigned num_active_macs  = 1;
    unsigned num_active_pes   = 1;
    unsigned num_active_chips = 1;
    // Component tile size transferred to upper level
    tile_size_t   mac_tile_size_send;
    tile_size_t    lb_tile_size_send;
    tile_size_t    gb_tile_size_send;
    tile_size_t  dram_tile_size_send;
    // Component tile size allocated in buffer
    tile_size_t  mac_tile_size_alloc;
    tile_size_t   lb_tile_size_alloc;
    tile_size_t   gb_tile_size_alloc;
    tile_size_t dram_tile_size_alloc;
    // Component access count
    access_count_t   lb_access_count;
    access_count_t   gb_access_count;
    access_count_t dram_access_count;

    // Dynamic energy 
    float    total_energy         = 0;
    float    mac_energy           = 0;
    float    local_buffer_energy  = 0;
    float    lb_energy_upper      = 0;
    float    lb_energy_lower      = 0;
    float    global_buffer_energy = 0;
    float    gb_energy_upper      = 0;
    float    gb_energy_lower      = 0;
    float    dram_energy          = 0;
    // Static energy 
    float    total_static_energy  = 0;
    float    mac_static           = 0;
    float    local_buffer_static  = 0;
    float    global_buffer_static = 0;
    float    dram_static          = 0;
    // Cycle
    float    total_cycle          = 0;
    float    on_chip_cycle        = 0;
    float    mac_cycle            = 0;
    float    local_buffer_cycle   = 0;
    float    global_buffer_cycle  = 0;
    float    dram_cycle           = 0;
    // Power
    float    average_power_acc           = 0;
    float    average_power_on_chip       = 0;
    float    mac_power                   = 0;  
    float    local_buffer_power          = 0;
    float    global_buffer_power         = 0;
    float    dram_power                  = 0;
    float    mac_dynamic_power           = 0;  
    float    local_buffer_dynamic_power  = 0;
    float    global_buffer_dynamic_power = 0;
    float    dram_dynamic_power          = 0;
    float    mac_static_power            = 0;  
    float    local_buffer_static_power   = 0;
    float    global_buffer_static_power  = 0;
    float    dram_static_power           = 0;

    // The number of spatially arranged component being used
    arr_size_t  macs_activated;
    arr_size_t   pes_activated;
    arr_size_t chips_activated;
    // Physical limit
    arr_size_t  macs_capacity;
    arr_size_t   pes_capacity;
    arr_size_t chips_capacity;
    buffer_size_t lb_capacity;
    buffer_size_t gb_capacity;
    // User constraints
    arr_cnst_t  macs_constraint;
    arr_cnst_t   pes_constraint;
    arr_cnst_t chips_constraint;
    // Radix degree
    rdx_degree_t  macs_radix;
    rdx_degree_t   pes_radix;
    rdx_degree_t chips_radix;

    // Whether buffer type is shared or sepearted
    bool is_lb_shared;
    bool is_gb_shared;
    // Whether buffer exist
    bool is_lb_exist = true;
    bool is_gb_exist = true;
    // Bypass
    bypass_t lb_bypass;
    bypass_t lb_separate;
    bypass_t gb_bypass;
    bypass_t gb_separate;
    // bitwidth
    bitwidth_t   lb_bitwidth;
    bitwidth_t   gb_bitwidth;
    // bitwidth_t dram_bitwidth;
    float        dram_bitwidth;
    // subwidth 
    bitwidth_t   lb_subwidth;
    bitwidth_t   gb_subwidth;

    unsigned cluster_size;  // Cluster means a group that makes one output data
    unsigned num_clusters_mac;  // Number of clusters in PE array
    unsigned num_clusters_pe;  // Number of clusters in PE array
    unsigned num_clusters_chip;  // Number of clusters in PE array

    // Unit access energy
    float            mac_unit_energy;
    unit_cost_t       lb_unit_energy;
    unit_cost_t       gb_unit_energy;
    unit_cost_t     dram_unit_energy;
    // Unit access cycle
    float             mac_unit_cycle;
    unit_cost_t        lb_unit_cycle;
    unit_cost_t        gb_unit_cycle;
    unit_cost_t      dram_unit_cycle;
    // Unit Static power
    float            mac_unit_static;
    float             lb_unit_static;
    float             gb_unit_static;
    float           dram_unit_static;
    // Memory mapping constraint
    std::vector<unsigned> lb_constraint;
    std::vector<unsigned> gb_constraint;
    std::vector<unsigned> dram_constraint;

    unsigned dram_idx;      // Off-chip DRAM index
    unsigned   gb_idx;      // GlobalBuffer index
    unsigned   lb_idx;      // LocalBuffer index
    unsigned   rg_idx;      // ResigsterFile index 

    /* extra variables for in-depth analyze*/
    float   gb_cycle_i;
    float   gb_cycle_w;
    float   gb_cycle_o;
    float dram_cycle_i;
    float dram_cycle_w;
    float dram_cycle_o;
};
#endif
