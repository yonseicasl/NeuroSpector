#ifndef __MAPPING_TABLE_H__
#define __MAPPING_TABLE_H__

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "utils.h"

enum class parameter_t { // D(7)
    K, B, P, Q, C, R, S, SIZE
};

enum class component_t { // U(6)
    L0, L1, X, Y, L2, CHIP, SIZE 
};

class mapping_table_t {
public:
    mapping_table_t();
    ~mapping_table_t();

    void put_val(parameter_t D, component_t U, unsigned num_);
    unsigned get_val(parameter_t D, component_t U);
    unsigned product(parameter_t D, component_t U, bool is_bypass_L1, bool is_bypass_L2);
    unsigned quotient(parameter_t D, component_t U, bool is_bypass_L1, bool is_bypass_L2);
    float divider(parameter_t D, component_t U, bool is_bypass_L1, bool is_bypass_L2);
    void print_stats();

    // Layer parameter values
    std::vector<unsigned> layer_vals; // KBPQCRS
    unsigned stride;
    std::vector<bool> U_exists;

private:
    // Table(D(7) x U(6))
    unsigned D_size = static_cast<unsigned>(parameter_t::SIZE);
    unsigned U_size = static_cast<unsigned>(component_t::SIZE);
    std::vector<unsigned> table;
    std::vector<std::string> D_str;
    std::vector<std::string> U_str;
};

#endif 
