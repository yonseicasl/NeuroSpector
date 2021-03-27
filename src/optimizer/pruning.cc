#include "optimizer.h"

static handler_t handler;

/* Pruning (s-t or t-s) */
pruning_t::pruning_t(const std::string& acc_cfg_path_, 
                     const std::string& net_cfg_path_, 
                     const opt_type_t opt_type_, 
                     const unsigned num_threads_,
                     const bool is_fixed_)
    : optimizer_t(acc_cfg_path_, net_cfg_path_, is_fixed_),
      opt_type(opt_type_),
      num_threads(num_threads_),
      num_spatial(0),
      num_temporal(0) {
    if(exists.at(static_cast<unsigned>(component_t::S0)) || exists.at(static_cast<unsigned>(component_t::S2))) {
        handler.print_err(err_type_t::INVAILD, "s-t or t-s pruning optimizer does not support scaling");
        exit(1);
    }
    for(unsigned row = 1; row < U_size; row++) {
        if((row == static_cast<unsigned>(component_t::L1) || row == static_cast<unsigned>(component_t::L2)) && exists.at(row))
            num_temporal++;
        if((row == static_cast<unsigned>(component_t::S1_X) || row == static_cast<unsigned>(component_t::S1_Y)) && exists.at(row))
            num_spatial++;
    }
}

pruning_t::~pruning_t() {

}

// Optimizer APIs
void pruning_t::run() {
    for(unsigned idx = 0; idx < mapping_tables.size(); idx++) 
        run(idx + 1);
    return;
}

void pruning_t::run(const unsigned idx_) {
    handler.print_line(50, "*");
    if(opt_type == opt_type_t::S_T)
        std::cout << "# PRUNING (S-T) OPTIMIZATION" << std::endl;
    else 
        std::cout << "# PRUNING (T-S) OPTIMIZATION" << std::endl;
    std::cout << "# NETWORK    : " << network_name << std::endl;
    std::cout << "# LAYER      : " << mapping_tables.at(idx_ - 1).get_layer_name() << std::endl;
    handler.print_line(50, "*");
    // Reset variables & containers
    reset(idx_);
    // Run the engine
    engine(idx_);
    for(unsigned l1_df = 0; l1_df < l1_dataflows.size(); l1_df++) {
        for(unsigned l2_df = 0; l2_df < l2_dataflows.size(); l2_df++) {
            // Update
            update(static_cast<dataflow_t>(l1_dataflows.at(l1_df)), 
                   static_cast<dataflow_t>(l1_dataflows.at(l2_df)));
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


void pruning_t::print_stats() {
    std::string df_str("IWO");
    handler.print_line(50, "*");

    handler.print_line(50, "*");
    return;
}

void pruning_t::print_csv() {
    std::string df_str("IWO");
    handler.print_line(50, "*");

    handler.print_line(50, "*");
    return;
}

// Optimizer private functions
void pruning_t::reset(const unsigned idx_) {
    valid_cnt.clear();
    total_cnt.clear();
    valid_cnt.assign(2, 0);
    total_cnt.assign(2, 0);
    return;
}

void pruning_t::engine(const unsigned idx_) {
    // Threads initialization
    std::vector<std::thread> workers;
    std::mutex m;
    if(opt_type == opt_type_t::S_T) {
        mapping_space_t spatial_mapping_space(num_spatial + 1, mapping_tables.at(idx_ - 1).get_layer_values());
        total_cnt.at(0) = spatial_mapping_space.get_num_permutations();
        // Spatial mapping first
        for(unsigned tid = 0; tid < num_threads; tid++) {
            workers.push_back(std::thread(&pruning_t::spatial_worker, 
                                          this, 
                                          tid, 
                                          std::ref(mapping_tables.at(idx_ - 1)),
                                          std::ref(spatial_mapping_space),
                                          std::ref(m)));
        }
        // Thread join
        for(unsigned tid = 0; tid < num_threads; tid++) 
            workers.at(tid).join();
        workers.clear();
        std::cout << "# NUM OF ENTRIES: " << after_spatial.size() << std::endl;
        exit(1);
    }
    else {
        mapping_space_t temporal_mapping_space(num_temporal + 1, mapping_tables.at(idx_ - 1).get_layer_values());
        total_cnt.at(1) = temporal_mapping_space.get_num_permutations();
        // Temporal mapping first
        for(unsigned tid = 0; tid < num_threads; tid++) {
            workers.push_back(std::thread(&pruning_t::temporal_worker, 
                                          this, 
                                          tid, 
                                          std::ref(mapping_tables.at(idx_ - 1)),
                                          std::ref(temporal_mapping_space),
                                          std::ref(m)));
        }
        // Thread join
        for(unsigned tid = 0; tid < num_threads; tid++) 
            workers.at(tid).join();
        workers.clear();
        std::cout << "# NUM OF ENTRIES: " << after_temporal.size() << std::endl;
        exit(1);
    }
    return;
}

void pruning_t::update(const dataflow_t l1_dataflow_,
                       const dataflow_t l2_dataflow_) {
//    unsigned local_best_idx = 0;
//    double local_min_energy = DBL_MAX;
//    for(size_t idx = 0; idx < best_mappings.size(); idx++) {
//        stats_t curr_stats(accelerator, best_mappings.at(idx), l1_dataflow_, l2_dataflow_);
//        curr_stats.update_stats();
//        if(local_min_energy > curr_stats.get_total_energy()) {
//            local_min_energy = curr_stats.get_total_energy();
//            local_best_idx = idx;
//        }
//    }
//    if(final_best_energy > local_min_energy) {
//        final_best_idx = local_best_idx;
//        final_best_energy = local_min_energy;
//        final_best_mappings.clear();
//        final_best_mappings.assign(best_mappings.begin(), best_mappings.end());
//        final_best_dataflows.clear();
//        final_best_dataflows.push_back(l1_dataflow_);
//        final_best_dataflows.push_back(l2_dataflow_);
//    }
    return;
}

void pruning_t::spatial_worker(const unsigned tid_,
                               const mapping_table_t& init_mapping_,
                               const mapping_space_t& mapping_space_, 
                               std::mutex& m_) {
    uint64_t local_valid_cnt = 0;
    std::vector<mapping_table_t> local_valid_mappings;
    // Current mapping table
    mapping_table_t curr_mapping(init_mapping_);
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Start finding valid mappings
    for(size_t k = range.start_k; k < range.end_k; k++) {
        curr_mapping.put_column_spatial_degrees(parameter_t::K, mapping_space_.get_permutations(0).at(k));
        for(size_t b = range.start_b; b < range.end_b; b++) {
            curr_mapping.put_column_spatial_degrees(parameter_t::B, mapping_space_.get_permutations(1).at(b));
            for(size_t p = range.start_p; p < range.end_p; p++) {
                curr_mapping.put_column_spatial_degrees(parameter_t::P, mapping_space_.get_permutations(2).at(p));
                for(size_t q = range.start_q; q < range.end_q; q++) {
                    curr_mapping.put_column_spatial_degrees(parameter_t::Q, mapping_space_.get_permutations(3).at(q));
                    for(size_t c = range.start_c; c < range.end_c; c++) {
                        curr_mapping.put_column_spatial_degrees(parameter_t::C, mapping_space_.get_permutations(4).at(c));
                        for(size_t s = range.start_s; s < range.end_s; s++) {
                            curr_mapping.put_column_spatial_degrees(parameter_t::S, mapping_space_.get_permutations(5).at(s));
                            for(size_t r = range.start_r; r < range.end_r; r++) {
                                curr_mapping.put_column_spatial_degrees(parameter_t::R, mapping_space_.get_permutations(6).at(r));
                                // Validity check
                                if(check_all_validity(curr_mapping)) {
                                    local_valid_cnt++;
                                    local_valid_mappings.push_back(curr_mapping);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(local_valid_cnt > 0) {
        m_.lock();
        valid_cnt.at(0) += local_valid_cnt;
        for(size_t i = 0; i < local_valid_mappings.size(); i++) {
            after_spatial.push_back(local_valid_mappings.at(i));
        }
        m_.unlock();
    }
    return;
}

void pruning_t::temporal_worker(const unsigned tid_,
                                const mapping_table_t& init_mapping_,
                                const mapping_space_t& mapping_space_, 
                                std::mutex& m_) {
    uint64_t local_valid_cnt = 0;
    std::vector<mapping_table_t> local_valid_mappings;
    // Current mapping table
    mapping_table_t curr_mapping(init_mapping_);
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Start finding valid mappings
    for(size_t k = range.start_k; k < range.end_k; k++) {
        curr_mapping.put_column_temporal_degrees(parameter_t::K, mapping_space_.get_permutations(0).at(k));
        for(size_t b = range.start_b; b < range.end_b; b++) {
            curr_mapping.put_column_temporal_degrees(parameter_t::B, mapping_space_.get_permutations(1).at(b));
            for(size_t p = range.start_p; p < range.end_p; p++) {
                curr_mapping.put_column_temporal_degrees(parameter_t::P, mapping_space_.get_permutations(2).at(p));
                for(size_t q = range.start_q; q < range.end_q; q++) {
                    curr_mapping.put_column_temporal_degrees(parameter_t::Q, mapping_space_.get_permutations(3).at(q));
                    for(size_t c = range.start_c; c < range.end_c; c++) {
                        curr_mapping.put_column_temporal_degrees(parameter_t::C, mapping_space_.get_permutations(4).at(c));
                        for(size_t s = range.start_s; s < range.end_s; s++) {
                            curr_mapping.put_column_temporal_degrees(parameter_t::S, mapping_space_.get_permutations(5).at(s));
                            for(size_t r = range.start_r; r < range.end_r; r++) {
                                curr_mapping.put_column_temporal_degrees(parameter_t::R, mapping_space_.get_permutations(6).at(r));
                                // Validity check
                                if(check_all_validity(curr_mapping)) {
                                    local_valid_cnt++;
                                    local_valid_mappings.push_back(curr_mapping);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(local_valid_cnt > 0) {
        m_.lock();
        valid_cnt.at(1) += local_valid_cnt;
        for(size_t i = 0; i < local_valid_mappings.size(); i++) {
            after_temporal.push_back(local_valid_mappings.at(i));
        }
        m_.unlock();
    }
    return;
}
