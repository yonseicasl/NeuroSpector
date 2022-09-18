#ifndef __SCHEDULING_TABLE_H__
#define __SCHEDULING_TABLE_H__

#include <cassert>

#include "accelerator.h"
#include "network.h"

class scheduling_table_t {
public:
    scheduling_table_t();
    scheduling_table_t(accelerator_t *accelerator_,
                       network_t *network_);
    // Constructor for analyze mode
    scheduling_table_t(accelerator_t *accelerator_,
                       network_t *network_,
                       const std::string& scheduling_table_pth_);
    ~scheduling_table_t();
    // Copy constructor
    scheduling_table_t(const scheduling_table_t &scheduling_table_);      
    void init();                                                    // Init. Scheduling table
    void init_table_rows();                                         // Init. # table rows
    void init_mapping_values();                                     // Init. Mapping values
    void print_stats();                                             // Print scheduling table
    void print_stats(std::ofstream &output_file_);                  // Print scheduling table

    unsigned    get_above_buffer_pos(unsigned pos_) const;          // Get buffer index one level above 
    unsigned    get_below_buffer_pos(unsigned pos_) const;          // Get lower temporal level's index
    unsigned    get_above_spatial_level_pos(unsigned pos_) const;   // Get upper spatial-X level index
    unsigned    get_num_rows() const;                               // Get # rows of scheduling table
    unsigned    get_dataflow_irrelevant_params_product(int idx_);   // Get product of dataflow irrelevant parameters 
    bool*       get_bypass(unsigned idx_) const;
    dataflow_t  get_dataflow(unsigned idx_) const;                  // Get dataflow of target component
    std::string get_component_name(unsigned idx_) const;            // Get target component name 
    reuse_t get_component_type(unsigned idx_) const;       // Get reuse type of target component 

    unsigned    get_correlation_product(int idx_, correlation_t correlation_);
    unsigned    get_column_wise_product(parameter_t param_,
                                     unsigned begin_, unsigned end_);
    unsigned    get_temporal_row_wise_product(int begin_, int end_); // Get a result that multiplying all values existed in temporal rows
    unsigned    get_row_product(int idx_);                           // Get a result that multiplying all values existed in a row
    std::vector<unsigned> get_row_values(unsigned idx_) const;       // Get mapping values of a component level
    std::vector<unsigned> get_row_wise_product(int begin_, int end_);

    unsigned get_mapping_value(unsigned row_, unsigned col_) const;   // Get mapping value at (row, col) 
    unsigned get_layer_index();                                       // Get current layer's index
    std::vector<unsigned> get_layer_parameters();                     // Get current layer's DNN parameters

    float get_num_mac_operations();                                   // Get total number of MAC operations in a layer 

    bool is_virtual(unsigned idx_);                                   // Check the component is virtual
    
    void load_dnn_layer(unsigned idx_);                               // Update DRAM mapping values to layer parameters
    void clear_set_of_rows(unsigned begin_, unsigned end_ );          // Set to all mapping values in targeted rows to 1
    void update_set_of_rows(unsigned begin_, unsigned end_,           // Update mapping values in targeted rows
                            std::vector<std::vector<unsigned>> mapping_values_set_);
    void update_row(unsigned component_pos_,                          // Update mapping values in targeted row
                    std::vector<unsigned> mapping_values_);
    void update_dataflow(std::vector<dataflow_t>);                    // Update targeted row's dataflow

    void fill_out_mapping_values(const parser_t parser_);             // Fill out scheduling table based on scheduling table file
    
    bool operator!=(const scheduling_table_t& scheduling_table_);

private:
    void add_virtual_component(reuse_t component_type_); 
    void update_mapping_value(unsigned dst_, unsigned val_); 

    accelerator_t         *accelerator;        // Target accelerator
    network_t             *network;            // Target network

    std::string           layer_name; 
    unsigned              layer_index;
    std::vector<unsigned> layer_parameters;
    float                 num_mac_operations;  // Total number of MAC operations in a layer

    unsigned              num_table_rows;      // # table rows
    unsigned              num_table_cols;      // # table cols
    std::vector<unsigned> mapping_values;      // Mapping_values
    std::vector<std::string>  row_names;       // row's names
    std::vector<reuse_t> row_types;       // row's types 
    std::vector<dataflow_t>   row_dataflows;   // row's dataflow 
    std::vector<unsigned>     row_index;       // row's determined
};

struct PartitioningInfo {
    float    cost = 0.0;
    bool     is_parallelized    = true;
    unsigned input_tile_size    = 0;
    unsigned input_access_count = 0;
    unsigned num_assigned_chips = 1;
    scheduling_table_t scheduling_table;
};

#endif 
