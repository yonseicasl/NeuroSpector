#ifndef __BOTTOM_UP_SEARCH__
#define __BOTTOM_UP_SEARCH__

#include <mutex>

#include "optimizer.h"

class bottom_up_t : public optimizer_t {
public:
    struct StrategyContainer {
        std::vector<strategy_t> strategy;
        scheduling_table_t scheduling_table;
    };

    bottom_up_t(const std::string& accelerator_pth_,
                const std::string& dataflow_,
                const std::string& network_pth_,
                const std::string& layer_,
                const std::string& batch_size_,
                const std::string& metric_,
                const std::string& cl_optimization_);
    ~bottom_up_t();
    // Run bottom-up search for all layers
    void run();
    // Run optimal scheduling option that processing multiple layers in parallel.
    void run(const std::vector<unsigned> indices_);  
    // Run bottom-up search for a layer
    void run(const unsigned idx_);

private:
    // Print results
    void print_results();
    void print_results(unsigned idx_);
    // Print multi-chip partitioning results
    void print_mcp_results();
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
                StrategyContainer& scheduling_candidate_,
                std::vector<StrategyContainer>& output_candidates_,
                std::mutex& m_);
    // Find optimal scheudling option based on primary strategy 
    void optimize_with_primary_strategy(unsigned end_pos_,
                                        analyzer_t& analyzer_,
                                        float& curr_cost,
                                        float& lowest_cost_,
                                        scheduling_table_t& curr_table_,
                                        scheduling_table_t& pm_table_);
    // Find optimal scheudling option based on supplementary strategy 
    void optimize_with_supplementary_strategy(unsigned begin_pos_,
                                              unsigned end_pos_,
                                              analyzer_t& analyzer_,
                                              float& curr_cost,
                                              float& lowest_cost_,
                                              unsigned& best_opportunity_,
                                              scheduling_table_t& curr_table_,
                                              scheduling_table_t& sp_table_);
    // Find the optimal way to process layers which are branched off the same root layer in parallel
    void multi_chip_partitioning(std::vector<scheduling_table_t>& tables_);
    // Collect all possible partitioning cases for a layer
    std::vector<PartitioningInfo> collect_partition_comb(scheduling_table_t table_); 
    float best_cost_of_multiple_layers; 

    metric_t metric;                               // Optimization metric
    bool     is_cross_layer_opt;                        // Optimize for network-level or not
   
    StrategyContainer global_best_scheduling_option;    // Best scheduling option for all dataflow
    StrategyContainer best_scheduling_option;           // Best scheduling option for a given dataflow
};
#endif
