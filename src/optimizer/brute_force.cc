#include "optimizer.h"

static handler_t handler;

/* Brute-force */
brute_force_t::brute_force_t(const std::string& acc_cfg_path_, 
                             const std::string& net_cfg_path_,
                             const opt_type_t opt_type_,
                             const unsigned num_threads_,
                             const bool is_fixed_)
    : optimizer_t(acc_cfg_path_, net_cfg_path_, is_fixed_),
      opt_type(opt_type_), 
      num_threads(num_threads_),
      total_cnt(0), 
      valid_cnt(0) ,
      final_best_stat(DBL_MAX) {

}             

brute_force_t::~brute_force_t() {
    
}

// Optimizer APIs
void brute_force_t::run() {
    for(unsigned idx = 0; idx < mappings.size(); idx++) 
        run(idx + 1);
    return;
}

void brute_force_t::run(const unsigned idx_) {
    handler.print_line(50, "*");
    switch(opt_type) {
        case opt_type_t::B_F_ENERGY: std::cout << "# ENERGY-DELAY OPTIMIZATION" << std::endl; break;
        case opt_type_t::B_F_CYCLE: std::cout << "# DELAY-ENERGY OPTIMIZATION" << std::endl; break;
        case opt_type_t::B_F_EDP: std::cout << "# EDP OPTIMIZATION" << std::endl; break;
        default: break;
    }
    std::cout << "# NETWORK    : " << network_name << std::endl;
    std::cout << "# LAYER      : " << mappings.at(idx_ - 1).get_layer_name() << std::endl;
    handler.print_line(50, "*");
    // Start optimizing
    for(unsigned l1_df = 0; l1_df < l1_dataflows.size(); l1_df++) {
        for(unsigned l2_df = 0; l2_df < l2_dataflows.size(); l2_df++) {
            // Reset variables & containers
            reset(idx_);
            // Run the engine
            engine(idx_, 
                   static_cast<dataflow_t>(l1_dataflows.at(l1_df)), 
                   static_cast<dataflow_t>(l2_dataflows.at(l2_df)));
            // Sync and update
            sync_and_update(static_cast<dataflow_t>(l1_dataflows.at(l1_df)), 
                            static_cast<dataflow_t>(l2_dataflows.at(l2_df)));
            // TODO: print stats per dataflow
        }
    }
    // Print stats
#ifdef CSV
    print_csv();
#else
    print_stats();
#endif
    return;
}

void brute_force_t::print_stats() {
    std::string df_str("IWO");
    std::cout << "# NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << valid_cnt
              << " (" << std::fixed << std::setprecision(2) 
              << float(valid_cnt) / total_cnt * 100 << "%)" << std::endl; 
    std::cout << "# TOTAL NUM OF MAPPINGS  : " 
              << std::setw(15) << total_cnt
              << " (100%)" << std::endl;
    std::cout << "# NUM OF SIMILAR MAPPINGS : " 
              << std::setw(15) << final_best_mappings.size() << std::endl;
    handler.print_line(50, "*");
    for(unsigned i = 0; i < final_best_mappings.size(); i++) {
        std::cout << "# BEST MAPPING TABLE " << i + 1 << std::endl;
        std::cout << "# DATAFLOWS (L1-L2): " 
                  << df_str.at(static_cast<unsigned>(final_best_dataflows.at(0))) 
                  << "S-" 
                  << df_str.at(static_cast<unsigned>(final_best_dataflows.at(1))) 
                  << "S" << std::endl; 
        final_best_mappings.at(i).print_stats();
        stats_t stats_tmp(accelerator, final_best_mappings.at(i), final_best_dataflows.at(0), final_best_dataflows.at(1));
        stats_tmp.update_stats();
        stats_tmp.print_stats();
        handler.print_line(50, "-");
    }
    return;
}

void brute_force_t::print_csv() {
    std::string df_str("IWO");
    std::cout << "VALID MAPPINGS,TOTAL MAPPINGS,SIMILAR MAPPINGS\n"  
              << valid_cnt << "," 
              << total_cnt << "," 
              << final_best_mappings.size() << std::endl;
    handler.print_line(50, "*");
    for(unsigned i = 0; i < final_best_mappings.size(); i++) {
        std::cout << "# BEST MAPPING TABLE " << i + 1 << std::endl;
        std::cout << "# DATAFLOWS (L1-L2): " 
                  << df_str.at(static_cast<unsigned>(final_best_dataflows.at(0))) 
                  << "S-" 
                  << df_str.at(static_cast<unsigned>(final_best_dataflows.at(1))) 
                  << "S" << std::endl; 
        final_best_mappings.at(i).print_csv();
        stats_t stats_tmp(accelerator, final_best_mappings.at(i), final_best_dataflows.at(0), final_best_dataflows.at(1));
        stats_tmp.update_stats();
        stats_tmp.print_csv();
        handler.print_line(50, "-");
    }
    return;
}

// Optimizer private functions
void brute_force_t::reset(const unsigned idx_) {
    // Reset valid count
    valid_cnt = 0;
    // Reset best_mappings
    best_mappings.clear();
    best_mappings.assign(num_threads, std::make_pair(DBL_MAX, mapping_table_t(mappings.at(idx_ - 1))));
    // Reset similar_mappings
    similar_mappings.clear();
    std::vector<mapping_table_t> mapping_tmp(1, mapping_table_t(mappings.at(idx_ - 1)));
    similar_mappings.assign(num_threads, mapping_tmp);
    return;
}

void brute_force_t::engine(const unsigned idx_,
                           const dataflow_t l1_dataflow_, 
                           const dataflow_t l2_dataflow_) {    
    // Mapping space generation
    static unsigned prev_idx = -1;
    static mapping_space_t mapping_space;
    if(prev_idx != idx_) {
        mapping_space = mapping_space_t(num_levels - 1, mappings.at(idx_ - 1).get_layer_values());
        total_cnt = mapping_space.get_num_permutations();
    }
    prev_idx = idx_;
    // Threads initialization
    std::vector<std::thread> workers;
    std::mutex m;
    // Multi-threading
    for(unsigned tid = 0; tid < num_threads; tid++) {
        switch(opt_type) {
            case opt_type_t::B_F_ENERGY: 
                workers.push_back(std::thread(&brute_force_t::energy_worker, 
                                              this, 
                                              tid, 
                                              std::ref(mappings.at(idx_ - 1)),
                                              std::ref(mapping_space),
                                              l1_dataflow_,
                                              l2_dataflow_,
                                              std::ref(m)));
                break;
            case opt_type_t::B_F_CYCLE: 
                workers.push_back(std::thread(&brute_force_t::cycle_worker, 
                                              this, 
                                              tid, 
                                              std::ref(mappings.at(idx_ - 1)),
                                              std::ref(mapping_space),
                                              l1_dataflow_,
                                              l2_dataflow_,
                                              std::ref(m)));
                break;
            case opt_type_t::B_F_EDP: 
                workers.push_back(std::thread(&brute_force_t::edp_worker, 
                                              this, 
                                              tid, 
                                              std::ref(mappings.at(idx_ - 1)),
                                              std::ref(mapping_space),
                                              l1_dataflow_,
                                              l2_dataflow_,
                                              std::ref(m)));
                break;
            default: break;
        }
    }
    // Thread join
    for(unsigned tid = 0; tid < num_threads; tid++) 
        workers.at(tid).join();
    workers.clear();
    return;
}

void brute_force_t::sync_and_update(const dataflow_t l1_dataflow_,
                                    const dataflow_t l2_dataflow_) {
    double local_min_energy = DBL_MAX;
    double local_min_cycle = DBL_MAX;
    double local_min_edp = DBL_MAX;
    std::vector<mapping_table_t> local_final_best_mappings;
    switch(opt_type) {
        case opt_type_t::B_F_ENERGY: 
            for(unsigned tid = 0; tid < num_threads; tid++) {
                if(local_min_energy > best_mappings.at(tid).first) {
                    local_min_energy = best_mappings.at(tid).first;
                    stats_t stats_tmp(accelerator, best_mappings.at(tid).second, l1_dataflow_, l2_dataflow_);
                    stats_tmp.update_stats();
                    local_min_cycle = stats_tmp.get_total_cycle();
                    local_final_best_mappings.clear();
                    local_final_best_mappings.push_back(best_mappings.at(tid).second);
                    for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                       local_final_best_mappings.push_back(*it);
                }
                else if(local_min_energy == best_mappings.at(tid).first) {
                    stats_t stats_tmp(accelerator, best_mappings.at(tid).second, l1_dataflow_, l2_dataflow_);
                    stats_tmp.update_stats();
                    if(local_min_cycle > stats_tmp.get_total_cycle()) {
                        local_min_cycle = stats_tmp.get_total_cycle();
                        local_final_best_mappings.clear();
                        local_final_best_mappings.push_back(best_mappings.at(tid).second);
                        for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                            local_final_best_mappings.push_back(*it);
                    }
                    else if(local_min_cycle == stats_tmp.get_total_cycle()) {
                        local_final_best_mappings.push_back(best_mappings.at(tid).second);
                        for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                            local_final_best_mappings.push_back(*it);
                    }
                    else {
                        // Nothing to do
                    } 
                }
                else {
                    // Nothing to do
                }
                // Update
                if(final_best_stat > local_min_energy) {
                    final_best_stat = local_min_energy;
                    final_best_mappings.clear();
                    final_best_mappings.assign(local_final_best_mappings.begin(), local_final_best_mappings.end());
                    final_best_dataflows.clear();
                    final_best_dataflows.push_back(l1_dataflow_);
                    final_best_dataflows.push_back(l2_dataflow_);
                }
            }
            break;
        case opt_type_t::B_F_CYCLE: 
            for(unsigned tid = 0; tid < num_threads; tid++) {
                if(local_min_cycle > best_mappings.at(tid).first) {
                    local_min_cycle = best_mappings.at(tid).first;
                    stats_t stats_tmp(accelerator, best_mappings.at(tid).second, l1_dataflow_, l2_dataflow_);
                    stats_tmp.update_stats();
                    local_min_energy = stats_tmp.get_total_energy();
                    local_final_best_mappings.clear();
                    local_final_best_mappings.push_back(best_mappings.at(tid).second);
                    for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                        local_final_best_mappings.push_back(*it);
                }
                else if(local_min_cycle == best_mappings.at(tid).first) {
                    stats_t stats_tmp(accelerator, best_mappings.at(tid).second, l1_dataflow_, l2_dataflow_);
                    stats_tmp.update_stats();
                    if(local_min_energy > stats_tmp.get_total_energy()) {
                        local_min_energy = stats_tmp.get_total_energy();
                        local_final_best_mappings.clear();
                        local_final_best_mappings.push_back(best_mappings.at(tid).second);
                        for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                            local_final_best_mappings.push_back(*it);
                    }
                    else if(local_min_energy == stats_tmp.get_total_energy()) {
                        local_final_best_mappings.push_back(best_mappings.at(tid).second);
                        for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                            local_final_best_mappings.push_back(*it);
                    }
                    else {
                        // Nothing to do
                    } 
                }
                else {
                    // Nothing to do
                }
                // Update
                if(final_best_stat > local_min_cycle) {
                    final_best_stat = local_min_cycle;
                    final_best_mappings.clear();
                    final_best_mappings.assign(local_final_best_mappings.begin(), local_final_best_mappings.end());
                    final_best_dataflows.clear();
                    final_best_dataflows.push_back(l1_dataflow_);
                    final_best_dataflows.push_back(l2_dataflow_);
                }
            }
            break;
        case opt_type_t::B_F_EDP: 
            for(unsigned tid = 0; tid < num_threads; tid++) {
                if(local_min_edp > best_mappings.at(tid).first) {
                    local_min_edp = best_mappings.at(tid).first;
                    stats_t stats_tmp(accelerator, best_mappings.at(tid).second, l1_dataflow_, l2_dataflow_);
                    stats_tmp.update_stats();
                    local_final_best_mappings.clear();
                    local_final_best_mappings.push_back(best_mappings.at(tid).second);
                    for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                        local_final_best_mappings.push_back(*it);
                }
                else if(local_min_edp == best_mappings.at(tid).first) {
                    local_final_best_mappings.push_back(best_mappings.at(tid).second);
                    for(auto it = similar_mappings.at(tid).begin(); it != similar_mappings.at(tid).end(); ++it) 
                        local_final_best_mappings.push_back(*it);
                }
                else {
                    // Nothing to do
                }
                // Update
                if(final_best_stat > local_min_edp) {
                    final_best_stat = local_min_cycle;
                    final_best_mappings.clear();
                    final_best_mappings.assign(local_final_best_mappings.begin(), local_final_best_mappings.end());
                    final_best_dataflows.clear();
                    final_best_dataflows.push_back(l1_dataflow_);
                    final_best_dataflows.push_back(l2_dataflow_);
                }
            }
            break;
        default: break;
    }
    return;
}

void brute_force_t::energy_worker(const unsigned tid_, 
                                  const mapping_table_t& init_mapping_,
                                  const mapping_space_t& mapping_space_,
                                  const dataflow_t l1_dataflow_,
                                  const dataflow_t l2_dataflow_,
                                  std::mutex& m_) {
    // Initialze local variables & containers
    double local_min_energy = DBL_MAX;
    double local_min_cycle = DBL_MAX;
    uint64_t local_valid_cnt = 0;
    mapping_table_t local_best_mapping(init_mapping_); 
    std::vector<mapping_table_t> local_similar_mappings;
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Current mapping table
    mapping_table_t curr_mapping(init_mapping_);
    // Start finding a local optimized mapping
    for(size_t g = range.start_g; g < range.end_g; g++) {
        curr_mapping.put_column_degrees(parameter_t::G, mapping_space_.get_permutations(0).at(g), component_t::MAC, component_t::DRAM);
        for(size_t k = range.start_k; k < range.end_k; k++) {
            curr_mapping.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(1).at(k), component_t::MAC, component_t::DRAM);
            for(size_t b = range.start_b; b < range.end_b; b++) {
                curr_mapping.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(2).at(b), component_t::MAC, component_t::DRAM);
                for(size_t p = range.start_p; p < range.end_p; p++) {
                    curr_mapping.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(3).at(p), component_t::MAC, component_t::DRAM);
                    for(size_t q = range.start_q; q < range.end_q; q++) {
                        curr_mapping.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(4).at(q), component_t::MAC, component_t::DRAM);
                        for(size_t c = range.start_c; c < range.end_c; c++) {
                            curr_mapping.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(5).at(c), component_t::MAC, component_t::DRAM);
                            for(size_t s = range.start_s; s < range.end_s; s++) {
                                curr_mapping.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(6).at(s), component_t::MAC, component_t::DRAM);
                                for(size_t r = range.start_r; r < range.end_r; r++) {
                                    curr_mapping.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(7).at(r), component_t::MAC, component_t::DRAM);
                                    // Find the best mapping table
                                    if(check_all_validity(curr_mapping)) {
                                        stats_t curr_stats(accelerator, curr_mapping, l1_dataflow_, l2_dataflow_);
                                        curr_stats.update_stats();
                                        if(local_min_energy > curr_stats.get_total_energy()) {
                                            local_min_energy = curr_stats.get_total_energy();
                                            local_min_cycle = curr_stats.get_total_cycle();
                                            local_best_mapping.swap_degrees(curr_mapping.get_degrees());
                                            local_similar_mappings.clear();
                                        }
                                        else if(local_min_energy == curr_stats.get_total_energy()) {
                                            if(local_min_cycle > curr_stats.get_total_cycle()) {
                                                local_min_cycle = curr_stats.get_total_cycle();
                                                local_best_mapping.swap_degrees(curr_mapping.get_degrees());
                                                local_similar_mappings.clear();
                                            }
                                            else if(local_min_cycle == curr_stats.get_total_cycle()) {
                                                mapping_table_t similar_mapping_tmp(curr_mapping);
                                                local_similar_mappings.push_back(similar_mapping_tmp);
                                            }
                                            else {
                                                // Nothing to do
                                            }
                                        }
                                        else {
                                            // Nothing to do
                                        }
                                        local_valid_cnt++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Update global
    if(local_valid_cnt > 0) {
        m_.lock();
        valid_cnt += local_valid_cnt;
        m_.unlock();
        best_mappings.at(tid_).first = local_min_energy;
        best_mappings.at(tid_).second.swap_degrees(local_best_mapping.get_degrees());
        similar_mappings.at(tid_).assign(local_similar_mappings.begin(), local_similar_mappings.end());
    }
    return;
}

void brute_force_t::cycle_worker(const unsigned tid_, 
                                 const mapping_table_t& init_mapping_,
                                 const mapping_space_t& mapping_space_,
                                 const dataflow_t l1_dataflow_,
                                 const dataflow_t l2_dataflow_,
                                 std::mutex& m_) {
    // Initialze local variables & containers
    double local_min_cycle = DBL_MAX;
    double local_min_energy = DBL_MAX;
    uint64_t local_valid_cnt = 0;
    mapping_table_t local_best_mapping(init_mapping_); 
    std::vector<mapping_table_t> local_similar_mappings;
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Current mapping table
    mapping_table_t curr_mapping(init_mapping_);
    // Start finding a local optimized mapping
    for(size_t g = range.start_g; g < range.end_g; g++) {
        curr_mapping.put_column_degrees(parameter_t::G, mapping_space_.get_permutations(0).at(g), component_t::MAC, component_t::DRAM);
        for(size_t k = range.start_k; k < range.end_k; k++) {
            curr_mapping.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(1).at(k), component_t::MAC, component_t::DRAM);
            for(size_t b = range.start_b; b < range.end_b; b++) {
                curr_mapping.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(2).at(b), component_t::MAC, component_t::DRAM);
                for(size_t p = range.start_p; p < range.end_p; p++) {
                    curr_mapping.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(3).at(p), component_t::MAC, component_t::DRAM);
                    for(size_t q = range.start_q; q < range.end_q; q++) {
                        curr_mapping.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(4).at(q), component_t::MAC, component_t::DRAM);
                        for(size_t c = range.start_c; c < range.end_c; c++) {
                            curr_mapping.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(5).at(c), component_t::MAC, component_t::DRAM);
                            for(size_t s = range.start_s; s < range.end_s; s++) {
                                curr_mapping.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(6).at(s), component_t::MAC, component_t::DRAM);
                                for(size_t r = range.start_r; r < range.end_r; r++) {
                                    curr_mapping.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(7).at(r), component_t::MAC, component_t::DRAM);
                                    // Find the best mapping table
                                    if(check_all_validity(curr_mapping)) {
                                        stats_t curr_stats(accelerator, curr_mapping, l1_dataflow_, l2_dataflow_);
                                        curr_stats.update_stats();
                                        if(local_min_cycle > curr_stats.get_total_cycle()) {
                                            local_min_cycle = curr_stats.get_total_cycle();
                                            local_min_energy = curr_stats.get_total_energy();
                                            local_best_mapping.swap_degrees(curr_mapping.get_degrees());
                                            local_similar_mappings.clear();
                                        }
                                        else if(local_min_cycle == curr_stats.get_total_cycle()) {
                                            if(local_min_energy > curr_stats.get_total_energy()) {
                                                local_min_energy = curr_stats.get_total_energy();
                                                local_best_mapping.swap_degrees(curr_mapping.get_degrees());
                                                local_similar_mappings.clear();
                                            }
                                            else if(local_min_energy == curr_stats.get_total_energy()) {
                                                mapping_table_t similar_mapping_tmpt(curr_mapping);
                                                local_similar_mappings.push_back(similar_mapping_tmpt);
                                            }
                                            else {
                                                // Nothing to do
                                            }
                                        }
                                        else {
                                            // Nothing to do
                                        }
                                        local_valid_cnt++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Update global
    if(local_valid_cnt > 0) {
        m_.lock();
        valid_cnt += local_valid_cnt;
        m_.unlock();
        best_mappings.at(tid_).first = local_min_cycle;
        best_mappings.at(tid_).second.swap_degrees(local_best_mapping.get_degrees());
        similar_mappings.at(tid_).assign(local_similar_mappings.begin(), local_similar_mappings.end());
    }
    return;
}

void brute_force_t::edp_worker(const unsigned tid_, 
                               const mapping_table_t& init_mapping_,
                               const mapping_space_t& mapping_space_,
                               const dataflow_t l1_dataflow_,
                               const dataflow_t l2_dataflow_,
                               std::mutex& m_) {
    // Initialze local variables & containers
    double local_min_edp = DBL_MAX;
    uint64_t local_valid_cnt = 0;
    mapping_table_t local_best_mapping(init_mapping_); 
    std::vector<mapping_table_t> local_similar_mappings;
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Current mapping table
    mapping_table_t curr_mapping(init_mapping_);
    // Start finding a local optimized mapping
    for(size_t g = range.start_g; g < range.end_g; g++) {
        curr_mapping.put_column_degrees(parameter_t::G, mapping_space_.get_permutations(0).at(g), component_t::MAC, component_t::DRAM);
        for(size_t k = range.start_k; k < range.end_k; k++) {
            curr_mapping.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(1).at(k), component_t::MAC, component_t::DRAM);
            for(size_t b = range.start_b; b < range.end_b; b++) {
                curr_mapping.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(2).at(b), component_t::MAC, component_t::DRAM);
                for(size_t p = range.start_p; p < range.end_p; p++) {
                    curr_mapping.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(3).at(p), component_t::MAC, component_t::DRAM);
                    for(size_t q = range.start_q; q < range.end_q; q++) {
                        curr_mapping.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(4).at(q), component_t::MAC, component_t::DRAM);
                        for(size_t c = range.start_c; c < range.end_c; c++) {
                            curr_mapping.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(5).at(c), component_t::MAC, component_t::DRAM);
                            for(size_t s = range.start_s; s < range.end_s; s++) {
                                curr_mapping.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(6).at(s), component_t::MAC, component_t::DRAM);
                                for(size_t r = range.start_r; r < range.end_r; r++) {
                                    curr_mapping.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(7).at(r), component_t::MAC, component_t::DRAM);
                                    // Find the best mapping table
                                    if(check_all_validity(curr_mapping)) {
                                        stats_t curr_stats(accelerator, curr_mapping, l1_dataflow_, l2_dataflow_);
                                        curr_stats.update_stats();
                                        if(local_min_edp > curr_stats.get_total_edp()) {
                                            local_min_edp = curr_stats.get_total_edp();
                                            local_best_mapping.swap_degrees(curr_mapping.get_degrees());
                                            local_similar_mappings.clear();
                                        }
                                        else if(local_min_edp == curr_stats.get_total_edp()) {
                                            mapping_table_t similar_mapping_tmp(curr_mapping);
                                            local_similar_mappings.push_back(similar_mapping_tmp);
                                        }
                                        else {
                                            // Nothing to do
                                        }
                                        local_valid_cnt++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Update global
    if(local_valid_cnt > 0) {
        m_.lock();
        valid_cnt += local_valid_cnt;
        m_.unlock();
        best_mappings.at(tid_).first = local_min_edp;
        best_mappings.at(tid_).second.swap_degrees(local_best_mapping.get_degrees());
        similar_mappings.at(tid_).assign(local_similar_mappings.begin(), local_similar_mappings.end());
    }
    return;
}
