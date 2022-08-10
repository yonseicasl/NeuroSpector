#include "optimizer.h"

optimizer_t::optimizer_t(const std::string& accelerator_pth_,
                         const std::string& dataflow_,
                         const std::string& network_pth_,
                         const std::string& layer_) 
    : accelerator(new accelerator_t(accelerator_pth_)),
      network(new network_t(network_pth_)) {
    
    // Init accelerator configuration
    accelerator->init_accelerator();
    accelerator->print_spec();
    // Init DNN configuration
    network->init_network();
    // Init scheduling table
    scheduling_table = new scheduling_table_t(accelerator, network);
    scheduling_table->init();

    is_fixed  = dataflow_.compare("fixed") == 0 ? true : false;
    if(!layer_.empty()) { layer_idx = stoi(layer_); }       // If layer_ is empty, run all layers
    std::cout << "[message] end optimzier constructor" << std::endl;
}

optimizer_t::~optimizer_t() {
    delete accelerator;
    delete network;
    delete scheduling_table;
}

void optimizer_t::run() { return; }
void optimizer_t::run(const unsigned idx_) { return; }
void optimizer_t::print_stats() { return; }

std::vector<std::vector<dataflow_t>> optimizer_t::generate_dataflow_combinations() {
    std::vector<std::vector<dataflow_t>> dataflow_combinations;
    std::vector<dataflow_t> combination;
    unsigned component_idx = 0;
    if(is_fixed) {
        // Traverse temporal rows and collect dataflow of each component
        for(unsigned i = 0; i < scheduling_table->get_num_rows() - 1; i++) {
            if(scheduling_table->get_component_type(i) == component_type_t::TEMPORAL
            && scheduling_table->get_component_name(i) != "virtual") {
                component_idx = scheduling_table->get_component_index(i);
                combination.push_back(accelerator->get_dataflow(component_idx));
            }
        }
        dataflow_combinations.push_back(combination);
        combination.clear();
    }
    else {
        std::vector<dataflow_t> possible_dataflows;
        std::vector<data_type_t> bypass;
        data_type_t stationary_data;
        
        unsigned permutation_cnt   = 0;
        unsigned permutation_bound = 1;
        
        std::vector<std::vector<dataflow_t>> temp;
        // Traverse rows of scheduling table 
        // and Reorganize possible dataflows that each component can have 
        for(unsigned i = 0; i < scheduling_table->get_num_rows() - 1; i++) {
            if(scheduling_table->get_component_type(i) == component_type_t::TEMPORAL
            && scheduling_table->get_component_name(i) != "virtual") {
                for(unsigned df = (unsigned)dataflow_t::IS; 
                             df < (unsigned)dataflow_t::SIZE; 
                             df++) {
                    component_idx = scheduling_table->get_component_index(i);
                    if(accelerator->get_dataflow(component_idx) == dataflow_t::NONE) {
                        possible_dataflows.push_back(dataflow_t::NONE);
                        break;
                    }
                    else {
                        bypass = accelerator->get_bypass(component_idx);
                        stationary_data = (data_type_t)(df-1);
                        auto exist = find(bypass.begin(), bypass.end(), 
                                          stationary_data);
                        if(exist != bypass.end()) { continue; }
                        possible_dataflows.push_back((dataflow_t)df);
                    }
                }
                temp.push_back(possible_dataflows);
                possible_dataflows.clear();
            }
        }
        // Compute permutation_bound
        for(auto it = temp.begin(); it != temp.end(); ++it) {
            permutation_bound *= it->size();
        }
        std::vector<unsigned> indices(temp.size(), 0);
        while(permutation_cnt < permutation_bound) {
            // Generate new dataflow combination
            for(unsigned i = 0; i < temp.size(); i++) {
                std::vector<dataflow_t> components_dataflow = temp.at(i);
                combination.push_back(components_dataflow.at(indices.at(i)));
            }
            // Update the number of dataflow combinations
            dataflow_combinations.push_back(combination);
            // clear combination
            combination.clear();
            // Change indices
            indices.at(0)++;
            assert(indices.size() == temp.size());
            for(unsigned i = 0; i < temp.size() - 1; i++) {
                if(indices.at(i) > temp.at(i).size()  -1) {
                    indices.at(i) = 0;
                    indices.at(i + 1)++;
                }
            }
            permutation_cnt++;
        }
    }
    return dataflow_combinations;
}