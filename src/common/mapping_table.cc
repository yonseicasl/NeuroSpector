#include "mapping_table.h"

static handler_t handler;

/* Mapping table */
mapping_table_t::mapping_table_t(const std::vector<bool>& exists_, 
                                 const std::string& layer_name_, 
                                 const std::vector<unsigned>& layer_values_, 
                                 const unsigned stride_)
    : D_size(static_cast<unsigned>(parameter_t::SIZE)),
      U_size(static_cast<unsigned>(component_t::SIZE)), 
      stride(stride_), 
      exists(exists_),
      layer_values(layer_values_),
      degrees(D_size * U_size, 1),
      layer_name(layer_name_) {
    if(layer_values.size() != D_size)
        handler.print_err(err_type_t::INVAILD, "# of layer values");
}

mapping_table_t::mapping_table_t(const mapping_table_t& rhs_) 
    : D_size(rhs_.D_size),
      U_size(rhs_.U_size), 
      stride(rhs_.stride), 
      exists(rhs_.exists),
      layer_values(rhs_.layer_values),
      degrees(rhs_.degrees), 
      layer_name(rhs_.layer_name) {
}

mapping_table_t::~mapping_table_t() {
}

// Initialization APIs
void mapping_table_t::init_degrees(const std::vector<unsigned>& src_) {
    // TODO: assign
    if(degrees.size() - D_size != src_.size())
        handler.print_err(err_type_t::INVAILD, "# of degrees");
    for(unsigned i = 0; i < src_.size(); i++) 
        degrees.at(i) = src_.at(i);
    return;
}

// Mapping table common APIs
void mapping_table_t::print_stats() const {
    handler.print_line(20, "*");
    std::cout << "# LAYER: " << layer_name << std::endl;
    handler.print_line(20, "*");
    std::cout << "# MAPPING TABLE" << std::endl;
    handler.print_line(63, "-");
    std::cout << "   U \\ D   |  ";
    for(unsigned column = 0; column < D_size; column++) {
        std::cout << D_str.at(column) << "("
                  << std::setw(3) << layer_values.at(column) << ") ";
    }
    std::cout << std::endl;
    handler.print_line(63, "-");
    for(unsigned row = 0; row < U_size; row++) {
        if(exists.at(row)) {
            if(row == U_size - 1)
                handler.print_line(63, "-");
            std::cout << std::setw(9) << U_str.substr(row * 7, 7) << "  |"; 
            for(unsigned column = 0; column < D_size; column++) {
                std::cout << std::setw(7) 
                          << get_degree(static_cast<parameter_t>(column), static_cast<component_t>(row));
            }
            std::cout << std::endl;
        }
    }
    handler.print_line(63, "-");
    return;
}

void mapping_table_t::print_csv() const {
    std::cout << layer_name;
    for(unsigned i = 0; i < layer_values.size(); i++)
        std::cout << "," << layer_values.at(i);
    std::cout << "," << stride << "\n" << std::endl;
    for(unsigned row = 1; row < U_size; row++) {
        if(exists.at(row)) {
            std::cout << csv_str.substr(row * 5, 5); 
            for(unsigned column = 0; column < D_size; column++) {
                std::cout << get_degree(static_cast<parameter_t>(column), static_cast<component_t>(row)) << ",";
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
    return;
}

unsigned mapping_table_t::get_degree(const parameter_t D, const component_t U) const {
    unsigned column = static_cast<unsigned>(D);
    unsigned row = static_cast<unsigned>(U);
    return degrees.at(column + D_size * row);
}

unsigned mapping_table_t::get_stride() const {
    return stride;
}

size_t mapping_table_t::get_product(const parameter_t D, const component_t U) const {
    unsigned column = static_cast<unsigned>(D);
    size_t product = 1; 
    for(unsigned row = 0; row < static_cast<unsigned>(U) + 1; row++) {
        product *= degrees.at(column + D_size * row);
    }
    return product;
}

size_t mapping_table_t::get_iteration(const component_t U) const {
    size_t product = 1;
    switch(U) {
        case component_t::L1: 
            for(unsigned column = 0; column < D_size; column++) {
                product *= get_degree(static_cast<parameter_t>(column), component_t::L1);
                product *= get_degree(static_cast<parameter_t>(column), component_t::L2);
                product *= get_degree(static_cast<parameter_t>(column), component_t::DRAM);
            }
            break;
        case component_t::L2: 
            for(unsigned column = 0; column < D_size; column++) {
                product *= get_degree(static_cast<parameter_t>(column), component_t::L2);
                product *= get_degree(static_cast<parameter_t>(column), component_t::DRAM);
            }
            break;
        case component_t::DRAM: 
            for(unsigned column = 0; column < D_size; column++) 
                product *= get_degree(static_cast<parameter_t>(column), component_t::DRAM);
            break;
        default: handler.print_err(err_type_t::INVAILD, "COMPONENT");
            break;
    }
    return product;
}

size_t mapping_table_t::get_num_macs() const {
    size_t product = 1;
    for(unsigned i = 0; i < degrees.size(); i++)
        product *= degrees.at(i);
    return product;
}

size_t mapping_table_t::get_input_tile_size(const component_t U) const {
    size_t tile_size = 1;
    tile_size *= get_product(parameter_t::B, U)
               * get_product(parameter_t::C, U)
               * ((get_product(parameter_t::P, U) - 1) * stride + get_product(parameter_t::S, U))
               * ((get_product(parameter_t::Q, U) - 1) * stride + get_product(parameter_t::R, U));
    return tile_size;
}

size_t mapping_table_t::get_filter_tile_size(const component_t U) const {
    size_t tile_size = 1;
    tile_size *= get_product(parameter_t::K, U)
               * get_product(parameter_t::C, U)
               * get_product(parameter_t::S, U)
               * get_product(parameter_t::R, U);
    return tile_size;
}

size_t mapping_table_t::get_output_tile_size(const component_t U) const {
    size_t tile_size = 1;
    tile_size *= get_product(parameter_t::B, U)
               * get_product(parameter_t::K, U)
               * get_product(parameter_t::P, U)
               * get_product(parameter_t::Q, U);
    return tile_size;
}

std::string mapping_table_t::get_layer_name() {
    return layer_name;
}

void mapping_table_t::update_dram_row() {
    // TODO: padding
    for(unsigned column = 0; column < D_size; column++) {
       unsigned val = layer_values.at(column) / get_product(static_cast<parameter_t>(column), component_t::L2);
       unsigned remainder = layer_values.at(column) % get_product(static_cast<parameter_t>(column), component_t::L2);
       if(remainder > 0)
           val++;
       degrees.at(column + D_size * static_cast<unsigned>(component_t::DRAM)) = val;
    }
    return;
}

// Mapping table APIs for the optimizer
void mapping_table_t::put_column_degrees(const parameter_t D, 
                                         const std::vector<unsigned>& container_, 
                                         const component_t start_, 
                                         const component_t end_) {
    unsigned exists_cnt = 0;
    for(unsigned row = static_cast<unsigned>(start_); row < static_cast<unsigned>(end_) + 1; row++) {
        if(row != 0 && exists.at(row)) {
            degrees.at(static_cast<unsigned>(D) + D_size * row) = container_.at(exists_cnt);
            exists_cnt++;
        }
    }
    return;
}

void mapping_table_t::put_column_spatial_first_degrees(const parameter_t D, 
                                                 const std::vector<unsigned>& container_) {
    unsigned exists_cnt = 0;
    if(exists.at(static_cast<unsigned>(component_t::S1_X))) {
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::S1_X)) = container_.at(exists_cnt);
        exists_cnt++;
    }
    if(exists.at(static_cast<unsigned>(component_t::S1_Y))) {
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::S1_Y)) = container_.at(exists_cnt);
        exists_cnt++;
    }
    if(exists.at(static_cast<unsigned>(component_t::DRAM))) 
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::DRAM)) = container_.at(exists_cnt);
    return;
}

void mapping_table_t::put_column_spatial_later_degrees(const parameter_t D, 
                                                 const std::vector<unsigned>& container_) {
    unsigned exists_cnt = 0;
    if(exists.at(static_cast<unsigned>(component_t::S1_X))) {
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::S1_X)) = container_.at(exists_cnt);
        exists_cnt++;
    }
    if(exists.at(static_cast<unsigned>(component_t::S1_Y))) {
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::S1_Y)) = container_.at(exists_cnt);
        exists_cnt++;
    }
    if(exists.at(static_cast<unsigned>(component_t::L2))) 
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::L2)) = container_.at(exists_cnt);
    return;
}

void mapping_table_t::put_column_temporal_degrees(const parameter_t D, 
                                                  const std::vector<unsigned>& container_) {
    unsigned exists_cnt = 0;
    if(exists.at(static_cast<unsigned>(component_t::L1))) {
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::L1)) = container_.at(exists_cnt);
        exists_cnt++;
    }
    if(exists.at(static_cast<unsigned>(component_t::L2))) {
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::L2)) = container_.at(exists_cnt);
        exists_cnt++;
    }
    if(exists.at(static_cast<unsigned>(component_t::DRAM))) 
        degrees.at(static_cast<unsigned>(D) + D_size * static_cast<unsigned>(component_t::DRAM)) = container_.at(exists_cnt);
    return;
}

void mapping_table_t::swap_degrees(const std::vector<unsigned>& degrees_) {
    degrees.assign(degrees_.cbegin(), degrees_.cend());
    return;
}

std::vector<unsigned> mapping_table_t::get_degrees() const {
    return degrees;
}

std::vector<unsigned> mapping_table_t::get_layer_values() const {
    return layer_values; 
}

std::vector<unsigned> mapping_table_t::get_row_degrees(const component_t U) const {
    std::vector<unsigned> rtn_degrees;
    for(unsigned column = 0; column < D_size; column++) 
        rtn_degrees.push_back(degrees.at(column + D_size * static_cast<unsigned>(U)));
    return rtn_degrees;
}
