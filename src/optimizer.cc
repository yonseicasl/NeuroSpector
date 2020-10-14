#include "optimizer.h"
#include <iomanip>

static handler_t handler;

optimizer_t::optimizer_t(analyzer_configs_t &analyzer_configs_) 
    : analyzer_configs(analyzer_configs_) {
    init_acc();
    init_layer_and_map();
    std::cout << "# optimizer initialization complete!" << std::endl;
}

optimizer_t::~optimizer_t() {
    delete accelerator;
    for(size_t i = 0; i < mapping_tables.size(); i++) {
        delete mapping_tables.at(i).second;
        delete data_sizes.at(i);
    }
}

void optimizer_t::init_acc() {
    std::string parameters = "KBPQCRS";
    accelerator = new accelerator_t;
    accelerator->name = analyzer_configs.accelerator.name;
    // MAC
    accelerator->mac_per_pe = analyzer_configs.accelerator.mac_per_pe;
    accelerator->mac_width = analyzer_configs.accelerator.mac_width;
    accelerator->mac_stationary = analyzer_configs.accelerator.mac_stationary;
    // L1
    accelerator->input_L1_sizes = analyzer_configs.accelerator.L1_sizes.at(0);
    accelerator->weight_L1_sizes = analyzer_configs.accelerator.L1_sizes.at(1);
    accelerator->output_L1_sizes = analyzer_configs.accelerator.L1_sizes.at(2);
    unsigned L1_bypass = 0;
    L1_bypass = analyzer_configs.accelerator.L1_bypass.at(0) * 4;       // Input
    L1_bypass += analyzer_configs.accelerator.L1_bypass.at(1) * 2;      // Weight
    L1_bypass += analyzer_configs.accelerator.L1_bypass.at(2) * 1;      // Output
    accelerator->L1_bypass = static_cast<bypass_t>(L1_bypass);
    accelerator->L1_stationary = analyzer_configs.accelerator.L1_stationary;
    // X, Y
    accelerator->array_size_x = analyzer_configs.accelerator.array_size_x;
    accelerator->array_size_y = analyzer_configs.accelerator.array_size_y;
    accelerator->array_unroll_x = static_cast<parameter_t>(parameters.find(analyzer_configs.accelerator.array_unroll_x));
    accelerator->array_unroll_y = static_cast<parameter_t>(parameters.find(analyzer_configs.accelerator.array_unroll_y));
    // L2
    accelerator->L2_size = analyzer_configs.accelerator.L2_size;
    unsigned L2_bypass = 0;
    L2_bypass = analyzer_configs.accelerator.L2_bypass.at(0) * 4;       // Input
    L2_bypass += analyzer_configs.accelerator.L2_bypass.at(1) * 2;      // Weight
    L2_bypass += analyzer_configs.accelerator.L2_bypass.at(2) * 1;      // Output
    accelerator->L2_bypass = static_cast<bypass_t>(L2_bypass);
    accelerator->L2_stationary = analyzer_configs.accelerator.L2_stationary;
    // PRECISION
    accelerator->input_precision = analyzer_configs.accelerator.precision.at(0);     // Input
    accelerator->weight_precision = analyzer_configs.accelerator.precision.at(1);    // Weight
    accelerator->output_precision = analyzer_configs.accelerator.precision.at(2);    // Output
}

void optimizer_t::init_layer_and_map() {
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
        mapping_tables.push_back(std::make_pair(analyzer_configs.layers_and_maps.at(i).name, tmp_mt)); 
    }
}


void optimizer_t::print_stats(unsigned idx_) {
    print_accelerator();
    handler.print_line(35, "-");
//    std::cout << "\n# NETWORK: " << analyzer_configs.network_name << std::endl;
//    handler.print_line(35, "-");
//    if(idx_ == 0) {
//        for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
//            std::cout << "# LAYER: " << mapping_tables.at(idx).first << std::endl;
//            handler.print_line(17, "*");
//            std::cout << "# MAPPING TABLE" << std::endl;
//            print_mapping_tables(idx);
//            print_tiles(idx);
//            handler.print_line(42, "-");
//            std::cout << " DATA SIZE |" 
//                      << std::setw(10) << data_sizes.at(idx)->input_size 
//                      << std::setw(10) << data_sizes.at(idx)->weight_size
//                      << std::setw(10) << data_sizes.at(idx)->output_size << std::endl;
//            print_accesses(idx);
//            std::cout << std::endl;
//        }
//    }
//    else {
//        std::cout << "# LAYER: " << mapping_tables.at(idx_ - 1).first << std::endl;
//        handler.print_line(17, "*");
//        std::cout << "# MAPPING TABLE" << std::endl;
//        print_mapping_tables(idx_ - 1);
//        print_tiles(idx_ - 1);
//        handler.print_line(42, "-");
//        std::cout << " DATA SIZE |" 
//                  << std::setw(10) << data_sizes.at(idx_ - 1)->input_size 
//                  << std::setw(10) << data_sizes.at(idx_ - 1)->weight_size
//                  << std::setw(10) << data_sizes.at(idx_ - 1)->output_size << std::endl;
//        print_accesses(idx_ - 1);
//        std::cout << std::endl;
//    }
}

void optimizer_t::print_accelerator() {
     accelerator->print_stats(); 
}

void optimizer_t::print_mapping_tables(unsigned idx_) {
    mapping_tables.at(idx_).second->print_stats();
}
