#ifndef __SCHEDULING_TABLE_H__
#define __SCHEDULING_TABLE_H__

#include <cassert>

#include "accelerator.h"
#include "network.h"

typedef std::vector<unsigned>              table_row_t;
typedef std::vector<std::vector<unsigned>> table_rows_t;

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
    std::string get_dataflow() const;                               // Get dataflow of target component as string
    std::string get_component_name(unsigned idx_) const;            // Get target component name 
    reuse_t     get_component_type(unsigned idx_) const;            // Get reuse type of target component 

    unsigned    get_single_row_correlation_product(int idx_, correlation_t correlation_);
    unsigned    get_correlation_product(int idx_, correlation_t correlation_);
    unsigned    get_column_wise_product(parameter_t param_,
                                     unsigned begin_, unsigned end_);
    unsigned    get_temporal_row_wise_product(int begin_, int end_); // Get a result that multiplying all values existed in temporal rows
    unsigned    get_row_product(int idx_);                           // Get a result that multiplying all values existed in a row

    unsigned    get_num_clusters(data_t dtype_, component_t comp_); 
    unsigned    get_num_clusters();
    unsigned    get_num_utilized_pe();
    unsigned    get_num_valid_temp_rows();                           // Counts the non-virtual buffer component
    unsigned    get_cluster_size();

    table_row_t get_row_values(unsigned idx_) const;       // Get mapping values of a component level
    std::vector<unsigned> get_row_wise_product(int begin_, int end_);

    unsigned get_mapping_value(unsigned row_, unsigned col_) const;   // Get mapping value at (row, col) 
    unsigned get_layer_index();                                       // Get current layer's index
    std::vector<unsigned> get_layer_parameters();                     // Get current layer's DNN parameters

    float get_num_mac_operations();                                   // Get total number of MAC operations in a layer 

    bool is_virtual(unsigned idx_);                                   // Check the component is virtual
    bool is_skippable(unsigned idx_);                                 // Check the component is skippable
    bool is_spatial(unsigned idx_);                                   // Check the component is spatial component
    
    void load_dnn_layer(unsigned idx_);                               // Update DRAM mapping values to layer parameters
    void clear_set_of_rows(unsigned begin_, unsigned end_ );          // Set to all mapping values in targeted rows to 1
    void update_set_of_rows(unsigned begin_, unsigned end_,           // Update mapping values in targeted rows
                            std::vector<std::vector<unsigned>> mapping_values_set_);
    void update_set_of_rows_spatial(unsigned begin_, unsigned end_,   // Update mapping values in spatial rows
                            std::vector<std::vector<unsigned>> mapping_values_set_);
    void update_set_of_rows_temporal(unsigned begin_, unsigned end_,  // Update mapping values in temporal rows
                            std::vector<std::vector<unsigned>> mapping_values_set_);
    void update_row(unsigned component_pos_,                          // Update mapping values in targeted row
                    std::vector<unsigned> mapping_values_);
    void update_dataflow(std::vector<dataflow_t>);                    // Update targeted row's dataflow

    void fill_out_mapping_values(const parser_t parser_);             // Fill out scheduling table based on scheduling table file
    bool operator!=(const scheduling_table_t& scheduling_table_);

    void split(const component_t ref_comp_);
    void clear_split_info();
    table_row_t get_temp_row();                                       // Get mapping values in temporary row
    component_t get_seperate_point();

private:
    void add_virtual_component(reuse_t component_type_); 
    void add_virtual_component(component_t component_); 
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
    std::vector<reuse_t>      row_types;       // row's types 
    std::vector<dataflow_t>   row_dataflows;   // row's dataflow 
    std::vector<unsigned>     row_index;       // row's determined
    std::vector<bool>         row_skippable;   // row's virtual flag
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
