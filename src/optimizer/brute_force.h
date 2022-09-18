#ifndef __BRUTE_FORCE_SEARCH__
#define __BRUTE_FORCE_SEARCH__

#include <mutex>

#include "optimizer.h"

class brute_force_t : public optimizer_t {
public:
    brute_force_t(const std::string& accelerator_pth_,
                  const std::string& dataflow_,
                  const std::string& network_pth_,
                  const std::string& layer_,
                  const std::string& metric_,
                  const std::string& thread_);
    ~brute_force_t();
    // Run brute-force search for all layers
    void run();
    // Run brute-force search for a layer
    void run(const unsigned idx_);
    // Print results
    void print_results();
    void print_results(unsigned idx_);

private:
    // Reset all variables
    void reset(); 
    // Optimize for a given dataflow combination
    void engine();
    // Update the optimal scheduling table
    void update();
    // Search for an optimal mapping option for a given dataflow
    void search(unsigned tid_,
             unsigned begin_pos_,
             unsigned end_pos_,
             mapping_space_t& mapping_space_,
             std::mutex& m_);

    metric_t metric;    // Optimization metric
    unsigned num_threads;    // Num. threads to be used
    
    // Best scheduling option for all dataflow
    scheduling_table_t              global_best_scheduling_option;
    // Best scheduling option for a given dataflow
    std::vector<scheduling_table_t> best_scheduling_option;
    float global_best_cost;

};
#endif
