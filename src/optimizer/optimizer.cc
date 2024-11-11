#include "optimizer.h"

optimizer_t::optimizer_t(const std::string& accelerator_pth_,
                         const std::string& dataflow_,
                         const std::string& network_pth_,
                         const std::string& layer_,
                         const std::string& batch_size_) 
    : accelerator(new accelerator_t(accelerator_pth_)),
      network(new network_t(network_pth_)) {
    
    // Print out accelerator specifications
    accelerator->print_spec();
    // Init DNN configuration
    network->init_network();
    if(batch_size_ != "1") { 
        network->update_batch_size(batch_size_);
    }
    // Init scheduling table
    scheduling_table = new scheduling_table_t(accelerator, network);
    scheduling_table->init();
    is_fixed  = dataflow_.compare("flexible") == 0 ? false : true;

    // If the dataflow is specified in command line.
    if(is_fixed && !(dataflow_.compare("fixed") == 0)) {
        // Convert string value to dataflow set
        std::vector<std::string> string_dataflow = split(dataflow_, '-');
        std::vector<dataflow_t>  target_dataflow;

        // Change the table's dataflow set to the specified dataflow set
        for(unsigned i = 0; i < string_dataflow.size(); ++i) {
            std::string df_string = lowercase(string_dataflow.at(i));
            dataflow_t  df = (dataflow_t)get_enum_type(dataflow_str, df_string);
            if(df > dataflow_t::SIZE) {
                std::cerr << "Invalid dataflow setting; valid setting example (ws-os)" << std::endl;
                exit(0);
            }
            target_dataflow.push_back(df);
        }
        // Check whether the #dataflows is equal to valid temporal comp.
        if(target_dataflow.size() == scheduling_table->get_num_valid_temp_rows()) {
            scheduling_table->update_dataflow(target_dataflow);
        }
        else { std::cerr << "[Error] dataflow config isn't matched with the " 
                         << "valid temporal components. Check the accelerator config. "
                         << "or the dataflow arg." << std::endl; 
               exit(false); 
        }
    }
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
// Count total number of `spatial` component level to fill out
unsigned optimizer_t::get_num_spatial_levels(unsigned begin_pos_, 
                                             unsigned end_pos_) {
    unsigned num_targeted_level = 1;
    // From begin_pos_ to end_pos_ count the number of actually exist spatial levels.
    for(unsigned i = begin_pos_; i < end_pos_; i++) {
        if(!scheduling_table->is_skippable(i) && scheduling_table->is_spatial(i)) 
            num_targeted_level++;
    }
    return num_targeted_level;
}
// Count total number of `temporal` component level to fill out
unsigned optimizer_t::get_num_temporal_levels(unsigned begin_pos_, 
                                              unsigned end_pos_) {
    unsigned num_targeted_level = 1;
    // From begin_pos_ to end_pos_ count the number of actually exist temporal levels.
    for(unsigned i = begin_pos_; i < end_pos_; i++) {
        if(!scheduling_table->is_skippable(i) && !scheduling_table->is_spatial(i)) 
            num_targeted_level++;
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
// Check whether the current layer configuration has ever been considered before
unsigned optimizer_t::is_dulicated(scheduling_table_t* scheduling_table_) {
    std::cout << "[message] Check duplication of layer config." << std::endl;
    std::vector<unsigned> layer_params = scheduling_table_->get_layer_parameters();
    
    // Traverse the optimization history and find the similar one
    unsigned duplicated_layer = search_history(layer_params);
    if(duplicated_layer > 0) { 
        return duplicated_layer;
    }
    return 0;
}
// Search the optimization history and find the same layer configuration
unsigned optimizer_t::search_history(std::vector<unsigned> layer_params_) {
    for(auto it = optimization_history.begin(); it != optimization_history.end(); ++it) {
        if(it->second == layer_params_) {
            return it->first;
        }
    } 
    return 0;
}
// Record the current layer configuration
void optimizer_t::record_layer_configuration(scheduling_table_t* scheduling_table_) {
    std::vector<unsigned> layer_params; 
    unsigned              layer_index = scheduling_table_->get_layer_index();
    layer_params = scheduling_table_->get_layer_parameters();

    std::cout << "[message] Record layer config. [layer: " << scheduling_table_->get_layer_index() + 1 << ", ";
    for(auto it = layer_params.begin(); it != layer_params.end(); ++it) {
        std::cout << *it << ", ";
    }
    std::cout << "]" << std::endl;

    // Record the parameter
    optimization_history.insert(std::make_pair(layer_index+1, layer_params));
    
    return;
}
