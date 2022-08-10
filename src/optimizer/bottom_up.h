#ifndef __BOTTOM_UP_SEARCH__
#define __BOTTOM_UP_SEARCH__

#include <mutex>

#include "optimizer.h"

class bottom_up_t : public optimizer_t {
public:
    struct StrategyContainer {
        std::vector<strategy_type_t> strategy;
        scheduling_table_t scheduling_table;
    };

    bottom_up_t(const std::string& accelerator_pth_,
                const std::string& dataflow_,
                const std::string& network_pth_,
                const std::string& layer_,
                const std::string& metric_,
                const std::string& cl_optimization_,
                const std::string& thread_);
    ~bottom_up_t();
    void run();
    void run(const unsigned idx_);
    void run(const std::vector<unsigned> indices_);  // Multi-chip parititioning
    void print_stats();

    void reset(); 
    void engine();
    void update();
    void search(unsigned tid_,
                unsigned begin_pos_,
                unsigned end_pos_,
                StrategyContainer& scheduling_candidate_,
                std::vector<StrategyContainer>& output_candidates_,
                std::mutex& m_);
    unsigned get_num_targeted_levels(unsigned begin_pos_, unsigned end_pos_);
    
    void optimize_with_primary_strategy(unsigned end_pos_,
                                        analyzer_t& analyzer_,
                                        float& best_cost_,
                                        scheduling_table_t& curr_table_,
                                        scheduling_table_t& pm_table_);
    void optimize_with_supplementary_strategy(unsigned begin_pos_,
                                              unsigned end_pos_,
                                              analyzer_t& analyzer_,
                                              unsigned& best_opportunity_,
                                              scheduling_table_t& curr_table_,
                                              scheduling_table_t& sp_table_);
    void multi_chip_partitioning(std::vector<scheduling_table_t>& tables_);
    std::vector<PartitioningInfo> collect_partition_comb(scheduling_table_t table_); 
private:
    metric_type_t metric = metric_type_t::ENERGY;
    unsigned num_threads;
    bool     is_cross_layer_opt;
   
    StrategyContainer global_best_scheduling_option;
    StrategyContainer best_scheduling_option;
};
#endif
