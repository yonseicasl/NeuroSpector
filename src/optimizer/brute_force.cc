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
      num_permutations(0), 
      num_threads(num_threads_),
      global_valid_cnt(0), 
      final_min_stat(DBL_MAX) {

}             

brute_force_t::~brute_force_t() {
    
}

// Optimizer APIs
void brute_force_t::run() {
    // TODO
    return;
}

void brute_force_t::run(const unsigned idx_) {
    accelerator->print_stats();
    std::cout << "# NETWORK    : " << network_name << std::endl;
    std::cout << "# NUM THREADS: " << num_threads << std::endl;
    handler.print_line(50, "*");
    // Mapping space generation
    mapping_space_t mapping_space(num_levels - 1, mapping_tables.at(idx_ - 1).get_layer_values());
    num_permutations = mapping_space.get_num_permutations();
    // Fixed datatflows
    if(is_fixed) {
        // Global reset
        global_reset(idx_);
        // Threads initialization
        std::vector<std::thread> workers;
        std::mutex m;
        // Start optimizing
        switch(opt_type) {
            case opt_type_t::B_F_ENERGY: 
                std::cout << "# ENERGY-DELAY OPTIMIZATION" << std::endl; 
                // Multi-threading
                for(unsigned tid = 0; tid < num_threads; tid++) {
                    workers.push_back(std::thread(&brute_force_t::energy_worker, this, 
                                                  tid, 
                                                  std::ref(mapping_tables.at(idx_ - 1)),
                                                  std::ref(mapping_space), 
                                                  accelerator->l1_dataflow(),
                                                  accelerator->l2_dataflow(),
                                                  std::ref(m)));
                }
                // Thread join
                for(unsigned tid = 0; tid < num_threads; tid++) 
                    workers.at(tid).join();
                break;
            case opt_type_t::B_F_CYCLE: 
                std::cout << "# DELAY-ENERGY OPTIMIZATION" << std::endl; 
                // Multi-threading
                for(unsigned tid = 0; tid < num_threads; tid++) {
                    workers.push_back(std::thread(&brute_force_t::cycle_worker, this, 
                                                  tid, 
                                                  std::ref(mapping_tables.at(idx_ - 1)),
                                                  std::ref(mapping_space), 
                                                  accelerator->l1_dataflow(),
                                                  accelerator->l2_dataflow(),
                                                  std::ref(m)));
                }
                // Thread join
                for(unsigned tid = 0; tid < num_threads; tid++) 
                    workers.at(tid).join();
                break;
            case opt_type_t::B_F_EDP: 
                std::cout << "# EDP OPTIMIZATION" << std::endl; 
                // Multi-threading
                for(unsigned tid = 0; tid < num_threads; tid++) {
                    workers.push_back(std::thread(&brute_force_t::edp_worker, this, 
                                                  tid, 
                                                  std::ref(mapping_tables.at(idx_ - 1)),
                                                  std::ref(mapping_space), 
                                                  accelerator->l1_dataflow(),
                                                  accelerator->l2_dataflow(),
                                                  std::ref(m)));
                }
                // Thread join
                for(unsigned tid = 0; tid < num_threads; tid++) 
                    workers.at(tid).join();
                break;
            default: break;
        }
        // Sync
        final_min_stat = DBL_MAX;
        double second_stat = DBL_MAX;
        for(unsigned tid = 0; tid < num_threads; tid++) {
            if(final_min_stat > global_min_stats.at(tid)) {
                final_min_stat = global_min_stats.at(tid);
                final_best_mappings.clear();
                final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                    final_best_mappings.push_back(*it);
                if(opt_type == opt_type_t::B_F_ENERGY) {
                    // Cycle
                    // TODO: datatflows
                    stats_t stats(accelerator, final_best_mappings.at(0));
                    stats.update_stats();
                    second_stat = stats.get_total_cycle();
                }
                else if(opt_type == opt_type_t::B_F_CYCLE) {
                    // Energy
                    // TODO: datatflows
                    stats_t stats(accelerator, final_best_mappings.at(0));
                    stats.update_stats();
                    second_stat = stats.get_total_energy();
                }
                else {
                    // B_F_EDP: Nothing to do
                } 
            }
            else if(final_min_stat == global_min_stats.at(tid)) {
                if(opt_type == opt_type_t::B_F_ENERGY) {
                    // TODO: datatflows
                    stats_t stats(accelerator, global_best_mapping_tables.at(tid));
                    stats.update_stats();
                    if(second_stat > stats.get_total_cycle()) {
                        final_best_mappings.clear();
                        final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                        for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                            final_best_mappings.push_back(*it);
                    }
                    else if(second_stat == stats.get_total_cycle()) {
                        final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                        for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                            final_best_mappings.push_back(*it);
                    }
                    else {
                        // Nothing to do
                    } 
                }
                else if(opt_type == opt_type_t::B_F_CYCLE) {
                    // TODO: datatflows
                    stats_t stats(accelerator, global_best_mapping_tables.at(tid));
                    stats.update_stats();
                    if(second_stat > stats.get_total_energy()) {
                        final_best_mappings.clear();
                        final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                        for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                            final_best_mappings.push_back(*it);
                    }
                    else if(second_stat == stats.get_total_energy()) {
                        final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                        for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                            final_best_mappings.push_back(*it);
                    }
                    else {
                        // Nothing to do
                    } 
                }
                else {
                    // B_F_EDP
                    final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                    for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                        final_best_mappings.push_back(*it);
                } 
            }
            else {
                // Nothing to do
            }
        }
        // Print stats
        print_stats();
        for(size_t i = 0; i < final_best_mappings.size(); i++) {
            std::cout << "\n# BEST MAPPING TABLE " << i + 1 << std::endl;
#ifdef CSV
            final_best_mappings.at(i).print_csv();
            mapping_table_t for_stats(final_best_mappings.at(i));
            // TODO: datatflows
            stats_t stats(accelerator, for_stats);
            stats.update_stats();
            stats.print_csv();
#else
            final_best_mappings.at(i).print_stats();
            mapping_table_t for_stats(final_best_mappings.at(i));
            // TODO: datatflows
            stats_t stats(accelerator, for_stats);
            stats.update_stats();
            stats.print_stats();
#endif
        }
    }
    // Flexible datatflows
    else {
        // Start optimizing
        std::string df_str("IWO");
        for(unsigned l1_df = 0; l1_df < l1_dataflow.size(); l1_df++) {
            for(unsigned l2_df = 0; l2_df < l2_dataflow.size(); l2_df++) {
                // Global reset
                global_reset(idx_);
                // Threads initialization
                std::vector<std::thread> workers;
                std::mutex m;
                // Start optimizing
                switch(opt_type) {
                    case opt_type_t::B_F_ENERGY: 
                        if(l1_df == 0 && l2_df == 0)
                            std::cout << "# ENERGY-DELAY OPTIMIZATION" << std::endl; 
                        // Multi-threading
                        for(unsigned tid = 0; tid < num_threads; tid++) {
                            workers.push_back(std::thread(&brute_force_t::energy_worker, this, 
                                                          tid, 
                                                          std::ref(mapping_tables.at(idx_ - 1)),
                                                          std::ref(mapping_space), 
                                                          static_cast<dataflow_t>(l1_df),
                                                          static_cast<dataflow_t>(l2_df),
                                                          std::ref(m)));
                        }
                        // Thread join
                        for(unsigned tid = 0; tid < num_threads; tid++) 
                            workers.at(tid).join();
                        break;
                    case opt_type_t::B_F_CYCLE: 
                        if(l1_df == 0 && l2_df == 0)
                            std::cout << "# DELAY-ENERGY OPTIMIZATION" << std::endl; 
                        // Multi-threading
                        for(unsigned tid = 0; tid < num_threads; tid++) {
                            workers.push_back(std::thread(&brute_force_t::cycle_worker, this, 
                                                          tid, 
                                                          std::ref(mapping_tables.at(idx_ - 1)),
                                                          std::ref(mapping_space), 
                                                          static_cast<dataflow_t>(l1_df),
                                                          static_cast<dataflow_t>(l2_df),
                                                          std::ref(m)));
                        }
                        // Thread join
                        for(unsigned tid = 0; tid < num_threads; tid++) 
                            workers.at(tid).join();
                        break;
                    case opt_type_t::B_F_EDP: 
                        if(l1_df == 0 && l2_df == 0)
                            std::cout << "# EDP OPTIMIZATION" << std::endl; 
                        // Multi-threading
                        for(unsigned tid = 0; tid < num_threads; tid++) {
                            workers.push_back(std::thread(&brute_force_t::edp_worker, this, 
                                                          tid, 
                                                          std::ref(mapping_tables.at(idx_ - 1)),
                                                          std::ref(mapping_space), 
                                                          static_cast<dataflow_t>(l1_df),
                                                          static_cast<dataflow_t>(l2_df),
                                                          std::ref(m)));
                        }
                        // Thread join
                        for(unsigned tid = 0; tid < num_threads; tid++) 
                            workers.at(tid).join();
                        break;
                    default: break;
                }
                // Sync
                workers.clear();
                final_min_stat = DBL_MAX;
                double second_stat = DBL_MAX;
                for(unsigned tid = 0; tid < num_threads; tid++) {
                    if(final_min_stat > global_min_stats.at(tid)) {
                        final_min_stat = global_min_stats.at(tid);
                        final_best_mappings.clear();
                        final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                        for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                            final_best_mappings.push_back(*it);
                        if(opt_type == opt_type_t::B_F_ENERGY) {
                            // Cycle
                            stats_t stats(accelerator, final_best_mappings.at(0), static_cast<dataflow_t>(l1_df), static_cast<dataflow_t>(l2_df));
                            stats.update_stats();
                            second_stat = stats.get_total_cycle();
                        }
                        else if(opt_type == opt_type_t::B_F_CYCLE) {
                            // Energy
                            stats_t stats(accelerator, final_best_mappings.at(0), static_cast<dataflow_t>(l1_df), static_cast<dataflow_t>(l2_df));
                            stats.update_stats();
                            second_stat = stats.get_total_energy();
                        }
                        else {
                            // B_F_EDP: Nothing to do
                        } 
                    }
                    else if(final_min_stat == global_min_stats.at(tid)) {
                        if(opt_type == opt_type_t::B_F_ENERGY) {
                            stats_t stats(accelerator, global_best_mapping_tables.at(tid), static_cast<dataflow_t>(l1_df), static_cast<dataflow_t>(l2_df));
                            stats.update_stats();
                            if(second_stat > stats.get_total_cycle()) {
                                final_best_mappings.clear();
                                final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                                for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                                    final_best_mappings.push_back(*it);
                            }
                            else if(second_stat == stats.get_total_cycle()) {
                                final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                                for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                                    final_best_mappings.push_back(*it);
                            }
                            else {
                                // Nothing to do
                            } 
                        }
                        else if(opt_type == opt_type_t::B_F_CYCLE) {
                            stats_t stats(accelerator, global_best_mapping_tables.at(tid), static_cast<dataflow_t>(l1_df), static_cast<dataflow_t>(l2_df));
                            stats.update_stats();
                            if(second_stat > stats.get_total_energy()) {
                                final_best_mappings.clear();
                                final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                                for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                                    final_best_mappings.push_back(*it);
                            }
                            else if(second_stat == stats.get_total_energy()) {
                                final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                                for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                                    final_best_mappings.push_back(*it);
                            }
                            else {
                                // Nothing to do
                            } 
                        }
                        else {
                            // B_F_EDP
                            final_best_mappings.push_back(global_best_mapping_tables.at(tid));
                            for(auto it = global_similar_mapping_tables.at(tid).begin(); it != global_similar_mapping_tables.at(tid).end(); ++it) 
                                final_best_mappings.push_back(*it);
                        } 
                    }
                    else {
                        // Nothing to do
                    }
                }
                // Print stats
#ifdef SIMPLE
                if(l1_df == 0 && l2_df == 0) {
                    print_stats();
                    std::cout << "# " << mapping_tables.at(idx_ - 1).get_layer_name() << std::endl;
                }
                handler.print_line(50, "*");
                std::cout << "# DATAFLOWS: " << df_str.at(l1_df) << "S-" << df_str.at(l2_df) << "S" << std::endl; 
                mapping_table_t for_stats(final_best_mappings.at(0));
                stats_t stats(accelerator, for_stats, static_cast<dataflow_t>(l1_df), static_cast<dataflow_t>(l2_df));
                stats.update_stats();
                stats.print_csv();
#else
                print_stats();
                handler.print_line(50, "*");
                std::cout << "# DATAFLOWS: " << df_str.at(l1_df) << "S-" << df_str.at(l2_df) << "S" << std::endl; 
                for(size_t i = 0; i < final_best_mappings.size(); i++) {
                    std::cout << "\n# BEST MAPPING TABLE " << i + 1 << std::endl;
#ifdef CSV
                    final_best_mappings.at(i).print_csv();
                    mapping_table_t for_stats(final_best_mappings.at(i));
                    stats_t stats(accelerator, for_stats, static_cast<dataflow_t>(l1_df), static_cast<dataflow_t>(l2_df));
                    stats.update_stats();
                    stats.print_csv();
#else
                    final_best_mappings.at(i).print_stats();
                    mapping_table_t for_stats(final_best_mappings.at(i));
                    stats_t stats(accelerator, for_stats, static_cast<dataflow_t>(l1_df), static_cast<dataflow_t>(l2_df));
                    stats.update_stats();
                    stats.print_stats();
#endif
                }
#endif
            }
        }
    }
}

void brute_force_t::global_reset(const unsigned idx_) {
    // Global clear
    global_min_stats.clear();
    global_best_mapping_tables.clear();
    global_similar_mapping_tables.clear();
    // Global initialization 
    global_valid_cnt = 0;
    global_min_stats.assign(num_threads, DBL_MAX);
    global_best_mapping_tables.assign(num_threads, mapping_table_t(mapping_tables.at(idx_ - 1)));
    std::vector<mapping_table_t> tmp(1, mapping_table_t(mapping_tables.at(idx_ - 1)));
    global_similar_mapping_tables.assign(num_threads, tmp);
    return;
}

void brute_force_t::print_stats() {
#ifdef SIMPLE
    std::cout << "VALID MAPPINGS,TOTAL MAPPINGS\n"  
              << global_valid_cnt << "," 
              << num_permutations << std::endl;
    handler.print_line(50, "*");
#else
#ifdef CSV
    std::cout << "VALID MAPPINGS,TOTAL MAPPINGS,SIMILAR MAPPINGS\n"  
              << global_valid_cnt << "," 
              << num_permutations << "," 
              << final_best_mappings.size() << std::endl;
    handler.print_line(50, "*");
#else
    std::cout << "# NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << global_valid_cnt 
              << " (" << std::fixed << std::setprecision(2) 
              << float(global_valid_cnt) / num_permutations * 100 << "%)" << std::endl; 
    std::cout << "# TOTAL NUM OF MAPPINGS  : " 
              << std::setw(15) << num_permutations 
              << " (100%)" << std::endl;
    std::cout << "# NUM OF SIMILAR MAPPINGS : " 
              << std::setw(15) << final_best_mappings.size() << std::endl;
    handler.print_line(50, "*");
#endif
#endif
}

// Mapping workers
void brute_force_t::energy_worker(const unsigned tid_, 
                                  const mapping_table_t& init_mapping_,
                                  const mapping_space_t& mapping_space_,
                                  const dataflow_t l1_dataflow_,
                                  const dataflow_t l2_dataflow_,
                                  std::mutex& m_) {
    // Initialze variables & containers
    double min_energy = DBL_MAX;
    double min_cycle = DBL_MAX;
    uint64_t valid_cnt = 0;
    unsigned similar_cnt = 0;
    mapping_table_t curr_mapping_table(init_mapping_);
    mapping_table_t local_best_mapping_table(init_mapping_); 
    std::vector<mapping_table_t> similar_mapping_tables;
    // Thread index adjustment
    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations());
    // Start finding a local optimized mapping
    for(size_t k = range.start_k; k < range.end_k; k++) {
        curr_mapping_table.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(0).at(k), component_t::MAC, component_t::DRAM);
        for(size_t b = range.start_b; b < range.end_b; b++) {
            curr_mapping_table.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(1).at(b), component_t::MAC, component_t::DRAM);
            for(size_t p = range.start_p; p < range.end_p; p++) {
                curr_mapping_table.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(2).at(p), component_t::MAC, component_t::DRAM);
                for(size_t q = range.start_q; q < range.end_q; q++) {
                    curr_mapping_table.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(3).at(q), component_t::MAC, component_t::DRAM);
                    for(size_t c = range.start_c; c < range.end_c; c++) {
                        curr_mapping_table.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(4).at(c), component_t::MAC, component_t::DRAM);
                        for(size_t s = range.start_s; s < range.end_s; s++) {
                            curr_mapping_table.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(5).at(s), component_t::MAC, component_t::DRAM);
                            for(size_t r = range.start_r; r < range.end_r; r++) {
                                curr_mapping_table.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(6).at(r), component_t::MAC, component_t::DRAM);
                                // Find the best mapping table
                                stats_t curr_stats(accelerator, curr_mapping_table, l1_dataflow_, l2_dataflow_);
                                if(check_all_validity(curr_mapping_table)) {
                                    curr_stats.update_stats();
                                    if(min_energy > curr_stats.get_total_energy()) {
                                        min_energy = curr_stats.get_total_energy();
                                        min_cycle = curr_stats.get_total_cycle();
                                        local_best_mapping_table.swap_degrees(curr_mapping_table.get_degrees());
                                        similar_cnt = 0;
                                    }
                                    else if(min_energy == curr_stats.get_total_energy()) {
                                        if(similar_cnt == 0)
                                            similar_mapping_tables.clear();
                                        if(min_cycle > curr_stats.get_total_cycle()) {
                                            min_cycle = curr_stats.get_total_cycle();
                                            local_best_mapping_table.swap_degrees(curr_mapping_table.get_degrees());
                                            similar_cnt = 0;
                                        }
                                        else if(min_cycle == curr_stats.get_total_cycle()) {
                                            mapping_table_t similar_mapping_table(curr_mapping_table);
                                            similar_mapping_tables.push_back(similar_mapping_table);
                                            similar_cnt++;
                                        }
                                        else {
                                            // Nothing to do
                                        }
                                    }
                                    else {
                                        // Nothing to do
                                    }
                                    valid_cnt++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Sync
    m_.lock();
    global_valid_cnt += valid_cnt;
    global_min_stats.at(tid_) = min_energy;
    global_best_mapping_tables.at(tid_).swap_degrees(local_best_mapping_table.get_degrees());
    global_similar_mapping_tables.at(tid_).assign(similar_mapping_tables.begin(), similar_mapping_tables.end());
    m_.unlock();
    return;
}

void brute_force_t::cycle_worker(const unsigned tid_, 
                                 const mapping_table_t& init_mapping_,
                                 const mapping_space_t& mapping_space_,
                                 const dataflow_t l1_dataflow_,
                                 const dataflow_t l2_dataflow_,
                                 std::mutex& m_) {
//    mapping_table_t curr_mapping_table(mapping_tables.at(idx_ - 1));
//    mapping_table_t local_best_mapping_table(mapping_tables.at(idx_ - 1)); 
//    double min_energy = DBL_MAX;
//    size_t min_cycle = -1;
//    size_t match_cnt = 0;
//    uint64_t total_cnt = 0;
//    uint64_t valid_cnt = 0;
//    std::vector<mapping_table_t> similar_mapping_tables;
//    // Thread index adjustment
//    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations(), std::ref(m_));
//    for(size_t k = range.start_k; k < range.end_k; k++) {
//        curr_mapping_table.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(0).at(k), start_, end_);
//        for(size_t b = range.start_b; b < range.end_b; b++) {
//            curr_mapping_table.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(1).at(b), start_, end_);
//            for(size_t p = range.start_p; p < range.end_p; p++) {
//                curr_mapping_table.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(2).at(p), start_, end_);
//                for(size_t q = range.start_q; q < range.end_q; q++) {
//                    curr_mapping_table.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(3).at(q), start_, end_);
//                    for(size_t c = range.start_c; c < range.end_c; c++) {
//                        curr_mapping_table.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(4).at(c), start_, end_);
//                        for(size_t s = range.start_s; s < range.end_s; s++) {
//                            curr_mapping_table.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(5).at(s), start_, end_);
//                            for(size_t r = range.start_r; r < range.end_r; r++) {
//                                curr_mapping_table.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(6).at(r), start_, end_);
//                                // Find the best mapping table
//                                stats_t curr_stats(accelerator, curr_mapping_table);
//                                if(check_all_validity(curr_mapping_table)) {
//                                    valid_cnt++;
//                                    curr_stats.update_stats();
//                                    if(min_cycle > curr_stats.get_total_cycle()) {
//                                        min_energy = curr_stats.get_total_energy();
//                                        min_cycle = curr_stats.get_total_cycle();
//                                        local_best_mapping_table->swap_degrees(curr_mapping_table->get_degrees());
//                                        match_cnt = 0;
//                                    }
//                                    else if(min_cycle == curr_stats.get_total_cycle()) {
//                                        if(match_cnt == 0)
//                                            similar_mapping_tables.clear();
//                                        if(min_energy > curr_stats.get_total_energy()) {
//                                            min_energy = curr_stats.get_total_energy();
//                                            local_best_mapping_table->swap_degrees(curr_mapping_table->get_degrees());
//                                            match_cnt = 0;
//                                        }
//                                        else if(min_energy == curr_stats.get_total_energy()) {
//                                            mapping_table_t similar_mapping_table(mapping_tables.at(idx_ - 1));
//                                            similar_mapping_table.swap_degrees(curr_mapping_table->get_degrees());
//                                            similar_mapping_tables.push_back(similar_mapping_table);
//                                            match_cnt++;
//                                        }
//                                        else {
//                                            // Nothing to do
//                                        }
//                                    }
//                                    else {
//                                        // Nothing to do
//                                    }
//                                }
//                                total_cnt++;
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }
//    // Sync
//    m_.lock();
//    global_total_cnt += total_cnt;
//    global_valid_cnt += valid_cnt;
//    global_min_energy.at(tid_) = min_energy;
//    global_best_mapping_table.at(tid_).swap_degrees(local_best_mapping_table.get_degrees());
//    global_similar_mapping_tables.at(tid_).assign(similar_mapping_tables.begin(), similar_mapping_tables.end());
//    m_.unlock();
    return;
}

void brute_force_t::edp_worker(const unsigned tid_, 
                               const mapping_table_t& init_mapping_,
                               const mapping_space_t& mapping_space_,
                               const dataflow_t l1_dataflow_,
                               const dataflow_t l2_dataflow_,
                               std::mutex& m_) {
//    mapping_table_t curr_mapping_table(mapping_tables.at(idx_ - 1));
//    mapping_table_t local_best_mapping_table(mapping_tables.at(idx_ - 1)); 
//    double min_edp = DBL_MAX;
//    size_t match_cnt = 0;
//    uint64_t total_cnt = 0;
//    uint64_t valid_cnt = 0;
//    std::vector<mapping_table_t> similar_mapping_tables;
//    // Thread index adjustment
//    range_t range(tid_, num_threads, mapping_space_.get_layer_permutations(), std::ref(m_));
//    for(size_t k = range.start_k; k < range.end_k; k++) {
//        curr_mapping_table.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(0).at(k), start_, end_);
//        for(size_t b = range.start_b; b < range.end_b; b++) {
//            curr_mapping_table.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(1).at(b), start_, end_);
//            for(size_t p = range.start_p; p < range.end_p; p++) {
//                curr_mapping_table.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(2).at(p), start_, end_);
//                for(size_t q = range.start_q; q < range.end_q; q++) {
//                    curr_mapping_table.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(3).at(q), start_, end_);
//                    for(size_t c = range.start_c; c < range.end_c; c++) {
//                        curr_mapping_table.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(4).at(c), start_, end_);
//                        for(size_t s = range.start_s; s < range.end_s; s++) {
//                            curr_mapping_table.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(5).at(s), start_, end_);
//                            for(size_t r = range.start_r; r < range.end_r; r++) {
//                                curr_mapping_table.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(6).at(r), start_, end_);
//                                // Find the best mapping table
//                                stats_t curr_stats(accelerator, curr_mapping_table);
//                                if(check_all_validity(curr_mapping_table)) {
//                                    valid_cnt++;
//                                    curr_stats.update_stats();
//                                    if(min_edp > curr_stats.get_total_edp()) {
//                                        min_energy = curr_stats.get_total_energy();
//                                        min_cycle = curr_stats.get_total_cycle();
//                                        min_edp = curr_stats.get_total_edp();
//                                        local_best_mapping_table->swap_degrees(curr_mapping_table->get_degrees());
//                                        match_cnt = 0;
//                                    }
//                                    else if(min_edp == curr_stats.get_total_edp()) {
//                                        if(match_cnt == 0)
//                                            similar_mapping_tables.clear();
//                                        mapping_table_t similar_mapping_table(mapping_tables.at(idx_ - 1));
//                                        similar_mapping_table.swap_degrees(curr_mapping_table->get_degrees());
//                                        similar_mapping_tables.push_back(similar_mapping_table);
//                                        match_cnt++;
//                                    }
//                                    else {
//                                        // Nothing to do
//                                    }
//                                }
//                                total_cnt++;
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }
//    // Sync
//    m_.lock();
//    global_total_cnt += total_cnt;
//    global_valid_cnt += valid_cnt;
//    global_min_energy.at(tid_) = min_energy;
//    global_best_mapping_table.at(tid_).swap_degrees(local_best_mapping_table.get_degrees());
//    global_similar_mapping_tables.at(tid_).assign(similar_mapping_tables.begin(), similar_mapping_tables.end());
//    m_.unlock();
    return;
}
