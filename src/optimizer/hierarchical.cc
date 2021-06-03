#include "optimizer.h"

#define SEQ_MAX 3
#define TOP_K_FIRST 1
#define TOP_K_SECOND 1
#define TOP_K_THIRD 1

static handler_t handler;

/* Hierarchical */
hierarchical_t::hierarchical_t(const std::string& acc_cfg_path_, 
                               const std::string& net_cfg_path_, 
                               const bool is_fixed_)
    : optimizer_t(acc_cfg_path_, net_cfg_path_, is_fixed_),
      final_best_idx(0),
      final_best_energy(DBL_MAX) {
    // Initialze used_levels
    used_levels.assign(SEQ_MAX, 0);
    for(unsigned row = 1; row < U_size; row++) {
        // SEQ 2: MAC-L1
        if(row == static_cast<unsigned>(component_t::S0) && exists.at(row))
            used_levels.at(2)++;
        // SEQ 1: L1-L2
        if((row == static_cast<unsigned>(component_t::L1) || row == static_cast<unsigned>(component_t::S1_X) || row == static_cast<unsigned>(component_t::S1_Y)) && exists.at(row))
            used_levels.at(1)++;
        // SEQ 0: L2-DRAM
        if((row == static_cast<unsigned>(component_t::L2) || row == static_cast<unsigned>(component_t::S2)) && exists.at(row))
            used_levels.at(0)++;
    }
    // Initialze top_k
    // SEQ 0: L2-DRAM
    if(used_levels.at(0) > 0)
        top_k.push_back(TOP_K_FIRST);
    else 
        top_k.push_back(1);
    // SEQ 1: L1-L2
    if(used_levels.at(1) > 0)
        top_k.push_back(TOP_K_SECOND);
    else 
        top_k.push_back(1);
    // SEQ 2: MAC-L1
    if(used_levels.at(2) > 0)
        top_k.push_back(TOP_K_THIRD);
    else 
        top_k.push_back(1);
}     

hierarchical_t::~hierarchical_t() {

}

// Optimizer APIs
void hierarchical_t::run() {
    for(unsigned idx = 0; idx < mappings.size(); idx++) 
        run(idx + 1);
    return;
}

void hierarchical_t::run(const unsigned idx_) {
    handler.print_line(50, "*");
    std::cout << "# HIERARCHICAL OPTIMIZATION" << std::endl;
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
            // Update
            update(static_cast<dataflow_t>(l1_dataflows.at(l1_df)), 
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


void hierarchical_t::print_stats() {
    std::string df_str("IWO");
    handler.print_line(50, "*");
    std::cout << "# TOP K : " << top_k.at(0) << "-" << top_k.at(1) << "-" << top_k.at(2) << std::endl;
    std::cout << "# WINNER: " << seq_0_top_k << "-" << seq_1_top_k << "-" << seq_2_top_k << std::endl;
    handler.print_line(50, "*");
    std::cout << "# SEQ 0 (L2-S2)"<< "\n"
              << " - NUM OF TOTAL MAPPINGS  : " 
              << std::setw(15) << total_cnt.at(0) << "\n"
              << " - NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << valid_cnt.at(0) << std::endl;
    std::cout << "# SEQ 1 (L1-S1)" << std::endl;
    for(size_t i = 0; i < top_k.at(0); i++) {
        std::cout << " - NUM OF TOTAL MAPPINGS  : " 
                  << std::setw(15) << total_cnt.at(1 + i) << "\n"
                  << " - NUM OF VALID MAPPINGS  : " 
                  << std::setw(15) << valid_cnt.at(1 + i) << std::endl;
    }
    // TODO
    if(used_levels.at(2) != 0) {
        std::cout << "# SEQ 2 (MAC-S0)" << std::endl;
        for(size_t i = 0; i < top_k.at(0) * top_k.at(1); i++) {
            std::cout << " - NUM OF TOTAL MAPPINGS  : " 
                      << std::setw(15) << total_cnt.at(1 + top_k.at(0) + i) << "\n"
                      << " - NUM OF VALID MAPPINGS  : " 
                      << std::setw(15) << valid_cnt.at(1 + top_k.at(0) + i) << std::endl;
        }
    }
    handler.print_line(50, "*");
    std::cout << "# BEST MAPPING TABLE "<< std::endl;
    std::cout << "# DATAFLOWS (L1-L2): " 
              << df_str.at(static_cast<unsigned>(final_best_dataflows.at(0))) 
              << "S-" 
              << df_str.at(static_cast<unsigned>(final_best_dataflows.at(1))) 
              << "S" << std::endl; 
    final_best_mappings.at(final_best_idx).print_stats();
    stats_t stats_tmp(accelerator, final_best_mappings.at(final_best_idx), final_best_dataflows.at(0), final_best_dataflows.at(1));
    stats_tmp.update_stats();
    stats_tmp.print_stats();
    handler.print_line(50, "*");
    return;
}

void hierarchical_t::print_csv() {
    std::string df_str("IWO");
    handler.print_line(50, "*");
    std::cout << "# TOP K : " << top_k.at(0) << "-" << top_k.at(1) << "-" << top_k.at(2) << std::endl;
    std::cout << "# WINNER: " << seq_0_top_k << "-" << seq_1_top_k << "-" << seq_2_top_k << std::endl;
    handler.print_line(50, "*");
    std::cout << "# SEQ 0 (L2-S2)"<< "\n"
              << " - NUM OF TOTAL MAPPINGS  : " 
              << std::setw(15) << total_cnt.at(0) << "\n"
              << " - NUM OF VALID MAPPINGS  : " 
              << std::setw(15) << valid_cnt.at(0) << std::endl;
    std::cout << "# SEQ 1 (L1-S1)" << std::endl;
    for(size_t i = 0; i < top_k.at(0); i++) {
        std::cout << " - NUM OF TOTAL MAPPINGS  : " 
                  << std::setw(15) << total_cnt.at(1 + i) << "\n"
                  << " - NUM OF VALID MAPPINGS  : " 
                  << std::setw(15) << valid_cnt.at(1 + i) << std::endl;
    }
    // TODO
    if(used_levels.at(2) != 0) {
        std::cout << "# SEQ 2 (MAC-S0)" << std::endl;
        for(size_t i = 0; i < top_k.at(0) * top_k.at(1); i++) {
            std::cout << " - NUM OF TOTAL MAPPINGS  : " 
                      << std::setw(15) << total_cnt.at(1 + top_k.at(0) + i) << "\n"
                      << " - NUM OF VALID MAPPINGS  : " 
                      << std::setw(15) << valid_cnt.at(1 + top_k.at(0) + i) << std::endl;
        }
    }
    handler.print_line(50, "*");
    std::cout << "# BEST MAPPING TABLE "<< std::endl;
    std::cout << "# DATAFLOWS (L1-L2): " 
              << df_str.at(static_cast<unsigned>(final_best_dataflows.at(0))) 
              << "S-" 
              << df_str.at(static_cast<unsigned>(final_best_dataflows.at(1))) 
              << "S" << std::endl; 
    final_best_mappings.at(final_best_idx).print_csv();
    stats_t stats_tmp(accelerator, final_best_mappings.at(final_best_idx), final_best_dataflows.at(0), final_best_dataflows.at(1));
    stats_tmp.update_stats();
    stats_tmp.print_csv();
    handler.print_line(50, "*");
    return;
}

// Optimizer private functions
void hierarchical_t::reset(const unsigned idx_) {
    start_component = component_t::L2;
    end_component = component_t::DRAM;
    total_cnt.clear();
    valid_cnt.clear();
    best_mappings.clear();
    return;
}

void hierarchical_t::engine(const unsigned idx_, 
                            const dataflow_t l1_dataflow_,
                            const dataflow_t l2_dataflow_) {
    std::vector<mapping_table_t> rtn_first;
    std::vector<mapping_table_t> rtn_second;
    std::vector<mapping_table_t> rtn_third;
    // Start optimizing from SEQ 0 to SEQ 2
    for(unsigned seq = 0; seq < SEQ_MAX; seq++) {
        // SEQ 0: L2-S2
        if(seq == 0) {
            if(used_levels.at(seq) == 0) {
                start_component = component_t::L1; 
                continue; 
            }
            else {
                // Mapping space generation
                mapping_space_t mapping_space(used_levels.at(seq) + 1, mappings.at(idx_ - 1).get_layer_values());
                total_cnt.push_back(mapping_space.get_num_permutations());
                // Run worker()
                worker(seq,
                       mappings.at(idx_ - 1),
                       mapping_space,
                       l1_dataflow_,
                       l2_dataflow_,
                       rtn_first);
                // Change start & end components
                start_component = component_t::L1;
                end_component = component_t::L2;
            }
        }
        // SEQ 1: L1-S1
        else if(seq == 1) {
            if(used_levels.at(seq) == 0) { 
                start_component = component_t::S0; 
                continue; 
            }
            else {
                for(unsigned i = 0; i < rtn_first.size(); i++) {
                    // Mapping space generation
                    mapping_space_t mapping_space(used_levels.at(seq) + 1, rtn_first.at(i).get_row_degrees(end_component));
                    total_cnt.push_back(mapping_space.get_num_permutations());
                    // Run worker()
                    worker(seq,
                           rtn_first.at(i),
                           mapping_space,
                           l1_dataflow_,
                           l2_dataflow_,
                           rtn_second);
                }
                // Change start & end components
                start_component = component_t::S0;
                end_component = component_t::L1;
            }
        }
        // SEQ 2: MAC-S0
        else {
            if(used_levels.at(seq) == 0) {
                continue;
            }
            else {
                for(unsigned i = 0; i < rtn_second.size(); i++) {
                    // Mapping space generation
                    mapping_space_t mapping_space(used_levels.at(seq) + 1, rtn_second.at(i).get_row_degrees(end_component));
                    total_cnt.push_back(mapping_space.get_num_permutations());
                    // Run worker()
                    worker(seq,
                           rtn_second.at(i),
                           mapping_space,
                           l1_dataflow_,
                           l2_dataflow_,
                           rtn_third);
                }
            }
        }
    }
    // Best mappings
    if(rtn_third.size() != 0) 
        best_mappings.assign(rtn_third.begin(), rtn_third.end());
    else {
        if(rtn_second.size() != 0) 
            best_mappings.assign(rtn_second.begin(), rtn_second.end());
        else {
            if(rtn_first.size() != 0)  
                best_mappings.assign(rtn_first.begin(), rtn_first.end());
        }
    }
    return;
}

void hierarchical_t::update(const dataflow_t l1_dataflow_,
                            const dataflow_t l2_dataflow_) {
    unsigned local_best_idx = 0;
    double local_min_energy = DBL_MAX;
    for(size_t idx = 0; idx < best_mappings.size(); idx++) {
        stats_t curr_stats(accelerator, best_mappings.at(idx), l1_dataflow_, l2_dataflow_);
        curr_stats.update_stats();
        if(local_min_energy > curr_stats.get_total_energy()) {
            local_min_energy = curr_stats.get_total_energy();
            local_best_idx = idx;
        }
    }
    if(final_best_energy > local_min_energy) {
        final_best_idx = local_best_idx;
        final_best_energy = local_min_energy;
        final_best_mappings.clear();
        final_best_mappings.assign(best_mappings.begin(), best_mappings.end());
        final_best_dataflows.clear();
        final_best_dataflows.push_back(l1_dataflow_);
        final_best_dataflows.push_back(l2_dataflow_);
        seq_0_top_k = final_best_idx / (top_k.at(1) * top_k.at(2)) + 1;
        seq_1_top_k = (final_best_idx - ((seq_0_top_k - 1) * top_k.at(1) * top_k.at(2))) / top_k.at(2) + 1;
        seq_2_top_k = final_best_idx % top_k.at(2) + 1;
    }
    return;
}

void hierarchical_t::worker(const unsigned seq_,
                            const mapping_table_t& init_mapping_,
                            const mapping_space_t& mapping_space_,
                            const dataflow_t l1_dataflow_,
                            const dataflow_t l2_dataflow_,
                            std::vector<mapping_table_t>& rtn_) {
    valid_cnt.push_back(0);
    // Validation required components 
    component_t temporal = component_t::SIZE;
    component_t spatial = component_t::SIZE;
    component_t bottom = component_t::SIZE;
    // TODO: if skipped
    if(seq_ == 0) {
        temporal = component_t::L2;
        spatial = component_t::S2;
        bottom = component_t::DRAM;
    }
    if(seq_ == 1) {
        temporal = component_t::L1;
        spatial = component_t::S1_X;
        bottom = component_t::L2;
    }
    if(seq_ == 2) {
        temporal = component_t::MAC;
        spatial = component_t::S0;
        bottom = component_t::L1;
    }
    // Local best mappings
    std::map<double, mapping_table_t> local_best_mappings;
    local_best_mappings.insert(std::make_pair(DBL_MAX, init_mapping_));
    // Current energy & mapping
    double curr_energy = DBL_MAX;
    mapping_table_t curr_mapping(init_mapping_);
    // Start finding best mappings
    for(size_t g = 0; g < mapping_space_.get_permutations(0).size(); g++) {
        curr_mapping.put_column_degrees(parameter_t::G, mapping_space_.get_permutations(0).at(g), start_component, end_component);
        for(size_t k = 0; k < mapping_space_.get_permutations(1).size(); k++) {
            curr_mapping.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(1).at(k), start_component, end_component);
            for(size_t b = 0; b < mapping_space_.get_permutations(2).size(); b++) {
                curr_mapping.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(2).at(b), start_component, end_component);
                for(size_t p = 0; p < mapping_space_.get_permutations(3).size(); p++) {
                    curr_mapping.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(3).at(p), start_component, end_component);
                    for(size_t q = 0; q < mapping_space_.get_permutations(4).size(); q++) {
                        curr_mapping.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(4).at(q), start_component, end_component);
                        for(size_t c = 0; c < mapping_space_.get_permutations(5).size(); c++) {
                            curr_mapping.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(5).at(c), start_component, end_component);
                            for(size_t s = 0; s < mapping_space_.get_permutations(6).size(); s++) {
                                curr_mapping.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(6).at(s), start_component, end_component);
                                for(size_t r = 0; r < mapping_space_.get_permutations(7).size(); r++) {
                                    curr_mapping.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(7).at(r), start_component, end_component);
                                    // Validity check
                                    if(check_validity(temporal, curr_mapping) & check_validity(spatial, curr_mapping)) {
                                        // Get current energy
                                        stats_t curr_stats(accelerator, curr_mapping, l1_dataflow_, l2_dataflow_);
                                        curr_stats.update_stats();
                                        curr_energy = curr_stats.get_energy(bottom);
                                        // 'local_best_mappings' includes the same energy key value with the current energy
                                        auto entry = local_best_mappings.find(curr_energy);
                                        if(entry == local_best_mappings.end()) {
                                            // Comparison between the last candidate (highest energy) and the current energy
                                            if(std::prev(local_best_mappings.end())->first > curr_energy) 
                                                local_best_mappings.insert(std::make_pair(curr_energy, curr_mapping));
                                        }
                                        else {
                                            // Same cost then, compare bottom iterations
                                            if(entry->second.get_iteration(bottom) > curr_mapping.get_iteration(bottom))
                                                entry->second.swap_degrees(curr_mapping.get_degrees());
                                        }
                                        if(local_best_mappings.size() > top_k.at(seq_)) 
                                           local_best_mappings.erase(std::prev(local_best_mappings.end())); 
                                        valid_cnt.at(valid_cnt.size() - 1)++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } 
    for(auto it = local_best_mappings.begin(); it != local_best_mappings.end(); ++it) 
        rtn_.push_back(it->second);
    if(local_best_mappings.size() != top_k.at(seq_)) {
        for(unsigned i = 0; i < top_k.at(seq_) - local_best_mappings.size(); i++) 
            rtn_.push_back(init_mapping_);
    }
    return;
}
