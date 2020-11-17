#ifndef __MAPPING_TABLE_H__
#define __MAPPING_TABLE_H__

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "stats.h"
#include "utils.h"

enum class parameter_t { // D(7)
    K, B, P, Q, C, R, S, SIZE
};

enum class component_t { // U(6)
    MAC, L1, X, Y, L2, DRAM, SIZE 
};

class mapping_table_t {
public:
    mapping_table_t();
    ~mapping_table_t();
    void print_stats();
    void put_val(parameter_t D, component_t U, unsigned num_);
    unsigned get_val(parameter_t D, component_t U);
    unsigned get_product(parameter_t D, component_t U);
    unsigned get_reverse_product(parameter_t D, component_t U);
    void update_dram_row();
    void update_tile_size(); 
    void update_noc_info();
    void update_access_cnts();
    void update() { update_dram_row(); update_tile_size(); update_access_cnts(); }
    void print_tile_size();
    void print_access_cnts();
    // for optimizer
//    unsigned get_row_product(component_t U);
//    void expand(component_t U, unsigned max_expanded);
//    void alpha_expand(component_t U, unsigned max_expanded);
//    void beta_expand(component_t U, unsigned max_expanded);
//    void gamma_expand(component_t U, unsigned max_expanded);
    // Layer parameter values
    std::vector<unsigned> layer_vals; // KBPQCRS
    unsigned stride;
    // Information
    bool noc_exists = 1;
    tile_size_t tile_sizes;
    noc_info_t noc_info;
    access_cnts_t L1_access_cnts;
    access_cnts_t L2_access_cnts;
    access_cnts_t noc_access_cnts;
    access_cnts_t DRAM_access_cnts;
    // Dataflow
    dataflow_t L0_dataflow;
    dataflow_t L1_dataflow;
    dataflow_t L2_dataflow;
private:
    // Table(D(7) x U(6))
    unsigned D_size = static_cast<unsigned>(parameter_t::SIZE);
    unsigned U_size = static_cast<unsigned>(component_t::SIZE);
    std::vector<unsigned> degrees;
    std::string D_str = "KBPQCRS";
    std::vector<std::string> U_str;
};

#endif 
