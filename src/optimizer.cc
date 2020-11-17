#include "optimizer.h"
#include <iomanip>

static handler_t handler;

optimizer_t::optimizer_t(accelerator_configs_t &accelerator_configs_, network_configs_t &network_configs_) 
    : accelerator_configs(accelerator_configs_), network_configs(network_configs_) {
    init();
    std::cout << "# optimizer initialization complete!" << std::endl;
}

optimizer_t::~optimizer_t() {
    delete accelerator;
    for(size_t i = 0; i < mapping_tables.size(); i++) {
        delete mapping_tables.at(i).second;
    }
}

void optimizer_t::init() {
    accelerator = new accelerator_t(accelerator_configs);
    network_name = network_configs.network_name;
    for(size_t i = 0; i < network_configs.layers.size(); i++) {
        // Layer parameter values
        mapping_table_t *tmp_mt = new mapping_table_t;
        std::copy(network_configs.layers.at(i).layer_vals.begin(), 
                  network_configs.layers.at(i).layer_vals.end() - 1, 
                  tmp_mt->layer_vals.begin());
        tmp_mt->stride = network_configs.layers.at(i).layer_vals.at(7);
        tmp_mt->L0_dataflow = accelerator->L0_dataflow;
        tmp_mt->L1_dataflow = accelerator->L1_dataflow;
        tmp_mt->L2_dataflow = accelerator->L2_dataflow;
        tmp_mt->update();
        mapping_tables.push_back(std::make_pair(network_configs.layers.at(i).name, tmp_mt)); 
    }
}

void optimizer_t::print_stats(unsigned idx_) {
    print_accelerator();
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

void optimizer_t::print_accelerator() {
     accelerator->print_stats(); 
}

void optimizer_t::print_mapping_tables(unsigned idx_) {
    mapping_tables.at(idx_).second->print_stats();
}

void optimizer_t::optimize() {
//    for(size_t idx = 0; idx < mapping_tables.size(); idx++) {
//        /* PE array optimization */
//        unsigned map_x = static_cast<unsigned>(accelerator->array_map_x);
//        unsigned map_y = static_cast<unsigned>(accelerator->array_map_y);
//        unsigned expanded_x = 0;
//        unsigned expanded_y = 0;
//
//        if(accelerator->array_size_x <= mapping_tables.at(idx).second->layer_vals.at(map_x)) {
//            mapping_tables.at(idx).second->put_val(accelerator->array_map_x, component_t::X, accelerator->array_size_x);
//            // TODO: eyeriss conv2 
//        }
//        else {
//            mapping_tables.at(idx).second->put_val(accelerator->array_map_x, component_t::X, mapping_tables.at(idx).second->layer_vals.at(map_x));
//            expanded_x = accelerator->array_size_x / mapping_tables.at(idx).second->layer_vals.at(map_x);
//            if(expanded_x >= 2) {
//                // TODO: accelerator folding possible
//                mapping_tables.at(idx).second->expand(component_t::X, expanded_x);
//            }
//        }
//        if(accelerator->array_size_y <= mapping_tables.at(idx).second->layer_vals.at(map_y)) {
//            mapping_tables.at(idx).second->put_val(accelerator->array_map_y, component_t::Y, accelerator->array_size_y);
//            // TODO: eyeriss conv2 
//        }
//        else {
//            mapping_tables.at(idx).second->put_val(accelerator->array_map_y, component_t::Y, mapping_tables.at(idx).second->layer_vals.at(map_y) );
//            expanded_y = accelerator->array_size_y / mapping_tables.at(idx).second->layer_vals.at(map_y);
//            if(expanded_y >= 2) {
//                // TODO: accelerator folding possible
//                mapping_tables.at(idx).second->expand(component_t::Y, expanded_y);
//            }
//        }
//        /* MAC unit optimization */
//
//        /* L1 optimization */
//        if(accelerator->L1_stationary == "IS") {
//
//        }
//        else if(accelerator->L1_stationary == "WS") {
//
//        }
//        else if(accelerator->L1_stationary == "OS") {
//
//        }
//        else
//            handler.print_err(err_type_t::INVAILD, "L1 stationary in optimizer_t::optimize()");
//
//        /* L2 optimization */
//
//        float utx = float(mapping_tables.at(idx).second->get_row_product(component_t::X)) / accelerator->array_size_x;
//        float uty = float(mapping_tables.at(idx).second->get_row_product(component_t::Y)) / accelerator->array_size_y;
//        std::cout << "\n# " << mapping_tables.at(idx).first
//                  << "\n# PE array utilization: " << utx * uty * 100 << "%" << std::endl;
//        // Print mapping table
//        mapping_tables.at(idx).second->print_stats();
//    }
}
