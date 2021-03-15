#include "optimizer.h"

#define SEQ_MAX 3
#define TOP_FIRST 3
#define TOP_SECOND 4
#define TOP_THIRD 3

static handler_t handler;

/* Systematic */
systematic_t::systematic_t(const std::string& acc_cfg_path_, 
                           const std::string& net_cfg_path_, 
                           const bool is_fixed_) 
    : optimizer_t(acc_cfg_path_, net_cfg_path_, is_fixed_),
      used_levels(0), 
      start_component(component_t::L2), 
      end_component(component_t::DRAM),
      num_permutations_first(0),
      valid_cnt_first(0) {
    // L2-DRAM (0), L1-L2 (1), and MAC-L1 (2)
    static_top_k.push_back(TOP_FIRST);
    static_top_k.push_back(TOP_SECOND);
    static_top_k.push_back(TOP_THIRD);
}     

systematic_t::~systematic_t() {

}

// Optimizer APIs
void systematic_t::run() {
    for(unsigned idx = 0; idx < mapping_tables.size(); idx++) 
        run(idx + 1);
    return;
}

void systematic_t::run(const unsigned idx_) {
    if(is_fixed) {
        // Initialization
        reset_variables();
        // Start optimizing 
        for(unsigned seq = 0; seq < SEQ_MAX; seq++) {
            used_levels = 0;
            // L2 & S2
            if(seq == 0) {
                if(accelerator->s2_size() > 1) used_levels++;
                if(accelerator->l2_type() != buffer_type_t::NONE) used_levels++;
                if(used_levels == 0) { 
                    dynamic_top_k.at(0) = 1;
                    best_mappings_first.assign(dynamic_top_k.at(0), mapping_tables.at(idx_ - 1));
                    start_component = component_t::L1; 
                    continue; 
                }
                // Mapping space generation
                mapping_space_t mapping_space(used_levels + 1, mapping_tables.at(idx_ - 1).get_layer_values());
                num_permutations_first = mapping_space.get_num_permutations();
                // Run worker()
                worker(seq,
                       dynamic_top_k.at(0),
                       mapping_tables.at(idx_ - 1),
                       mapping_space,
                       start_component,
                       end_component,
                       accelerator->l1_dataflow(),
                       accelerator->l2_dataflow(),
                       valid_cnt_first,
                       best_mappings_first);
                // Change start & end components
                start_component = component_t::L1;
                end_component = component_t::L2;
            }
            // L1 & S1
            else if(seq == 1) {
                if(accelerator->s1_size_x() > 1) used_levels++;
                if(accelerator->s1_size_y() > 1) used_levels++;
                if(accelerator->l1_type() != buffer_type_t::NONE) used_levels++;
                if(used_levels == 0) { 
                    dynamic_top_k.at(1) = 1;
                    for(unsigned i = 0; i < dynamic_top_k.at(0); i++)
                        best_mappings_second.push_back(best_mappings_first.at(i));
                    start_component = component_t::S0; 
                    continue; 
                }
                num_permutations_second.assign(dynamic_top_k.at(0), 0);
                valid_cnt_second.assign(dynamic_top_k.at(0), 0);
                for(unsigned i = 0; i < dynamic_top_k.at(0); i++) {
                    // Mapping space generation
                    mapping_space_t mapping_space(used_levels + 1, best_mappings_first.at(i).get_row_degrees(end_component));
                    num_permutations_second.at(i) = mapping_space.get_num_permutations();
                    // Run worker()
                    worker(seq,
                           dynamic_top_k.at(1),
                           best_mappings_first.at(i),
                           mapping_space,
                           start_component,
                           end_component,
                           accelerator->l1_dataflow(),
                           accelerator->l2_dataflow(),
                           valid_cnt_second.at(i),
                           best_mappings_second);
                }
                // Change start & end components
                start_component = component_t::S0;
                end_component = component_t::L1;
            }
            // MAC & S0
            else {
                if(accelerator->macs_per_pe() * accelerator->mac_width() > 1) used_levels++;
                if(used_levels == 0) {
                    dynamic_top_k.at(2) = 1;
                    for(unsigned i = 0; i < dynamic_top_k.at(0) * dynamic_top_k.at(1); i++)
                        best_mappings_third.push_back(best_mappings_second.at(i));
                    continue;
                }
                num_permutations_third.assign(dynamic_top_k.at(0) * dynamic_top_k.at(1), 0);
                valid_cnt_third.assign(dynamic_top_k.at(0) * dynamic_top_k.at(1), 0);
                for(unsigned i = 0; i < dynamic_top_k.at(0) * dynamic_top_k.at(1); i++) {
                    // Mapping space generation
                    mapping_space_t mapping_space(used_levels + 1, best_mappings_second.at(i).get_row_degrees(end_component));
                    num_permutations_third.at(i) = mapping_space.get_num_permutations();
                    // Run worker()
                    worker(seq,
                           dynamic_top_k.at(2),
                           best_mappings_second.at(i),
                           mapping_space,
                           start_component,
                           end_component,
                           accelerator->l1_dataflow(),
                           accelerator->l2_dataflow(),
                           valid_cnt_third.at(i),
                           best_mappings_third);
                }
            }
        }
        // Print stats
        std::cout << "# NETWORK    : " << network_name << std::endl;
        std::cout << "# " << mapping_tables.at(idx_ - 1).get_layer_name() << std::endl;
        handler.print_line(50, "*");
        print_stats(idx_);
        handler.print_line(50, "*");
        for(unsigned i = 0; i < dynamic_top_k.at(0); i++) {
            for(unsigned j = 0; j < dynamic_top_k.at(1); j++) {
                for(unsigned k = 0; k < dynamic_top_k.at(2); k++) {
                    std::cout << "\n# TOP " << i + 1 << "-" << j + 1 << "-" << k + 1 << std::endl;
                    stats_t curr_stats(accelerator, best_mappings_third.at(k + j * dynamic_top_k.at(2) + i * dynamic_top_k.at(1) * dynamic_top_k.at(2)));
                    curr_stats.update_stats();
#ifdef SIMPLE
                    curr_stats.print_csv();
#else
#ifdef CSV
                    best_mappings_third.at(k + j * dynamic_top_k.at(2) + i * dynamic_top_k.at(1) * dynamic_top_k.at(2)).print_csv();
                    curr_stats.print_csv();
#else
                    best_mappings_third.at(k + j * dynamic_top_k.at(2) + i * dynamic_top_k.at(1) * dynamic_top_k.at(2)).print_stats();
                    curr_stats.print_stats();
#endif
#endif
                }
            }
        }
    }
    else {
        // Start optimizing
        std::string df_str("IWO");
        for(unsigned l1_df = 0; l1_df < l1_dataflow.size(); l1_df++) {
            for(unsigned l2_df = 0; l2_df < l2_dataflow.size(); l2_df++) {
                // Initialization
                reset_variables();
                // Start optimizing 
                for(unsigned seq = 0; seq < SEQ_MAX; seq++) {
                    used_levels = 0;
                    // L2 & S2
                    if(seq == 0) {
                        if(accelerator->s2_size() > 1) used_levels++;
                        if(accelerator->l2_type() != buffer_type_t::NONE) used_levels++;
                        if(used_levels == 0) { 
                            dynamic_top_k.at(0) = 1;
                            best_mappings_first.assign(dynamic_top_k.at(0), mapping_tables.at(idx_ - 1));
                            start_component = component_t::L1; 
                            continue; 
                        }
                        // Mapping space generation
                        mapping_space_t mapping_space(used_levels + 1, mapping_tables.at(idx_ - 1).get_layer_values());
                        num_permutations_first = mapping_space.get_num_permutations();
                        // Run worker()
                        worker(seq,
                               dynamic_top_k.at(0),
                               mapping_tables.at(idx_ - 1),
                               mapping_space,
                               start_component,
                               end_component,
                               static_cast<dataflow_t>(l1_dataflow.at(l1_df)),
                               static_cast<dataflow_t>(l2_dataflow.at(l2_df)),
                               valid_cnt_first,
                               best_mappings_first);
                        // Change start & end components
                        start_component = component_t::L1;
                        end_component = component_t::L2;
                    }
                    // L1 & S1
                    else if(seq == 1) {
                        if(accelerator->s1_size_x() > 1) used_levels++;
                        if(accelerator->s1_size_y() > 1) used_levels++;
                        if(accelerator->l1_type() != buffer_type_t::NONE) used_levels++;
                        if(used_levels == 0) { 
                            dynamic_top_k.at(1) = 1;
                            for(unsigned i = 0; i < dynamic_top_k.at(0); i++)
                                best_mappings_second.push_back(best_mappings_first.at(i));
                            start_component = component_t::S0; 
                            continue; 
                        }
                        num_permutations_second.assign(dynamic_top_k.at(0), 0);
                        valid_cnt_second.assign(dynamic_top_k.at(0), 0);
                        for(unsigned i = 0; i < dynamic_top_k.at(0); i++) {
                            // Mapping space generation
                            mapping_space_t mapping_space(used_levels + 1, best_mappings_first.at(i).get_row_degrees(end_component));
                            num_permutations_second.at(i) = mapping_space.get_num_permutations();
                            // Run worker()
                            worker(seq,
                                   dynamic_top_k.at(1),
                                   best_mappings_first.at(i),
                                   mapping_space,
                                   start_component,
                                   end_component,
                                   static_cast<dataflow_t>(l1_dataflow.at(l1_df)),
                                   static_cast<dataflow_t>(l2_dataflow.at(l2_df)),
                                   valid_cnt_second.at(i),
                                   best_mappings_second);
                        }
                        // Change start & end components
                        start_component = component_t::S0;
                        end_component = component_t::L1;
                    }
                    // MAC & S0
                    else {
                        if(accelerator->macs_per_pe() * accelerator->mac_width() > 1) used_levels++;
                        if(used_levels == 0) {
                            dynamic_top_k.at(2) = 1;
                            for(unsigned i = 0; i < dynamic_top_k.at(0) * dynamic_top_k.at(1); i++)
                                best_mappings_third.push_back(best_mappings_second.at(i));
                            continue;
                        }
                        num_permutations_third.assign(dynamic_top_k.at(0) * dynamic_top_k.at(1), 0);
                        valid_cnt_third.assign(dynamic_top_k.at(0) * dynamic_top_k.at(1), 0);
                        for(unsigned i = 0; i < dynamic_top_k.at(0) * dynamic_top_k.at(1); i++) {
                            // Mapping space generation
                            mapping_space_t mapping_space(used_levels + 1, best_mappings_second.at(i).get_row_degrees(end_component));
                            num_permutations_third.at(i) = mapping_space.get_num_permutations();
                            // Run worker()
                            worker(seq,
                                   dynamic_top_k.at(2),
                                   best_mappings_second.at(i),
                                   mapping_space,
                                   start_component,
                                   end_component,
                                   static_cast<dataflow_t>(l1_dataflow.at(l1_df)),
                                   static_cast<dataflow_t>(l2_dataflow.at(l2_df)),
                                   valid_cnt_third.at(i),
                                   best_mappings_third);
                        }
                    }
                }
                // Print stats
#ifdef SIMPLE
                if(l1_df == 0 && l2_df == 0) {
                    std::cout << "# NETWORK    : " << network_name << std::endl;
                    std::cout << "# " << mapping_tables.at(idx_ - 1).get_layer_name() << std::endl;
                    handler.print_line(50, "*");
                }
#else
                std::cout << "# NETWORK    : " << network_name << std::endl;
                std::cout << "# " << mapping_tables.at(idx_ - 1).get_layer_name() << std::endl;
                handler.print_line(50, "*");
#endif
                print_stats(idx_);
                handler.print_line(50, "*");
                std::cout << "# DATAFLOWS: " << df_str.at(l1_dataflow.at(l1_df)) << "S-" << df_str.at(l2_dataflow.at(l2_df)) << "S" << std::endl; 
                for(unsigned i = 0; i < dynamic_top_k.at(0); i++) {
                    for(unsigned j = 0; j < dynamic_top_k.at(1); j++) {
                        for(unsigned k = 0; k < dynamic_top_k.at(2); k++) {
                            std::cout << "\n# TOP " << i + 1 << "-" << j + 1 << "-" << k + 1 << std::endl;
                            stats_t curr_stats(accelerator, best_mappings_third.at(k + j * dynamic_top_k.at(2) + i * dynamic_top_k.at(1) * dynamic_top_k.at(2)));
                            curr_stats.update_stats();
#ifdef SIMPLE
                            curr_stats.print_csv();
#else
#ifdef CSV
                            best_mappings_third.at(k + j * dynamic_top_k.at(2) + i * dynamic_top_k.at(1) * dynamic_top_k.at(2)).print_csv();
                            curr_stats.print_csv();
#else
                            best_mappings_third.at(k + j * dynamic_top_k.at(2) + i * dynamic_top_k.at(1) * dynamic_top_k.at(2)).print_stats();
                            curr_stats.print_stats();
#endif
#endif
                        }
                    }
                }
            }
        }
    }
}

void systematic_t::reset_variables() {
    dynamic_top_k.clear();
    for(unsigned seq = 0; seq < SEQ_MAX; seq++) 
        dynamic_top_k.push_back(static_top_k.at(seq));
    start_component = component_t::L2;
    end_component = component_t::DRAM;
    num_permutations_first = 0;
    num_permutations_second.clear();
    num_permutations_third.clear();
    valid_cnt_first = 0;
    valid_cnt_second.clear();
    valid_cnt_third.clear();
    best_mappings_first.clear();
    best_mappings_second.clear();
    best_mappings_third.clear();
    return;
}

void systematic_t::print_stats(const unsigned idx_) {
    mapping_space_t original_space(num_levels - 1, mapping_tables.at(idx_ - 1).get_layer_values());
#ifdef CSV
    std::cout << "# SEQ 0"<< std::endl;
    std::cout << "#,ORIGINAL,L2-S2 TOTAL,L2-S2 VALID\n" 
              << "," << original_space.get_num_permutations()
              << "," << num_permutations_first
              << "," << valid_cnt_first << std::endl;
    std::cout << "# SEQ 1" << std::endl;
    for(unsigned i = 0; i < num_permutations_second.size(); i++) {
        std::cout << "#,L1-S1 TOTAL,L1-S1 VALID\n"
                  << "," << num_permutations_second.at(i) 
                  << "," << valid_cnt_second.at(i) << std::endl;
    }
    std::cout << "# SEQ 2 " << std::endl;
    for(unsigned i = 0; i < num_permutations_third.size(); i++) {
        std::cout << "#,MAC-S0 TOTAL,MAC-S0 VALID\n"
                  << "," << num_permutations_third.at(i)
                  << "," << valid_cnt_third.at(i) << std::endl;
    }
#else
    std::cout << "# SEQ 0"<< std::endl;
    std::cout << "#       ORIGINAL PERMUTATIONS: " << original_space.get_num_permutations() << std::endl;
    std::cout << "#    L2-S2 TOTAL PERMUTATIONS: " << num_permutations_first << std::endl;
    std::cout << "#    L2-S2 VALID PERMUTATIONS: " << valid_cnt_first << std::endl;
    std::cout << "# SEQ 1" << std::endl;
    for(unsigned i = 0; i < num_permutations_second.size(); i++) {
        std::cout << "#    L1-S1 TOTAL PERMUTATIONS: " << num_permutations_second.at(i) << std::endl;
        std::cout << "#    L1-S1 VALID PERMUTATIONS: " << valid_cnt_second.at(i) << std::endl;
    }
    std::cout << "# SEQ 2" << std::endl;
    for(unsigned i = 0; i < num_permutations_third.size(); i++) {
        std::cout << "#   MAC-S0 TOTAL PERMUTATIONS: " << num_permutations_third.at(i) << std::endl;
        std::cout << "#   MAC-S0 VALID PERMUTATIONS: " << valid_cnt_third.at(i) << std::endl;
    }
#endif
    return;
}

// Mapping worker
void systematic_t::worker(const unsigned seq_,
                          const unsigned top_k_,
                          const mapping_table_t& init_mapping_,
                          const mapping_space_t& mapping_space_,
                          const component_t start_,
                          const component_t end_,
                          const dataflow_t l1_dataflow_,
                          const dataflow_t l2_dataflow_,
                          uint64_t& valid_cnt_,
                          std::vector<mapping_table_t>& best_mappings_) {
    // Validation required components 
    component_t check_1 = component_t::SIZE;
    component_t check_2 = component_t::SIZE;
    component_t energy_check = component_t::SIZE;
    if(start_ == component_t::L2) {
        check_1 = component_t::L2;
        check_2 = component_t::S2;
        energy_check = component_t::DRAM;
    }
    else if(start_ == component_t::L1) {
        check_1 = component_t::L1;
        check_2 = component_t::S1_X;
        energy_check = component_t::L2;
    }
    else if(start_ == component_t::S0) {
        check_1 = component_t::MAC;
        check_2 = component_t::S0;
        energy_check = component_t::L1;
    }
    else 
        handler.print_err(err_type_t::INVAILD, "COMPONENT systematic_t::worker");
    // Best mappings
    std::map<size_t, mapping_table_t> local_best_mappings;
    local_best_mappings.insert(std::make_pair(-1, init_mapping_));
    // Current mapping table
    mapping_table_t curr_mapping_table(init_mapping_);
    // Start finding best mappings
    for(size_t k = 0; k < mapping_space_.get_permutations(0).size(); k++) {
        curr_mapping_table.put_column_degrees(parameter_t::K, mapping_space_.get_permutations(0).at(k), start_, end_);
        for(size_t b = 0; b < mapping_space_.get_permutations(1).size(); b++) {
            curr_mapping_table.put_column_degrees(parameter_t::B, mapping_space_.get_permutations(1).at(b), start_, end_);
            for(size_t p = 0; p < mapping_space_.get_permutations(2).size(); p++) {
                curr_mapping_table.put_column_degrees(parameter_t::P, mapping_space_.get_permutations(2).at(p), start_, end_);
                for(size_t q = 0; q < mapping_space_.get_permutations(3).size(); q++) {
                    curr_mapping_table.put_column_degrees(parameter_t::Q, mapping_space_.get_permutations(3).at(q), start_, end_);
                    for(size_t c = 0; c < mapping_space_.get_permutations(4).size(); c++) {
                        curr_mapping_table.put_column_degrees(parameter_t::C, mapping_space_.get_permutations(4).at(c), start_, end_);
                        for(size_t s = 0; s < mapping_space_.get_permutations(5).size(); s++) {
                            curr_mapping_table.put_column_degrees(parameter_t::S, mapping_space_.get_permutations(5).at(s), start_, end_);
                            for(size_t r = 0; r < mapping_space_.get_permutations(6).size(); r++) {
                                curr_mapping_table.put_column_degrees(parameter_t::R, mapping_space_.get_permutations(6).at(r), start_, end_);
                                // Validity check
                                if(check_validity(check_1, curr_mapping_table) & check_validity(check_2, curr_mapping_table)) {
                                    // Update current stats
                                    stats_t curr_stats(accelerator, curr_mapping_table, l1_dataflow_, l2_dataflow_);
                                    curr_stats.update_stats();
                                    // 'local_best_mappings' includes the same energy key value with the curr mapping table's energy
                                    auto entry = local_best_mappings.find(curr_stats.get_energy(energy_check));
                                    if(entry != local_best_mappings.end()) {
                                        // DRAM iteration comparison 
                                        if(entry->second.get_iteration(end_) > curr_mapping_table.get_iteration(end_)) {
                                            // Change the previous table to the current table
                                            mapping_table_t to_be_saved(init_mapping_); 
                                            entry->second.swap_degrees(curr_mapping_table.get_degrees());
                                            local_best_mappings.insert(std::make_pair(curr_stats.get_energy(energy_check), to_be_saved));
                                        }
                                    }
                                    else {
                                        // Comparison between the last candidate (highest energy) and the current mapping table's energy
                                        if(std::prev(local_best_mappings.end())->first > curr_stats.get_energy(energy_check)) {
                                            mapping_table_t to_be_saved(init_mapping_); 
                                            to_be_saved.swap_degrees(curr_mapping_table.get_degrees());
                                            local_best_mappings.insert(std::make_pair(curr_stats.get_energy(energy_check), to_be_saved));
                                        }
                                    }
                                    valid_cnt_++;
                                    if(local_best_mappings.size() > top_k_) 
                                       local_best_mappings.erase(std::prev(local_best_mappings.end())); 
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for(auto it = local_best_mappings.begin(); it != local_best_mappings.end(); ++it) 
        best_mappings_.push_back(it->second);
    return;
}
