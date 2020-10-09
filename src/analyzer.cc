#include "analyzer.h"

static handler_t handler;

analyzer_t::analyzer_t(analyzer_configs_t &analyzer_configs_) 
    : analyzer_configs(analyzer_configs_) {
    init_acc();
    init_layer_and_map();
    std::cout << "# Analyzer initialization complete!" << std::endl;
}

analyzer_t::~analyzer_t() {
    for(size_t i = 0; i < mapping_tables.size(); i++)
        delete mapping_tables.at(i).second;
}

void analyzer_t::init_acc() {
    accelerator.array_size_x = analyzer_configs.accelerator.array_size_x;
    accelerator.array_size_y = analyzer_configs.accelerator.array_size_y;
    accelerator.input_precision = analyzer_configs.accelerator.precision.at(0);     // Input
    accelerator.weight_precision = analyzer_configs.accelerator.precision.at(1);    // Weight
    accelerator.output_precision = analyzer_configs.accelerator.precision.at(2);    // Output
    accelerator.bypass_L1 = analyzer_configs.accelerator.bypass_L1.at(0) * 1;       // Input
    accelerator.bypass_L1 += analyzer_configs.accelerator.bypass_L1.at(1) * 2;      // Weight
    accelerator.bypass_L1 += analyzer_configs.accelerator.bypass_L1.at(2) * 4;      // Output
    accelerator.bypass_L2 = analyzer_configs.accelerator.bypass_L2.at(0) * 1;       // Input
    accelerator.bypass_L2 += analyzer_configs.accelerator.bypass_L2.at(1) * 2;      // Weight
    accelerator.bypass_L2 += analyzer_configs.accelerator.bypass_L2.at(2) * 4;      // Output
}

void analyzer_t::init_layer_and_map() {
    for(size_t i = 0; i < analyzer_configs.layers_and_maps.size(); i++) {
        mapping_table_t *tmp_mt = new mapping_table_t;
        // Layer parameter values
        std::copy(analyzer_configs.layers_and_maps.at(i).layer_vals.begin(), 
                  analyzer_configs.layers_and_maps.at(i).layer_vals.end() - 1, 
                  tmp_mt->layer_vals.begin());
        tmp_mt->stride = analyzer_configs.layers_and_maps.at(i).layer_vals.at(7);
        // Mapping table values 
        for(size_t r = 0; r < MAP_TABLE_ROWS; r++) {
            for(size_t c = 0; c < MAP_TABLE_COLUMNS; c++) {
                parameter_t D = static_cast<parameter_t>(c); 
                component_t U = static_cast<component_t>(r); 
                tmp_mt->put_val(D, U, analyzer_configs.layers_and_maps.at(i).map_vals.at(c + r * MAP_TABLE_ROWS));
            }
        }
        mapping_tables.push_back(std::make_pair(analyzer_configs.layers_and_maps.at(i).name, tmp_mt)); 
    }
}

void analyzer_t::print_mapping_tables() {
    for(size_t i = 0; i < mapping_tables.size(); i++) {
        std::cout << "\n# " << mapping_tables.at(i).first << std::endl;
        mapping_tables.at(i).second->print_stats();
    }
}
