#include "mapping_table.h"

static handler_t handler;

mapping_table_t::mapping_table_t()
    : stride(1) {
    layer_vals.assign(7, 1);
    table.assign(D_size * U_size, 1);
    D_str.push_back("K");
    D_str.push_back("B");
    D_str.push_back("P");
    D_str.push_back("Q");
    D_str.push_back("C");
    D_str.push_back("R");
    D_str.push_back("S");
    U_str.push_back(" L0 (S)");
    U_str.push_back(" L1 (T)");
    U_str.push_back("  X (S)");
    U_str.push_back("  Y (S)");
    U_str.push_back(" L2 (T)");
    U_str.push_back("CHIP(S)");
}

mapping_table_t::~mapping_table_t() {
}

void mapping_table_t::put_val(parameter_t D, component_t U, unsigned num_) {
    unsigned column = static_cast<unsigned>(D);
    unsigned row = static_cast<unsigned>(U);
    table.at(column + 7 * row) = num_;
}

unsigned mapping_table_t::get_val(parameter_t D, component_t U) {
    unsigned column = static_cast<unsigned>(D);
    unsigned row = static_cast<unsigned>(U);
    return table.at(column + 7 * row);
}

unsigned mapping_table_t::product(parameter_t D, component_t U) {
    component_t next_U = static_cast<component_t>(static_cast<unsigned>(U) + 1);
    unsigned column = static_cast<unsigned>(D);
    unsigned product = 1;
    for(const auto &u : enum_range<component_t>(next_U)) {
        unsigned row  = static_cast<unsigned>(u);
        product *= table.at(column + 7 * row);
    }
    return product;
}

unsigned mapping_table_t::quotient(parameter_t D, component_t U) {
    float d_val = layer_vals.at(static_cast<unsigned>(D));  
    return round(d_val / product(D, U));
}

float mapping_table_t::divider(parameter_t D, component_t U) {
    float d_val = layer_vals.at(static_cast<unsigned>(D));  
    return d_val / product(D, U);
}

void mapping_table_t::print_stats() {
    component_t last_component = static_cast<component_t>(U_size);
    parameter_t last_parameter = static_cast<parameter_t>(D_size);
    std::cout << "  U \\ D  |  ";
    for(const auto &d : enum_range<parameter_t>(last_parameter)) {
        std::cout << D_str.at(static_cast<unsigned>(d)) << "("
                  << std::setw(3) << layer_vals.at(static_cast<unsigned>(d)) << ") ";
    }
    std::cout << std::endl;
    handler.print_line(60);
    for(const auto &u : enum_range<component_t>(last_component)) {
        std::cout << U_str.at(static_cast<unsigned>(u)) << "  |"; 
        for(const auto &d : enum_range<parameter_t>(last_parameter)) {
            std::cout << std::setw(7) << get_val(d, u);
        }
        std::cout << std::endl;
    }
}
