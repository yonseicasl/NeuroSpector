#include "optimizer.h"

static handler_t handler;

/* Optimizer */
optimizer_t::optimizer_t(const std::string& acc_cfg_path_, 
                         const std::string& net_cfg_path_, 
                         const bool is_fixed_) 
    : is_fixed(is_fixed_),
      D_size(static_cast<unsigned>(parameter_t::SIZE)),
      U_size(static_cast<unsigned>(component_t::SIZE)), 
      num_levels(0),
      accelerator(new accelerator_t(acc_cfg_path_)), 
      network_name("") {
    // Component exist bits from MAC to DRAM
    exists.push_back(true); // MAC
    exists.push_back(accelerator->macs_per_pe() * accelerator->mac_width() != 1 ? true : false);
    exists.push_back(accelerator->l1_type() != buffer_type_t::NONE ? true : false);
    exists.push_back(accelerator->s1_size_x() > 1 ? true : false);
    exists.push_back(accelerator->s1_size_y() > 1 ? true : false);
    exists.push_back(accelerator->l2_type() != buffer_type_t::NONE ? true : false);
    exists.push_back(accelerator->s2_size() > 1 ? true : false);
    exists.push_back(true); // DRAM 
    // # of existent levels
    for(unsigned u = 0; u < U_size; u++) {
        if(exists.at(u))
            num_levels++;
    }
    // Mapping tables initialization
    net_cfg_t *net_cfg = new net_cfg_t(net_cfg_path_);
    network_name = net_cfg->network_name;
    for(size_t idx = 0; idx < net_cfg->layers.size(); idx++) {
        mapping_table_t mapping_tmp(net_cfg->layers.at(idx).is_grouped,
                                    exists, 
                                    net_cfg->layers.at(idx).name, 
                                    net_cfg->layers.at(idx).values,
                                    net_cfg->layers.at(idx).stride);
        mappings.push_back(mapping_tmp); 
    }
    delete net_cfg;
    init_dataflows();
    accelerator->print_stats();
}

optimizer_t::~optimizer_t() {
    delete accelerator;
}

// Optimizer APIs
void optimizer_t::run() {
    return;
}

void optimizer_t::run(const unsigned idx_) {
    return;
}

void optimizer_t::print_stats() {
    return;
}

void optimizer_t::print_csv() {
    return;
}

// Initialze dataflows
void optimizer_t::init_dataflows() {
    // For flexible dataflows
    if(!is_fixed) {
        // IS (0), WS (1), and OS (2)
        for(unsigned i = 0; i < 3; i++) {
            if(i == 0) {
                if(accelerator->l2_filter_bypass() || accelerator->l2_output_bypass())
                    l2_dataflows.push_back(i);
                else if(accelerator->l2_input_bypass()) 
                    l1_dataflows.push_back(i);
                else {
                    l1_dataflows.push_back(i);
                    l2_dataflows.push_back(i);
                }
            }
            else if(i == 1) {
                if(accelerator->l2_input_bypass() || accelerator->l2_output_bypass())
                    l2_dataflows.push_back(i);
                else if(accelerator->l2_filter_bypass()) 
                    l1_dataflows.push_back(i);
                else {
                    l1_dataflows.push_back(i);
                    l2_dataflows.push_back(i);
                }
            }
            else {
                if(accelerator->l2_input_bypass() || accelerator->l2_filter_bypass())
                    l2_dataflows.push_back(i);
                else if(accelerator->l2_output_bypass()) 
                    l1_dataflows.push_back(i);
                else {
                    l1_dataflows.push_back(i);
                    l2_dataflows.push_back(i);
                }
            }
        }
    }
    // For fixed dataflows
    else {
        // IS (0), WS (1), and OS (2)
        l1_dataflows.push_back(static_cast<unsigned>(accelerator->l1_dataflow()));
        l2_dataflows.push_back(static_cast<unsigned>(accelerator->l2_dataflow()));
    }
    return;
}

// Check each mapping table with the accelerator
bool optimizer_t::check_all_validity(const mapping_table_t& mapping_table_) const {
    bool rtn = mac_validity(mapping_table_) 
             & s0_validity(mapping_table_) 
             & l1_validity(mapping_table_)
             & s1_x_validity(mapping_table_)
             & s1_y_validity(mapping_table_)
             & l2_validity(mapping_table_)
             & s2_validity(mapping_table_);
    return rtn;
}

bool optimizer_t::check_validity(const component_t U, const mapping_table_t& mapping_table_) const {
    bool rtn = false;
    switch(U) {
        case component_t::MAC: rtn = mac_validity(mapping_table_); break;
        case component_t::S0: rtn = s0_validity(mapping_table_); break;
        case component_t::L1: rtn = l1_validity(mapping_table_); break;
        case component_t::S1_X: rtn = s1_x_validity(mapping_table_) & s1_y_validity(mapping_table_); break;
        case component_t::S1_Y: rtn = s1_x_validity(mapping_table_) & s1_y_validity(mapping_table_); break;
        case component_t::L2: rtn = l2_validity(mapping_table_); break;
        case component_t::S2: rtn = s2_validity(mapping_table_); break;
        default: handler.print_err(err_type_t::INVAILD, "COMPONENT"); break;
    }
    return rtn;
}

bool optimizer_t::mac_validity(const mapping_table_t& mapping_table_) const {
    return true;
}     

bool optimizer_t::s0_validity(const mapping_table_t& mapping_table_) const {
    bool validity = true;
    unsigned macs_per_pe_val = 1;
    unsigned mac_width_val = 1;
    macs_per_pe_val *= mapping_table_.get_degree(parameter_t::K, component_t::S0)
                     * mapping_table_.get_degree(parameter_t::B, component_t::S0)
                     * mapping_table_.get_degree(parameter_t::P, component_t::S0)
                     * mapping_table_.get_degree(parameter_t::Q, component_t::S0);
    mac_width_val *= mapping_table_.get_degree(parameter_t::C, component_t::S0)
                   * mapping_table_.get_degree(parameter_t::R, component_t::S0)
                   * mapping_table_.get_degree(parameter_t::S, component_t::S0);
    if(macs_per_pe_val > accelerator->macs_per_pe() || mac_width_val > accelerator->mac_width()) 
        validity = false;
    return validity;
}

bool optimizer_t::l1_validity(const mapping_table_t& mapping_table_) const {
    bool validity = true;
    unsigned shared_tile_size = 0; 
    switch(accelerator->l1_type()) {
        case buffer_type_t::NONE: 
            return validity; 
            break;
        case buffer_type_t::SEPARATED: 
            if(!accelerator->l1_input_bypass() && mapping_table_.get_input_tile_size(component_t::L1) > accelerator->l1_input_size())
                validity = false;
            if(!accelerator->l1_filter_bypass() && mapping_table_.get_filter_tile_size(component_t::L1) > accelerator->l1_filter_size())
                validity = false;
            if(!accelerator->l1_output_bypass() && mapping_table_.get_output_tile_size(component_t::L1) > accelerator->l1_output_size())
                validity = false;
            return validity; 
            break;
        case buffer_type_t::SHARED: 
            shared_tile_size = mapping_table_.get_input_tile_size(component_t::L1) 
                             + mapping_table_.get_filter_tile_size(component_t::L1)
                             + mapping_table_.get_output_tile_size(component_t::L1);
            if(shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_IF: 
            shared_tile_size = mapping_table_.get_input_tile_size(component_t::L1) 
                             + mapping_table_.get_filter_tile_size(component_t::L1);
            if((!accelerator->l1_output_bypass() && mapping_table_.get_output_tile_size(component_t::L1) > accelerator->l1_output_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_FO: 
            shared_tile_size = mapping_table_.get_filter_tile_size(component_t::L1) 
                             + mapping_table_.get_output_tile_size(component_t::L1);
            if((!accelerator->l1_input_bypass() && mapping_table_.get_input_tile_size(component_t::L1) > accelerator->l1_input_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_OI: 
            shared_tile_size = mapping_table_.get_input_tile_size(component_t::L1) 
                             + mapping_table_.get_output_tile_size(component_t::L1);
            if((!accelerator->l1_filter_bypass() && mapping_table_.get_filter_tile_size(component_t::L1) > accelerator->l1_filter_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        default: handler.print_err(err_type_t::INVAILD, "L1 TYPE"); break;
    } 
    // Utilization threshold for shared buffers
    float utilization = float(shared_tile_size) / accelerator->l1_shared_size();
    if(utilization < L1_THRESHOLD)
        validity = false;
    return validity;
}     

bool optimizer_t::s1_x_validity(const mapping_table_t& mapping_table_) const {
    bool validity = true;
    unsigned s1_size_x_val = 1;
    for(unsigned column = 0; column < D_size; column++)
        s1_size_x_val *= mapping_table_.get_degree(static_cast<parameter_t>(column), component_t::S1_X);
    if(s1_size_x_val > accelerator->s1_size_x()) 
        validity = false;
    float utilization = float(s1_size_x_val) / accelerator->s1_size_x();
    if(utilization < S1_THRESHOLD)
        validity = false;
    return validity;
}     

bool optimizer_t::s1_y_validity(const mapping_table_t& mapping_table_) const {
    bool validity = true;
    unsigned s1_size_y_val = 1;
    for(unsigned column = 0; column < D_size; column++)
        s1_size_y_val *= mapping_table_.get_degree(static_cast<parameter_t>(column), component_t::S1_Y);
    if(s1_size_y_val > accelerator->s1_size_y()) 
        validity = false;
    float utilization = float(s1_size_y_val) / accelerator->s1_size_y();
    if(utilization < S1_THRESHOLD)
        validity = false;
    return validity;
}     

bool optimizer_t::l2_validity(const mapping_table_t& mapping_table_) const {
    bool validity = true;
    unsigned shared_tile_size = 0; 
    switch(accelerator->l2_type()) {
        case buffer_type_t::NONE: 
            return validity;
            break;
        case buffer_type_t::SEPARATED: 
            if(!accelerator->l2_input_bypass() && mapping_table_.get_input_tile_size(component_t::L2) > accelerator->l2_input_size())
                validity = false;
            if(!accelerator->l2_filter_bypass() && mapping_table_.get_filter_tile_size(component_t::L2) > accelerator->l2_filter_size())
                validity = false;
            if(!accelerator->l2_output_bypass() && mapping_table_.get_output_tile_size(component_t::L2) > accelerator->l2_output_size())
                validity = false;
            return validity;
            break;
        case buffer_type_t::SHARED: 
            shared_tile_size = mapping_table_.get_input_tile_size(component_t::L2) 
                             + mapping_table_.get_filter_tile_size(component_t::L2)
                             + mapping_table_.get_output_tile_size(component_t::L2);
            if(shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_IF: 
            shared_tile_size = mapping_table_.get_input_tile_size(component_t::L2) 
                             + mapping_table_.get_filter_tile_size(component_t::L2);
            if((!accelerator->l2_output_bypass() && mapping_table_.get_output_tile_size(component_t::L2) > accelerator->l2_output_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_FO: 
            shared_tile_size = mapping_table_.get_filter_tile_size(component_t::L2) 
                             + mapping_table_.get_output_tile_size(component_t::L2);
            if((!accelerator->l2_input_bypass() && mapping_table_.get_input_tile_size(component_t::L2) > accelerator->l2_input_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_OI: 
            shared_tile_size = mapping_table_.get_input_tile_size(component_t::L2) 
                             + mapping_table_.get_output_tile_size(component_t::L2);
            if((!accelerator->l2_filter_bypass() && mapping_table_.get_filter_tile_size(component_t::L2) > accelerator->l2_filter_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        default: handler.print_err(err_type_t::INVAILD, "L2 TYPE"); break;
    } 
    // Utilization threshold for shared buffers
    float utilization = float(shared_tile_size) / accelerator->l2_shared_size();
    if(utilization < L2_THRESHOLD)
        validity = false;
    return validity;
}     

bool optimizer_t::s2_validity(const mapping_table_t& mapping_table_) const {
    bool validity = true;
    unsigned s2_size_val = 1;
    for(unsigned column = 0; column < D_size; column++) {
        // Only G, K, B, P, and Q
        if(column == static_cast<unsigned>(parameter_t::G) || 
           column == static_cast<unsigned>(parameter_t::K) || 
           column == static_cast<unsigned>(parameter_t::B) || 
           column == static_cast<unsigned>(parameter_t::P) || 
           column == static_cast<unsigned>(parameter_t::Q))
            s2_size_val *= mapping_table_.get_degree(static_cast<parameter_t>(column), component_t::S2);
        else {
            if(mapping_table_.get_degree(static_cast<parameter_t>(column), component_t::S2) > 1) {
                validity = false;
                break;
            }
        }
    }
    if(s2_size_val > accelerator->s2_size()) 
        validity = false;
    return validity;
}     
