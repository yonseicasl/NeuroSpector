#include "analyzer.h"

static handler_t handler;

/* Analyzer */
analyzer_t::analyzer_t(const std::string& acc_cfg_path_,
                       const std::string& map_cfg_path_) 
    : D_size(static_cast<unsigned>(parameter_t::SIZE)),
      //U_size(static_cast<unsigned>(component_t::SIZE)), 
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
    // Mapping tables & stat containers initialization
    map_cfg_t *map_cfg = new map_cfg_t(map_cfg_path_);
    network_name = map_cfg->network_name;
    for(size_t idx = 0; idx < map_cfg->mapping_tables.size(); idx++) {
        // Mapping table
        mapping_table_t mapping_table(exists, 
                                      map_cfg->mapping_tables.at(idx).name,
                                      map_cfg->mapping_tables.at(idx).values,
                                      map_cfg->mapping_tables.at(idx).stride);
        mapping_table.init_degrees(map_cfg->mapping_tables.at(idx).degrees);
        mapping_table.update_dram_row();
        mapping_tables.push_back(mapping_table); 
        // Stat container
        stats_t *stats = new stats_t(accelerator, mapping_table);
        all_stats.push_back(stats);
        update_stats(idx);
    }
    delete map_cfg;
}

analyzer_t::~analyzer_t() {
    delete accelerator;
    for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
        delete all_stats.at(idx);
    }
}

// Analyzer APIs
void analyzer_t::print_stats() const {
    accelerator->print_stats();
    std::cout << "\n# NETWORK: " << network_name << std::endl;
    for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
        check_validity(idx);
#ifdef CSV
        mapping_tables.at(idx).print_csv();
        all_stats.at(idx)->print_csv();
#else
        mapping_tables.at(idx).print_stats();
        all_stats.at(idx)->print_stats();
#endif
    }
    return;
}

void analyzer_t::print_stats(const unsigned idx_) const {
    accelerator->print_stats();
    std::cout << "\n# NETWORK: " << network_name << std::endl;
    check_validity(idx_ - 1);
#ifdef CSV
    mapping_tables.at(idx_ - 1).print_csv();
    all_stats.at(idx_ - 1)->print_csv();
#else
    mapping_tables.at(idx_ - 1).print_stats();
    all_stats.at(idx_ - 1)->print_stats();
#endif
    return;
}

// Check each mapping table with the accelerator
void analyzer_t::check_validity(const unsigned idx_) const {
    if(!mac_validity(idx_)) { std::cout << "# Mapping table " << idx_ << " is invalid at MAC [T]" << std::endl; exit(1); }     
    if(!s0_validity(idx_)) { std::cout << "# Mapping table " << idx_ << " is invalid at S0 [S]" << std::endl; exit(1); }   
    if(!l1_validity(idx_)) { std::cout << "# Mapping table " << idx_ << " is invalid at L1 [T]" << std::endl; exit(1); }   
    if(!s1_x_validity(idx_)) { std::cout << "# Mapping table " << idx_ << " is invalid at S1_X [S]" << std::endl; exit(1); }    
    if(!s1_y_validity(idx_)) { std::cout << "# Mapping table " << idx_ << " is invalid at S1_Y [S]" << std::endl; exit(1); }    
    if(!l2_validity(idx_)) { std::cout << "# Mapping table " << idx_ << " is invalid at L2 [T]" << std::endl; exit(1); }    
    if(!s2_validity(idx_)) { std::cout << "# Mapping table " << idx_ << " is invalid at S2 [S]" << std::endl; exit(1); }     
    return;
}

bool analyzer_t::mac_validity(const unsigned idx_) const {
    return true;
}     

bool analyzer_t::s0_validity(const unsigned idx_) const {
    bool validity = true;
    unsigned macs_per_pe_val = 1;
    unsigned mac_width_val = 1;
    macs_per_pe_val *= mapping_tables.at(idx_).get_degree(parameter_t::K, component_t::S0)
                     * mapping_tables.at(idx_).get_degree(parameter_t::B, component_t::S0)
                     * mapping_tables.at(idx_).get_degree(parameter_t::P, component_t::S0)
                     * mapping_tables.at(idx_).get_degree(parameter_t::Q, component_t::S0);
    mac_width_val *= mapping_tables.at(idx_).get_degree(parameter_t::C, component_t::S0)
                   * mapping_tables.at(idx_).get_degree(parameter_t::R, component_t::S0)
                   * mapping_tables.at(idx_).get_degree(parameter_t::S, component_t::S0);
    if(macs_per_pe_val > accelerator->macs_per_pe() || mac_width_val > accelerator->mac_width()) 
        validity = false;
    return validity;
}

bool analyzer_t::l1_validity(const unsigned idx_) const {
    bool validity = true;
    unsigned shared_tile_size = 0; 
    switch(accelerator->l1_type()) {
        case buffer_type_t::NONE: break;
        case buffer_type_t::SEPARATED: 
            if(!accelerator->l1_input_bypass() && mapping_tables.at(idx_).get_input_tile_size(component_t::L1) > accelerator->l1_input_size())
                validity = false;
            if(!accelerator->l1_filter_bypass() && mapping_tables.at(idx_).get_filter_tile_size(component_t::L1) > accelerator->l1_filter_size())
                validity = false;
            if(!accelerator->l1_output_bypass() && mapping_tables.at(idx_).get_output_tile_size(component_t::L1) > accelerator->l1_output_size())
                validity = false;
            break;
        case buffer_type_t::SHARED: 
            shared_tile_size = mapping_tables.at(idx_).get_input_tile_size(component_t::L1) 
                             + mapping_tables.at(idx_).get_filter_tile_size(component_t::L1)
                             + mapping_tables.at(idx_).get_output_tile_size(component_t::L1);
            if(shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_IF: 
            shared_tile_size = mapping_tables.at(idx_).get_input_tile_size(component_t::L1) 
                             + mapping_tables.at(idx_).get_filter_tile_size(component_t::L1);
            if((!accelerator->l1_output_bypass() && mapping_tables.at(idx_).get_output_tile_size(component_t::L1) > accelerator->l1_output_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_FO: 
            shared_tile_size = mapping_tables.at(idx_).get_filter_tile_size(component_t::L1) 
                             + mapping_tables.at(idx_).get_output_tile_size(component_t::L1);
            if((!accelerator->l1_input_bypass() && mapping_tables.at(idx_).get_input_tile_size(component_t::L1) > accelerator->l1_input_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_OI: 
            shared_tile_size = mapping_tables.at(idx_).get_input_tile_size(component_t::L1) 
                             + mapping_tables.at(idx_).get_output_tile_size(component_t::L1);
            if((!accelerator->l1_filter_bypass() && mapping_tables.at(idx_).get_filter_tile_size(component_t::L1) > accelerator->l1_filter_size()) 
                || shared_tile_size > accelerator->l1_shared_size())
                validity = false;
            break;
        default: handler.print_err(err_type_t::INVAILD, "L1 TYPE"); break;
    } 
    return validity;
}     

bool analyzer_t::s1_x_validity(const unsigned idx_) const {
    bool validity = true;
    unsigned s1_size_x_val = 1;
    for(unsigned column = 0; column < D_size; column++)
        s1_size_x_val *= mapping_tables.at(idx_).get_degree(static_cast<parameter_t>(column), component_t::S1_X);
    if(s1_size_x_val > accelerator->s1_size_x()) 
        validity = false;
    return validity;
}     

bool analyzer_t::s1_y_validity(const unsigned idx_) const {
    bool validity = true;
    unsigned s1_size_y_val = 1;
    for(unsigned column = 0; column < D_size; column++)
        s1_size_y_val *= mapping_tables.at(idx_).get_degree(static_cast<parameter_t>(column), component_t::S1_Y);
    if(s1_size_y_val > accelerator->s1_size_y()) 
        validity = false;
    return validity;
}     

bool analyzer_t::l2_validity(const unsigned idx_) const {
    bool validity = true;
    unsigned shared_tile_size = 0; 
    switch(accelerator->l2_type()) {
        case buffer_type_t::NONE: break;
        case buffer_type_t::SEPARATED: 
            if(!accelerator->l2_input_bypass() && mapping_tables.at(idx_).get_input_tile_size(component_t::L2) > accelerator->l2_input_size())
                validity = false;
            if(!accelerator->l2_filter_bypass() && mapping_tables.at(idx_).get_filter_tile_size(component_t::L2) > accelerator->l2_filter_size())
                validity = false;
            if(!accelerator->l2_output_bypass() && mapping_tables.at(idx_).get_output_tile_size(component_t::L2) > accelerator->l2_output_size())
                validity = false;
            break;
        case buffer_type_t::SHARED: 
            shared_tile_size = mapping_tables.at(idx_).get_input_tile_size(component_t::L2) 
                             + mapping_tables.at(idx_).get_filter_tile_size(component_t::L2)
                             + mapping_tables.at(idx_).get_output_tile_size(component_t::L2);
            if(shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_IF: 
            shared_tile_size = mapping_tables.at(idx_).get_input_tile_size(component_t::L2) 
                             + mapping_tables.at(idx_).get_filter_tile_size(component_t::L2);
            if((!accelerator->l2_output_bypass() && mapping_tables.at(idx_).get_output_tile_size(component_t::L2) > accelerator->l2_output_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_FO: 
            shared_tile_size = mapping_tables.at(idx_).get_filter_tile_size(component_t::L2) 
                             + mapping_tables.at(idx_).get_output_tile_size(component_t::L2);
            if((!accelerator->l2_input_bypass() && mapping_tables.at(idx_).get_input_tile_size(component_t::L2) > accelerator->l2_input_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        case buffer_type_t::SHARED_OI: 
            shared_tile_size = mapping_tables.at(idx_).get_input_tile_size(component_t::L2) 
                             + mapping_tables.at(idx_).get_output_tile_size(component_t::L2);
            if((!accelerator->l2_filter_bypass() && mapping_tables.at(idx_).get_filter_tile_size(component_t::L2) > accelerator->l2_filter_size()) 
                || shared_tile_size > accelerator->l2_shared_size())
                validity = false;
            break;
        default: handler.print_err(err_type_t::INVAILD, "L2 TYPE"); break;
    } 
    return validity;
}     

bool analyzer_t::s2_validity(const unsigned idx_) const {
    bool validity = true;
    unsigned s2_size_val = 1;
    // Only K, B, P, and Q
    for(unsigned column = 0; column < D_size; column++) {
        if(column == 0 || column == 1 || column == 2 || column == 3)
            s2_size_val *= mapping_tables.at(idx_).get_degree(static_cast<parameter_t>(column), component_t::S2);
        else {
            if(mapping_tables.at(idx_).get_degree(static_cast<parameter_t>(column), component_t::S2) > 1) {
                validity = false;
                break;
            }
        }
    }
    if(s2_size_val > accelerator->s2_size()) 
        validity = false;
    return validity;
}     

void analyzer_t::update_stats(const unsigned idx_) {
    all_stats.at(idx_)->update_stats();
    return;
}
