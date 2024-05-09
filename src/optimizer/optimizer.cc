#include "optimizer.h"

optimizer_t::optimizer_t(const std::string& accelerator_pth_,
                         const std::string& dataflow_,
                         const std::string& network_pth_,
                         const std::string& layer_) 
    : accelerator(new accelerator_t(accelerator_pth_)),
      network(new network_t(network_pth_)) {
    
    // Print out accelerator specifications
    accelerator->print_spec();
    // Init DNN configuration
    network->init_network();
    // Init scheduling table
    scheduling_table = new scheduling_table_t(accelerator, network);
    scheduling_table->init();

    is_fixed  = dataflow_.compare("fixed") == 0 ? true : false;
    if(!layer_.empty()) { layer_idx = stoi(layer_); }       // If layer_ is empty, run all layers
}
optimizer_t::~optimizer_t() {
    delete accelerator;
    delete network;
    delete scheduling_table;
}
// Run optimizer for all layers
void optimizer_t::run() { return; }
// Run optimizer for target layer
void optimizer_t::run(const unsigned idx_) { return; }
// Print results on termianl
void optimizer_t::print_results() { return; }
// Count total number of component level to fill out
unsigned optimizer_t::get_num_targeted_levels(unsigned begin_pos_, 
                                              unsigned end_pos_) {
    unsigned num_targeted_level = 1;
    // From begin_pos_ to end_pos_, count the number of actually exist levels.
    for(unsigned i = begin_pos_; i < end_pos_; i++) {
        if(!scheduling_table->is_skippable(i)) num_targeted_level++;
    }
    return num_targeted_level;
}
// Generate all possible dataflow combination for the target accelerator
std::vector<std::vector<dataflow_t>> optimizer_t::generate_dataflow_combinations() {
    std::vector<std::vector<dataflow_t>> dataflow_combinations;
    std::vector<dataflow_t> combination;
    if(is_fixed) {
        // Traverse temporal rows and collect dataflow of each component
        for(unsigned i = 0; i < scheduling_table->get_num_rows() - 1; i++) {
            if(scheduling_table->get_component_type(i) == reuse_t::TEMPORAL
            && scheduling_table->get_dataflow(i) != dataflow_t::NONE) {
                combination.push_back(scheduling_table->get_dataflow(i));
            }
        }
        dataflow_combinations.push_back(combination);
        combination.clear();
    }
    else {
        std::vector<dataflow_t> possible_dataflows;
        bool* bypass;
        data_t stationary_data;
        
        unsigned permutation_cnt   = 0;
        unsigned permutation_bound = 1;
        
        std::vector<std::vector<dataflow_t>> temp;
        // Traverse rows of scheduling table 
        // and Reorganize possible dataflows that each component can have 
        for(unsigned i = 0; i < scheduling_table->get_num_rows() - 1; i++) {
            if(scheduling_table->get_component_type(i) == reuse_t::TEMPORAL
            && scheduling_table->get_component_name(i) != "virtual") {
                for(unsigned df = (unsigned)dataflow_t::IS; 
                             df < (unsigned)dataflow_t::SIZE; 
                             df++) {
                    if(scheduling_table->get_dataflow(i) == dataflow_t::NONE) {
                        possible_dataflows.push_back(dataflow_t::NONE);
                        break;
                    }
                    else {
                        bypass = scheduling_table->get_bypass(i);
                        stationary_data = (data_t)(df-1);
                        if(!bypass[(unsigned)stationary_data]) {
                            possible_dataflows.push_back((dataflow_t)df);
                        }
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
