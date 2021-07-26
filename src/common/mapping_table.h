#ifndef __MAPPING_TABLE_H__
#define __MAPPING_TABLE_H__

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "enums.h"
#include "utils.h"

/* mapping table */
class mapping_table_t {
public:
    mapping_table_t(const bool is_grouped_,
                    const std::vector<bool>& exists_, 
                    const std::string& layer_name_,
                    const std::vector<unsigned>& layer_values_, 
                    const unsigned stride_);
    ~mapping_table_t();
    // Copy constructor
    mapping_table_t(const mapping_table_t& rhs_);
    // Initialization APIs
    // TODO: REDUCTION
    void init_degrees(const std::vector<unsigned>& src_);
    // Mapping table common APIs
    void print_stats() const;
    void print_csv() const;
    unsigned get_degree(const parameter_t D, const component_t U) const;
    unsigned get_stride() const;
    size_t get_product(const parameter_t D, const component_t U) const;
    size_t get_column_product(const component_t U) const;
    size_t get_temporal_product(const parameter_t D, const component_t U) const;
    size_t get_iteration(const component_t U) const;
    size_t get_iteration(const parameter_t D, const component_t U) const;
    size_t get_num_macs() const;
    // TODO: REDUCTION
    size_t get_input_tile_size(const component_t U) const;
    size_t get_input_height_tile_size(const component_t U) const;
    size_t get_input_width_tile_size(const component_t U) const;
    size_t get_filter_tile_size(const component_t U) const;
    size_t get_output_tile_size(const component_t U) const;
    std::string get_layer_name();
    void update_dram_row();
    // Mapping table APIs for the optimizer
    void put_column_degrees(const parameter_t D, 
                            const std::vector<unsigned>& container_, 
                            const component_t start_, 
                            const component_t end_);
    // TODO: REDUCTION
    void put_column_spatial_first_degrees(const parameter_t D, 
                                          const std::vector<unsigned>& container_);
    void put_column_spatial_later_degrees(const parameter_t D, 
                                          const std::vector<unsigned>& container_);
    void put_column_temporal_degrees(const parameter_t D, 
                                     const std::vector<unsigned>& container_);
    void swap_degrees(const std::vector<unsigned>& degrees_);
    std::vector<unsigned> get_degrees() const; 
    std::vector<unsigned> get_layer_values() const;
    std::vector<unsigned> get_row_degrees(const component_t U) const;
    // S2 x L2 -> L1
    void leverage(const dataflow_t df_);

private:
    bool is_grouped;                          // For group conv
    unsigned D_size;                          // Mapping table column size
    unsigned U_size;                          // Mapping table row size
    unsigned stride;                          // Layer stride
    std::vector<bool> exists;                 // Component exist bits from MAC to DRAM
    std::vector<unsigned> layer_values;       // Layer parameter values (GKBPQCSR)
    std::vector<unsigned> degrees;            // Mapping degrees
    std::string layer_name;                 
    std::string D_str = "GKBPQCSR";
    std::string U_str = "MAC [T]S0  [S]L1  [T]S1_X[S]S1_Y[S]L2  [T]S2  [S]DRAM   ";
    std::string csv_str = " MAC,  S0,  L1,S1_X,S1_Y,  L2,  S2,DRAM,";
};

#endif 
