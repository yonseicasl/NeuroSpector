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
    void run();
    void run(const unsigned idx_);
    void print_stats();

    void reset(); 
    void engine();
    void update();
    void search(unsigned tid_,
             unsigned begin_pos_,
             unsigned end_pos_,
             mapping_space_t& mapping_space_,
             std::mutex& m_);
    unsigned get_num_targeted_levels(unsigned begin_pos_, unsigned end_pos_);

private:
    metric_type_t metric = metric_type_t::ENERGY;
    unsigned num_threads = 1;
    
    std::vector<scheduling_table_t> local_best_scheduling_scheme;
    scheduling_table_t              global_best_scheduling_scheme;
    float global_best_cost;

};
#endif
