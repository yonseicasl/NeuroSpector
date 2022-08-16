#include <float.h>
#include <unistd.h>
#include <thread>

#include "brute_force.h"


brute_force_t::brute_force_t(const std::string& accelerator_pth_,
                         const std::string& dataflow_,
                         const std::string& network_pth_,
                         const std::string& layer_,
                         const std::string& metric_,
                         const std::string& thread_)
    : optimizer_t(accelerator_pth_, dataflow_, network_pth_, layer_),
      metric(metric_type_t::ENERGY),
      num_threads(1) {
          std::cerr << "[message] construct brute_force class" << std::endl;
          // Init metric
          metric = (metric_type_t)get_enum_type(metric_str, metric_);
          // Init num threads
          if(!thread_.empty()) { num_threads = stoi(thread_); }       
}

brute_force_t::~brute_force_t() {

};
// Run brute-force search for all layers
void brute_force_t::run() {
    std::cerr << "[message] Run brute-force search for all layers" << std::endl;
    assert(layer_idx <= network->get_num_layers());
    for(unsigned idx = 0; idx < network->get_num_layers(); idx++) {
        if(layer_idx != 0) { if((idx + 1) != layer_idx) continue; }
        run(idx);
    }
    
}
// Run brute-force search for a layer
void brute_force_t::run(const unsigned idx_) {
    global_best_cost = FLT_MAX;
    std::cerr << "[message] Run brute-force search for a layer #" << idx_+1 << " ..."<< std::endl;
    scheduling_table->load_dnn_layer(idx_); // update target layer info.
    // Generate for all possible dataflow combinations
    dataflow_combinations = generate_dataflow_combinations();
    // Iterate for all possible dataflow combinations
    auto it = dataflow_combinations.begin();
    while(it != dataflow_combinations.end()) {
        scheduling_table->update_dataflow(*it);
        reset();
        engine();
        update();
        ++it;
    }
    print_results();
}
void brute_force_t::print_results() {
    analyzer_t analyzer(accelerator, network);
    analyzer.init(global_best_scheduling_option);
    analyzer.estimate_cost();
    analyzer.print_stats();
}
void brute_force_t::reset() {
    // Reset all variables
    best_scheduling_option.clear();
}
void brute_force_t::engine() {
    std::cerr << "[message] Engine start" << std::endl;
    unsigned begin_pos, end_pos;
    unsigned num_targeted_levels;
    std::vector<unsigned> undetermined_values;
    static mapping_space_t mapping_space;

    end_pos   = scheduling_table->get_num_rows() - 1;
    begin_pos = 0;

    // Get num targeted levels
    num_targeted_levels = get_num_targeted_levels(begin_pos, end_pos);
    // Update mapping_values
    undetermined_values = scheduling_table->get_row_values(end_pos);
    // Generate mapping space for target set
    mapping_space.generate(num_targeted_levels, undetermined_values);
    // Find optimal scheduling options
    std::vector<std::thread> workers;
    std::mutex m;
    for(unsigned tid = 0; tid < num_threads; tid++) {
        workers.push_back(std::thread(&brute_force_t::search,
                                      this,
                                      tid,
                                      begin_pos,
                                      end_pos,
                                      std::ref(mapping_space),
                                      std::ref(m)));
    }
    for(unsigned tid = 0; tid < num_threads; tid++) {
        workers.at(tid).join();
    }
    // Clear 
    workers.clear();
    mapping_space.clear();
    
    return;
}
void brute_force_t::update() {
    std::cerr << "[message] Update start " << num_threads << std::endl;
    float curr_cost;
    // Update the optimal scheduling option for each dataflow
    // Traverse all optimal scheduling schemes for all threads
    assert(best_scheduling_option.size() == num_threads);
    for(auto it = best_scheduling_option.begin(); it != best_scheduling_option.end(); ++it) {
        analyzer_t analyzer(accelerator, network);
        analyzer.init(*it);
        analyzer.estimate_cost();
        curr_cost = analyzer.get_total_cost(metric);
        analyzer.reset();
        if(curr_cost < global_best_cost) { 
            global_best_cost = curr_cost; 
            global_best_scheduling_option = *it;
        }
    }
}
void brute_force_t::search(unsigned tid_,
                           unsigned begin_pos_,
                           unsigned end_pos_,
                           mapping_space_t& mapping_space_,
                           std::mutex& m_) {
    float best_cost = FLT_MAX;
    float curr_cost = FLT_MAX;
    scheduling_table_t local_scheduling_option(*scheduling_table);
    scheduling_table_t best_local_scheduling_option(*scheduling_table);
    mapping_space_t local_mapping_space;
    std::vector<std::vector<unsigned>> mapping_values_set;
    
    accelerator_t *m_accelerator = new accelerator_t(*accelerator);
    analyzer_t analyzer(m_accelerator, network);

    // Take mapping candidates assigned to thread ID 
    local_mapping_space = mapping_space_.partition_off(tid_, num_threads);
    while(!local_mapping_space.is_last()) { 
        // Get new mapping values
        mapping_values_set = local_mapping_space.get_mapping_set();
        // Update mapping values of scheduling table
        local_scheduling_option.update_set_of_rows(begin_pos_, end_pos_, mapping_values_set);
        // Load scheduling table to analyzer
        analyzer.init(local_scheduling_option);
        // Check_Validity
        if(!analyzer.check_validity()) continue;
        // Evaluate scheduling tables cost
        analyzer.estimate_cost();
        // Get analyzer's cost
        curr_cost = analyzer.get_total_cost(metric);
        if(curr_cost < best_cost) {
            best_cost = curr_cost; 
            best_local_scheduling_option = local_scheduling_option;
        }
        // Compare DRAM iteration
        else if (curr_cost == best_cost) {
            // Get DRAM level access count
            unsigned iter_curr = 1, iter_best = 1;
            std::vector<unsigned> row_curr = local_scheduling_option.get_row_values(end_pos_);
            std::vector<unsigned> row_best   = best_local_scheduling_option.get_row_values(end_pos_);
            for(auto value = row_curr.begin(); value != row_curr.end(); ++value) {
                iter_curr *= *value;
            }
            for(auto value = row_best.begin(); value != row_best.end(); ++value) {
                iter_best *= *value;
            }
            // Compare DRAM iteration
            if(row_curr < row_best) {
                best_local_scheduling_option = local_scheduling_option;
            }
        }
        // Reset analyzer's variables
        analyzer.reset();
    }
    m_.lock();
    best_scheduling_option.push_back(best_local_scheduling_option);
    m_.unlock();
}