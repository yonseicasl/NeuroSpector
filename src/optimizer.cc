#include "optimizer.h"
#include <iomanip>

static handler_t handler;

optimizer_t::optimizer_t(optimizer_configs_t &optimizer_configs_) 
    : optimizer_configs(optimizer_configs_) {
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
    accelerator->name = optimizer_configs.accelerator.name;
    // MAC
    accelerator->mac_per_pe = optimizer_configs.accelerator.mac_per_pe;
    accelerator->mac_width = optimizer_configs.accelerator.mac_width;
    accelerator->L0_stationary = optimizer_configs.accelerator.L0_stationary;
    // L1
    accelerator->input_L1_sizes = optimizer_configs.accelerator.L1_sizes.at(0);
    accelerator->weight_L1_sizes = optimizer_configs.accelerator.L1_sizes.at(1);
    accelerator->output_L1_sizes = optimizer_configs.accelerator.L1_sizes.at(2);
    unsigned L1_bypass = 0;
    L1_bypass = optimizer_configs.accelerator.L1_bypass.at(0) * 4;       // Input
    L1_bypass += optimizer_configs.accelerator.L1_bypass.at(1) * 2;      // Weight
    L1_bypass += optimizer_configs.accelerator.L1_bypass.at(2) * 1;      // Output
    accelerator->L1_bypass = static_cast<bypass_t>(L1_bypass);
    accelerator->L1_stationary = optimizer_configs.accelerator.L1_stationary;
    // X, Y
    accelerator->array_size_x = optimizer_configs.accelerator.array_size_x;
    accelerator->array_size_y = optimizer_configs.accelerator.array_size_y;
    accelerator->array_unroll_x = static_cast<parameter_t>(parameters.find(optimizer_configs.accelerator.array_unroll_x));
    accelerator->array_unroll_y = static_cast<parameter_t>(parameters.find(optimizer_configs.accelerator.array_unroll_y));
    // L2
    accelerator->L2_size = optimizer_configs.accelerator.L2_size;
    unsigned L2_bypass = 0;
    L2_bypass = optimizer_configs.accelerator.L2_bypass.at(0) * 4;       // Input
    L2_bypass += optimizer_configs.accelerator.L2_bypass.at(1) * 2;      // Weight
    L2_bypass += optimizer_configs.accelerator.L2_bypass.at(2) * 1;      // Output
    accelerator->L2_bypass = static_cast<bypass_t>(L2_bypass);
    accelerator->L2_stationary = optimizer_configs.accelerator.L2_stationary;
    // PRECISION
    accelerator->input_precision = optimizer_configs.accelerator.precision.at(0);     // Input
    accelerator->weight_precision = optimizer_configs.accelerator.precision.at(1);    // Weight
    accelerator->output_precision = optimizer_configs.accelerator.precision.at(2);    // Output
}

void optimizer_t::init_layer_and_map() {
    for(size_t i = 0; i < optimizer_configs.layers.size(); i++) {
        // Data sizes
        data_size_t *tmp_data_size = new data_size_t;
        tmp_data_size->input_size = optimizer_configs.layers.at(i).layer_vals.at(1)
                                  * optimizer_configs.layers.at(i).layer_vals.at(4)
                                  * (((optimizer_configs.layers.at(i).layer_vals.at(2) - 1)
                                  * optimizer_configs.layers.at(i).layer_vals.at(7))
                                  + optimizer_configs.layers.at(i).layer_vals.at(5))
                                  * (((optimizer_configs.layers.at(i).layer_vals.at(3) - 1)
                                  * optimizer_configs.layers.at(i).layer_vals.at(7))
                                  + optimizer_configs.layers.at(i).layer_vals.at(6));
        tmp_data_size->weight_size = optimizer_configs.layers.at(i).layer_vals.at(0)
                                   * optimizer_configs.layers.at(i).layer_vals.at(4)
                                   * optimizer_configs.layers.at(i).layer_vals.at(5)
                                   * optimizer_configs.layers.at(i).layer_vals.at(6);
        tmp_data_size->output_size = optimizer_configs.layers.at(i).layer_vals.at(0)
                                   * optimizer_configs.layers.at(i).layer_vals.at(1)
                                   * optimizer_configs.layers.at(i).layer_vals.at(2)
                                   * optimizer_configs.layers.at(i).layer_vals.at(3);
        data_sizes.push_back(tmp_data_size);
        // Layer parameter values
        mapping_table_t *tmp_mt = new mapping_table_t;
        std::copy(optimizer_configs.layers.at(i).layer_vals.begin(), 
                  optimizer_configs.layers.at(i).layer_vals.end() - 1, 
                  tmp_mt->layer_vals.begin());
        tmp_mt->stride = optimizer_configs.layers.at(i).layer_vals.at(7);
        mapping_tables.push_back(std::make_pair(optimizer_configs.layers.at(i).name, tmp_mt)); 
    }
}


void optimizer_t::print_stats(unsigned idx_) {
    print_accelerator();
    handler.print_line(35, "-");
    std::cout << "\n# NETWORK: " << optimizer_configs.network_name << std::endl;
    handler.print_line(35, "-");
    if(idx_ == 0) {
        for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
            std::cout << "# LAYER: " << mapping_tables.at(idx).first << std::endl;
            handler.print_line(17, "*");
            std::cout << "# MAPPING TABLE" << std::endl;
            print_mapping_tables(idx);
            handler.print_line(42, "-");
            std::cout << " DATA SIZE |" 
                      << std::setw(10) << data_sizes.at(idx)->input_size 
                      << std::setw(10) << data_sizes.at(idx)->weight_size
                      << std::setw(10) << data_sizes.at(idx)->output_size << std::endl;
            std::cout << std::endl;
        }
    }
    else {
        std::cout << "# LAYER: " << mapping_tables.at(idx_ - 1).first << std::endl;
        handler.print_line(17, "*");
        std::cout << "# MAPPING TABLE" << std::endl;
        print_mapping_tables(idx_ - 1);
        handler.print_line(42, "-");
        std::cout << " DATA SIZE |" 
                  << std::setw(10) << data_sizes.at(idx_ - 1)->input_size 
                  << std::setw(10) << data_sizes.at(idx_ - 1)->weight_size
                  << std::setw(10) << data_sizes.at(idx_ - 1)->output_size << std::endl;
        std::cout << std::endl;
    }
}

void optimizer_t::print_accelerator() {
     accelerator->print_stats(); 
}

void optimizer_t::print_mapping_tables(unsigned idx_) {
    mapping_tables.at(idx_).second->print_stats();
}

void optimizer_t::optimize() {
    for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
        /* PE array optimization */
        unsigned map_x = static_cast<unsigned>(accelerator->array_unroll_x);
        unsigned map_y = static_cast<unsigned>(accelerator->array_unroll_y);
        unsigned expanded_x = 0;
        unsigned expanded_y = 0;

        if(accelerator->array_size_x <= mapping_tables.at(idx).second->layer_vals.at(map_x)) {
            mapping_tables.at(idx).second->put_val(accelerator->array_unroll_x, component_t::X, accelerator->array_size_x);
            // TODO: eyeriss conv2 
        }
        else {
            mapping_tables.at(idx).second->put_val(accelerator->array_unroll_x, component_t::X, mapping_tables.at(idx).second->layer_vals.at(map_x));
            expanded_x = accelerator->array_size_x / mapping_tables.at(idx).second->layer_vals.at(map_x);
            if(expanded_x >= 2) {
                // TODO: accelerator folding possible
                mapping_tables.at(idx).second->expand(component_t::X, expanded_x);
            }
        }
        if(accelerator->array_size_y <= mapping_tables.at(idx).second->layer_vals.at(map_y)) {
            mapping_tables.at(idx).second->put_val(accelerator->array_unroll_y, component_t::Y, accelerator->array_size_y);
            // TODO: eyeriss conv2 
        }
        else {
            mapping_tables.at(idx).second->put_val(accelerator->array_unroll_y, component_t::Y, mapping_tables.at(idx).second->layer_vals.at(map_y) );
            expanded_y = accelerator->array_size_y / mapping_tables.at(idx).second->layer_vals.at(map_y);
            if(expanded_y >= 2) {
                // TODO: accelerator folding possible
                mapping_tables.at(idx).second->expand(component_t::Y, expanded_y);
            }
        }
        /* MAC unit optimization */

        /* L1 optimization */
        if(accelerator->L1_stationary == "IS") {

        }
        else if(accelerator->L1_stationary == "WS") {

        }
        else if(accelerator->L1_stationary == "OS") {

        }
        else
            handler.print_err(err_type_t::INVAILD, "L1 stationary in optimizer_t::optimize()");

        /* L2 optimization */

        float utx = float(mapping_tables.at(idx).second->get_row_product(component_t::X)) / accelerator->array_size_x;
        float uty = float(mapping_tables.at(idx).second->get_row_product(component_t::Y)) / accelerator->array_size_y;
        std::cout << "\n# " << mapping_tables.at(idx).first
                  << "\n# PE array utilization: " << utx * uty * 100 << "%" << std::endl;
        // Print mapping table
        mapping_tables.at(idx).second->print_stats();
    }
}
