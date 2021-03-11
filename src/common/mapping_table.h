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
    mapping_table_t(const std::vector<bool>& exists_, 
                    const std::string& layer_name_,
                    const std::vector<unsigned>& layer_values_, 
                    const unsigned stride_);
    ~mapping_table_t();
    // Initialization APIs
    void init_degrees(const std::vector<unsigned>& src_);
    // Mapping table common APIs
    void print_stats() const;
    void print_csv() const;
    unsigned get_degree(const parameter_t D, const component_t U) const;
    unsigned get_stride() const;
    size_t get_product(const parameter_t D, const component_t U) const;
    size_t get_iteration(const component_t U) const;
    size_t get_num_macs() const;
    size_t get_input_tile_size(const component_t U) const;
    size_t get_filter_tile_size(const component_t U) const;
    size_t get_output_tile_size(const component_t U) const;
    std::string get_layer_name();
    void update_dram_row();
    // Mapping table APIs for the optimizer
    void put_column_degrees(const parameter_t D, 
                            const std::vector<unsigned>& container_, 
                            const component_t start_, 
                            const component_t end_);
    void swap_degrees(const std::vector<unsigned>& degrees_);
    std::vector<unsigned> get_degrees() const; 
    std::vector<unsigned> get_layer_values() const;
    std::vector<unsigned> get_l2_degrees() const;

private:
    unsigned D_size;                        // Mapping table column size
    unsigned U_size;                        // Mapping table row size
    unsigned stride;                        // Layer stride
    std::vector<bool> exists;               // Component exist bits from MAC to DRAM
    std::vector<unsigned> layer_values;     // Layer parameter values (KBPQCSR)
    std::vector<unsigned> degrees;          // Mapping degrees
    std::string layer_name;                 
    std::string D_str = "KBPQCSR";
    std::string U_str = "MAC [T]S0  [S]L1  [T]S1_X[S]S1_Y[S]L2  [T]S2_X[S]S2_Y[S]DRAM   ";
    std::string csv_str = " MAC,  S0,  L1,S1_X,S1_Y,  L2,S2_X,S2_Y,DRAM,";
};

#endif 
