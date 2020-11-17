#include "analyzer.h"
#include <iomanip>

static handler_t handler;

analyzer_t::analyzer_t(mapping_configs_t &mapping_configs_) 
    : mapping_configs(mapping_configs_) {
    init();
    std::cout << "# analyzer initialization complete!" << std::endl;
}

analyzer_t::~analyzer_t() {
    for(size_t i = 0; i < mapping_tables.size(); i++) {
        delete mapping_tables.at(i).second;
    }
}

void analyzer_t::init() {
    network_name = mapping_configs.network_name;
    for(size_t i = 0; i < mapping_configs.mappings.size(); i++) {
        // Layer parameter values
        mapping_table_t *tmp_mt = new mapping_table_t;
        std::copy(mapping_configs.mappings.at(i).layer_vals.begin(), 
                  mapping_configs.mappings.at(i).layer_vals.end() - 1, 
                  tmp_mt->layer_vals.begin());
        tmp_mt->stride = mapping_configs.mappings.at(i).layer_vals.at(7);
        tmp_mt->L0_dataflow = dataflow_t::NO;
        tmp_mt->L1_dataflow = dataflow_t::NO;
        tmp_mt->L2_dataflow = dataflow_t::NO;
        // Mapping table values 
        for(size_t u = 0; u < 5; u++) {
            for(size_t d = 0; d < 7; d++) {
                parameter_t D = static_cast<parameter_t>(d); 
                component_t U = static_cast<component_t>(u); 
                tmp_mt->put_val(D, U, mapping_configs.mappings.at(i).map_vals.at(d + u * 7));
            }
        }
        tmp_mt->update();
        mapping_tables.push_back(std::make_pair(mapping_configs.mappings.at(i).name, tmp_mt)); 
    }
}

void analyzer_t::print_stats(unsigned idx_) {
    handler.print_line(35, "-");
    std::cout << "\n# NETWORK: " << network_name << std::endl;
    handler.print_line(35, "-");
    if(idx_ == 0) {
        for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
            std::cout << "# LAYER: " << mapping_tables.at(idx).first << std::endl;
            handler.print_line(17, "*");
            std::cout << "# MAPPING TABLE" << std::endl;
            print_mapping_tables(idx);
        }
    }
    else {
        std::cout << "# LAYER: " << mapping_tables.at(idx_ - 1).first << std::endl;
        handler.print_line(17, "*");
        std::cout << "# MAPPING TABLE" << std::endl;
        print_mapping_tables(idx_ - 1);
    }
}

void analyzer_t::print_mapping_tables(unsigned idx_) {
    mapping_tables.at(idx_).second->print_stats();
}

