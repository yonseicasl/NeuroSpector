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

enum class data_type_t {
    INPUT, WEIGHT, OUTPUT, SIZE
};

class mapping_table_t {
public:
    mapping_table_t();
    ~mapping_table_t();

    void put_val(parameter_t D, component_t U, unsigned num_);
    unsigned get_val(parameter_t D, component_t U);
    unsigned product(parameter_t D, component_t U);
    unsigned quotient(parameter_t D, component_t U);
    float divider(parameter_t D, component_t U);
    void print_stats();

    // Layer parameter values
    std::vector<unsigned> layer_vals; // KBPQCRS
    unsigned stride;
private:
    // Table(D(7) x U(6))
    unsigned D_size = static_cast<unsigned>(parameter_t::SIZE);
    unsigned U_size = static_cast<unsigned>(component_t::SIZE);
    std::vector<unsigned> table;
    std::vector<std::string> D_str;
    std::vector<std::string> U_str;
};

#endif 
