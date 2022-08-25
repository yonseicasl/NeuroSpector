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
    bool check_validity() ;
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
    unsigned get_access_count(unsigned idx_, operation_t op_, data_t data_type_);

    struct tile_size_t {
        unsigned input;
        unsigned weight;
        unsigned output;
    };
    struct access_count_t {
        unsigned input_rd;
        unsigned weight_rd;
        unsigned output_rd;
        unsigned output_wt;
    };
    struct unit_cost_t {
        float input;
        float weight;
        float output;
    };
    struct buffer_size_t {
        float input;
        float weight;
        float output;
        float shared;
    };
    struct arr_size_t {
        unsigned dim_x;
        unsigned dim_y;
    };
    struct bypass_t {
        bool input;
        bool weight;
        bool output;
    };
private:
    void init_mac_array();
    void init_local_buffer();
    void init_pe_array();
    void init_global_buffer();
    void init_multi_chips();
    void init_dram();
    // Check hardware constraints
    bool check_hardware_constraints() ;
    // Check parameter constraints
    bool check_network_constraints() ;
    // Update tile size
    void update_tile_size();
    // Update tile-granular access count
    void update_access_count();
    // Update num. activated components
    void update_active_components();
    // Update accelerator's cycle
    void update_cycle();
    // Update accelerator's energy (static + dynamic)
    void update_energy();

    // Update input tile size to allocate (or send)
    unsigned update_input_tile_size(buffer_t buffer_,
                                    direction_t direction_);
    // Update weight tile size to allocate (or send)
    unsigned update_weight_tile_size(buffer_t buffer_,
                                     direction_t direction_);
    // Update output tile size to allocate (or send)
    unsigned update_output_tile_size(buffer_t buffer_,
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

    unsigned access_count = 0;
    unsigned write_access_count = 0;
    unsigned read_access_count  = 0;
    
    unsigned component_idx_curr = 0;
    unsigned component_idx_lower= 0;
    unsigned component_idx_upper= 0;
    
    dataflow_t dataflow = dataflow_t::NONE;

    // Total number of active components
    unsigned num_active_macs  = 1;
    unsigned num_active_pes   = 1;
    unsigned num_active_chips = 1;
    // Component utilization
    float       mac_arr_util;
    float  local_buffer_util;
    float        pe_arr_util;
    float global_buffer_util;
    float      chip_arr_util;
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

    // Total dynamic energy 
    float    total_energy         = 0;
    float    mac_energy           = 0;
    float    local_buffer_energy  = 0;
    float    lb_energy_upper      = 0;
    float    lb_energy_lower      = 0;
    float    global_buffer_energy = 0;
    float    gb_energy_upper      = 0;
    float    gb_energy_lower      = 0;
    float    dram_energy          = 0;
    // Total static energy 
    float    total_static_energy  = 0;
    float    mac_static           = 0;
    float    local_buffer_static  = 0;
    float    global_buffer_static = 0;
    float    dram_static          = 0;
    // Total cycle
    float    total_cycle          = 0;
    float    mac_cycle            = 0;
    float    local_buffer_cycle   = 0;
    float    global_buffer_cycle  = 0;
    float    dram_cycle           = 0;

    arr_size_t  macs_actived;
    arr_size_t   pes_actived;
    arr_size_t chips_actived;
    // Physical limit
    arr_size_t  macs_capacity;
    arr_size_t   pes_capacity;
    arr_size_t chips_capacity;
    buffer_size_t lb_capacity;
    buffer_size_t gb_capacity;
    // is buffer type is shared or sepearted
    bool is_lb_shared;
    bool is_gb_shared;
    // is buffer exist
    bool is_lb_exist;
    bool is_gb_exist;
    // Bypass
    bypass_t lb_bypass;
    bypass_t gb_bypass;
    // Bandwidth
    float   lb_bandwidth;
    float   gb_bandwidth;
    float dram_bandwidth;
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
    unit_cost_t       lb_unit_static;
    unit_cost_t       gb_unit_static;
    unit_cost_t     dram_unit_static;

    unsigned dram_idx; 
    unsigned   gb_idx; 
    unsigned   lb_idx;
};
#endif
