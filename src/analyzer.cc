#include "analyzer.h"
#include <iomanip>

static handler_t handler;

analyzer_t::analyzer_t(analyzer_configs_t &analyzer_configs_) 
    : analyzer_configs(analyzer_configs_) {
    init_acc();
    init_layer_and_map();
    std::cout << "# Analyzer initialization complete!" << std::endl;
    analyze_tiles();
    analyze_accesses();
}

analyzer_t::~analyzer_t() {
    delete accelerator;
    for(size_t i = 0; i < mapping_tables.size(); i++) {
        delete mapping_tables.at(i).second;
        delete data_sizes.at(i);
        delete tiles.at(i);
        delete accesses.at(i);
    }
}

void analyzer_t::init_acc() {
    accelerator = new accelerator_t;
    accelerator->name = analyzer_configs.accelerator.name;
    accelerator->array_size_x = analyzer_configs.accelerator.array_size_x;
    accelerator->array_size_y = analyzer_configs.accelerator.array_size_y;
    accelerator->input_precision = analyzer_configs.accelerator.precision.at(0);     // Input
    accelerator->weight_precision = analyzer_configs.accelerator.precision.at(1);    // Weight
    accelerator->output_precision = analyzer_configs.accelerator.precision.at(2);    // Output
    unsigned bypass_L1 = 0;
    unsigned bypass_L2 = 0;
    bypass_L1 = analyzer_configs.accelerator.bypass_L1.at(0) * 4;       // Input
    bypass_L1 += analyzer_configs.accelerator.bypass_L1.at(1) * 2;      // Weight
    bypass_L1 += analyzer_configs.accelerator.bypass_L1.at(2) * 1;      // Output
    bypass_L2 = analyzer_configs.accelerator.bypass_L2.at(0) * 4;       // Input
    bypass_L2 += analyzer_configs.accelerator.bypass_L2.at(1) * 2;      // Weight
    bypass_L2 += analyzer_configs.accelerator.bypass_L2.at(2) * 1;      // Output
    accelerator->bypass_L1 = static_cast<bypass_t>(bypass_L1);
    accelerator->bypass_L2 = static_cast<bypass_t>(bypass_L2);
}

void analyzer_t::init_layer_and_map() {
    for(size_t i = 0; i < analyzer_configs.layers_and_maps.size(); i++) {
        // Data sizes
        data_size_t *tmp_data_size = new data_size_t;
        tmp_data_size->input_size = analyzer_configs.layers_and_maps.at(i).layer_vals.at(1)
                                  * analyzer_configs.layers_and_maps.at(i).layer_vals.at(4)
                                  * (((analyzer_configs.layers_and_maps.at(i).layer_vals.at(2) - 1)
                                  * analyzer_configs.layers_and_maps.at(i).layer_vals.at(7))
                                  + analyzer_configs.layers_and_maps.at(i).layer_vals.at(5))
                                  * (((analyzer_configs.layers_and_maps.at(i).layer_vals.at(3) - 1)
                                  * analyzer_configs.layers_and_maps.at(i).layer_vals.at(7))
                                  + analyzer_configs.layers_and_maps.at(i).layer_vals.at(6));
        tmp_data_size->weight_size = analyzer_configs.layers_and_maps.at(i).layer_vals.at(0)
                                   * analyzer_configs.layers_and_maps.at(i).layer_vals.at(4)
                                   * analyzer_configs.layers_and_maps.at(i).layer_vals.at(5)
                                   * analyzer_configs.layers_and_maps.at(i).layer_vals.at(6);
        tmp_data_size->output_size = analyzer_configs.layers_and_maps.at(i).layer_vals.at(0)
                                   * analyzer_configs.layers_and_maps.at(i).layer_vals.at(1)
                                   * analyzer_configs.layers_and_maps.at(i).layer_vals.at(2)
                                   * analyzer_configs.layers_and_maps.at(i).layer_vals.at(3);
        data_sizes.push_back(tmp_data_size);
        // Layer parameter values
        mapping_table_t *tmp_mt = new mapping_table_t;
        std::copy(analyzer_configs.layers_and_maps.at(i).layer_vals.begin(), 
                  analyzer_configs.layers_and_maps.at(i).layer_vals.end() - 1, 
                  tmp_mt->layer_vals.begin());
        tmp_mt->stride = analyzer_configs.layers_and_maps.at(i).layer_vals.at(7);
        // Mapping table values 
        for(size_t r = 0; r < MAP_TABLE_ROWS; r++) {
            for(size_t c = 0; c < MAP_TABLE_COLUMNS + 1; c++) {
                if(c < MAP_TABLE_COLUMNS) {
                    parameter_t D = static_cast<parameter_t>(c); 
                    component_t U = static_cast<component_t>(r); 
                    tmp_mt->put_val(D, U, analyzer_configs.layers_and_maps.at(i).map_vals.at(c + r * (MAP_TABLE_COLUMNS + 1)));
                }
                else {
                    if(analyzer_configs.layers_and_maps.at(i).map_vals.at(c + r * (MAP_TABLE_COLUMNS + 1)) == 1)
                        tmp_mt->U_exists.at(r) = true;
                    else
                        tmp_mt->U_exists.at(r) = false;
                }
            }
        }
        mapping_tables.push_back(std::make_pair(analyzer_configs.layers_and_maps.at(i).name, tmp_mt)); 
    }
}

void analyzer_t::analyze_tiles() {
    for(size_t i = 0; i < mapping_tables.size(); i++) {
        tiles_t *tmp_tiles = new tiles_t(mapping_tables.at(i).second, accelerator);
        tiles.push_back(tmp_tiles);
    }
}

void analyzer_t::analyze_accesses() {
    for(size_t i = 0; i < mapping_tables.size(); i++) {
        accesses_t *tmp_accessess = new accesses_t(mapping_tables.at(i).second, accelerator, tiles.at(i));
        accesses.push_back(tmp_accessess);
    }
}

void analyzer_t::print_stats() {
    print_accelerator();
    handler.print_line(35, "-");
    std::cout << "\n# NETWORK: " << analyzer_configs.network_name << std::endl;
    handler.print_line(35, "-");
    for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
        std::cout << "# LAYER: " << mapping_tables.at(idx).first << std::endl;
        handler.print_line(17, "*");
        std::cout << "# MAPPING TABLE" << std::endl;
        print_mapping_tables(idx);
        print_tiles(idx);
        handler.print_line(42, "-");
        std::cout << " DATA SIZE |" 
                  << std::setw(10) << data_sizes.at(idx)->input_size 
                  << std::setw(10) << data_sizes.at(idx)->weight_size
                  << std::setw(10) << data_sizes.at(idx)->output_size << std::endl;
//        std::cout << " DATA SIZE |" 
//                  << std::setw(8) << std::fixed << std::setprecision(2) << double(data_sizes.at(idx)->input_size) / KB_UNIT << "KB"
//                  << std::setw(8) << std::fixed << std::setprecision(2) << double(data_sizes.at(idx)->weight_size) / KB_UNIT << "KB"
//                  << std::setw(8) << std::fixed << std::setprecision(2) << double(data_sizes.at(idx)->output_size) / KB_UNIT << "KB" << std::endl;
        print_accesses(idx);
        std::cout << std::endl;
    }
}

void analyzer_t::print_accelerator() {
     accelerator->print_stats(); 
}

void analyzer_t::print_mapping_tables(unsigned idx_) {
    mapping_tables.at(idx_).second->print_stats();
}

void analyzer_t::print_tiles(unsigned idx_) {
    tiles.at(idx_)->print_stats();
}

void analyzer_t::print_accesses(unsigned idx_) {
    accesses.at(idx_)->print_stats();
}
