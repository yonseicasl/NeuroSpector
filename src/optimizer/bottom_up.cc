#include <climits>
#include <float.h>
#include <unistd.h>
#include <thread>

#include "bottom_up.h"

bottom_up_t::bottom_up_t(const std::string& accelerator_pth_,
                         const std::string& dataflow_,
                         const std::string& network_pth_,
                         const std::string& layer_,
                         const std::string& metric_,
                         const std::string& cl_optimization_)
    : optimizer_t(accelerator_pth_, dataflow_, network_pth_, layer_),
      best_cost_of_multiple_layers(FLT_MAX),
      metric(metric_t::ENERGY) {
          std::cerr << "[message] construct bottom_up class" << std::endl;
          // Init metric
          metric = (metric_t)get_enum_type(metric_str, metric_);
          // Init cross_layer_optimziation
          is_cross_layer_opt = cl_optimization_ == "true" ? true : false;
}

bottom_up_t::~bottom_up_t() {

};
// Run bottom-up search for all layers
void bottom_up_t::run() {
    assert(layer_idx <= network->get_num_layers());
    for(unsigned idx = 0; idx < network->get_num_layers(); idx++) {
        if(layer_idx != 0) { if((idx + 1) != layer_idx) continue; }
        scheduling_table->load_dnn_layer(idx); 
        run(idx);
    }
}
// Run optimal scheduling option that processing multiple layers in parallel.
void bottom_up_t::run(const std::vector<unsigned> indices_) {
    std::vector<scheduling_table_t> scheduling_tables;
    std::cerr << "[message] Run mulitple layers in parallel" << std::endl;
    // Generate multiple scheduling table
    for(unsigned i = 0 ; i < indices_.size(); i++) {
        scheduling_table->load_dnn_layer(indices_.at(i) - 1);
        scheduling_table->print_stats();
        scheduling_tables.push_back(*scheduling_table);
    }
    std::cerr << "[message] Run chip partitioning" << std::endl;
    // Identify the number of chips to assign to each neural layer
    if(accelerator->get_total_num_chips() > 1) {
        std::cerr << "[message] Target Acc. has multiple # chips" << std::endl;
        multi_chip_partitioning(scheduling_tables);
    }
    std::cerr << "[message] Target Acc. is single chip" << std::endl;
    // Optimize per-layer scheduling
    unsigned idx = 0;
    for(auto it = scheduling_tables.begin();
			 it != scheduling_tables.end();
			 ++it) {
        // Load the incomplete scheduling table
		*scheduling_table = *it;
        // Fill out rest rows of scheduling table
		run(indices_.at(idx) - 1);
        // Store the completed scheduling table
		*it = *scheduling_table;
        idx++;
	}
    // Print out multi-chip partitioning results
    print_mcp_results();
}
// Run bottom-up search for a layer
void bottom_up_t::run(const unsigned idx_) {
    std::cerr << "[message] Run bottom-up search for a layer #" << idx_ + 1 << std::endl;
    // Update target layer info.
    global_best_scheduling_option.scheduling_table = *scheduling_table;
    global_best_scheduling_option.strategy.clear();
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
    list_of_scheduling_table.push_back(global_best_scheduling_option.scheduling_table);
}
// Print results
void bottom_up_t::print_results() {
    analyzer_t analyzer(accelerator, network);
    std::cerr << "[message] Print out optimization result" << std::endl;
    std::string strategy[2] = {"PM", "SP"};
    std::cout << "[message] Strategies :";
    for(auto it = global_best_scheduling_option.strategy.begin();
             it != global_best_scheduling_option.strategy.end();
             ++it) {
        std::cout << strategy[(unsigned)*it];
        if(it+1 != global_best_scheduling_option.strategy.end()) { std::cout<< "-"; }
    }
    std::cout << std::endl;
    analyzer.init(global_best_scheduling_option.scheduling_table);
    analyzer.estimate_cost();
    // If cross layer optimization is activated
    if(is_cross_layer_opt && !list_of_scheduling_table.empty()) {
        analyzer.estimate_cross_layer_reuse(list_of_scheduling_table.back(), metric);
    }
    analyzer.print_results(); 
    analyzer.reset();
}
// Print multi-chip partitioning results
void bottom_up_t::print_mcp_results() {
    float total_cost = 0.0f;
    analyzer_t analyzer(accelerator, network);
    // Analyze multiple scheduling tables
    for(auto it = list_of_scheduling_table.begin();
             it != list_of_scheduling_table.end();
             ++it) {
        analyzer.init(*it);
        analyzer.estimate_cost();
        total_cost += analyzer.get_total_cost(metric);
        analyzer.reset();
    }
    std::cout << "[message] Multip chip partitioning result : "
              << total_cost - best_cost_of_multiple_layers
              << std::endl;
}
// Reset all variables
void bottom_up_t::reset() {
    best_scheduling_option.scheduling_table = *scheduling_table;
    best_scheduling_option.strategy.clear();
}
// Optimize for a given dataflow combination
void bottom_up_t::engine() {
    std::cerr << "[message] Engine start" << std::endl;
    unsigned begin_pos, end_pos;
    std::vector<StrategyContainer> scheduling_candidates;

    // Initialize candidate and position
    StrategyContainer init_candidate;
    init_candidate.scheduling_table = *scheduling_table;
    scheduling_candidates.push_back(init_candidate);
    std::vector<std::vector<StrategyContainer>> output_candidates;

    std::vector<std::thread> workers;
    std::mutex m;
    
    begin_pos = scheduling_table->get_num_rows() - 1;
    // Compare DRAM row and dnn parameters
    std::vector<unsigned> dram_rows = scheduling_table->get_row_values(begin_pos);
    std::vector<unsigned> layer_param = scheduling_table->get_layer_parameters();
    for(unsigned i = 0; i < dram_rows.size(); i++) {
        if(dram_rows.at(i) != layer_param.at(i)) {
            begin_pos = scheduling_table->get_above_buffer_pos(begin_pos);
            break;
        }
    }
    // Walk along the scheduling table and optimize targeted levels
    while(begin_pos != 0) {
        // Change the targeted levels
        end_pos   = begin_pos;
        begin_pos = scheduling_table->get_above_buffer_pos(end_pos);
        // Candiates are concurrently explored
        for(unsigned tid = 0; tid < scheduling_candidates.size(); tid++) {
            output_candidates.resize(scheduling_candidates.size());
            workers.push_back(std::thread(&bottom_up_t::search,
                                          this,
                                          tid,
                                          begin_pos,
                                          end_pos,
                                          std::ref(scheduling_candidates.at(tid)),
                                          std::ref(output_candidates.at(tid)),
                                          std::ref(m)));
        }
        for(unsigned tid = 0; tid < scheduling_candidates.size(); tid++) {
            workers.at(tid).join();
        }
        workers.clear();
        scheduling_candidates.clear();
        // Merge all generated candidates of the targeted levels
        for(unsigned i = 0; i < output_candidates.size(); i++) {
            for(auto it = output_candidates.at(i).begin();
                     it != output_candidates.at(i).end();
                     ++it) {
                scheduling_candidates.push_back(*it);
            }
        }
        output_candidates.clear();
    }

    // Select best scheduling option
    analyzer_t analyzer(accelerator, network);
    float best_cost = FLT_MAX;
    for(auto it = scheduling_candidates.begin();
             it != scheduling_candidates.end();
             ++it) {
        analyzer.init(it->scheduling_table);
        analyzer.estimate_cost();
        if(analyzer.get_total_cost(metric) < best_cost) {
            best_cost = analyzer.get_total_cost(metric);
            best_scheduling_option = *it;
        }
    }
    analyzer.reset();
    return;
}
// Update the optimal scheduling table
void bottom_up_t::update() {
    // best_scheduling_option vs global_best_scheduling_option
    // Select global best scheduling option
    analyzer_t analyzer(accelerator, network);
    float best_df_cost = FLT_MAX;
    float curr_df_cost = FLT_MAX;

    analyzer.init(best_scheduling_option.scheduling_table);
    analyzer.estimate_cost();
    curr_df_cost = analyzer.get_total_cost(metric);
    analyzer.reset();

    analyzer.init(global_best_scheduling_option.scheduling_table);
    analyzer.estimate_cost();
    best_df_cost = analyzer.get_total_cost(metric);
    analyzer.reset();

    if(curr_df_cost < best_df_cost) {
        global_best_scheduling_option = best_scheduling_option;
    }
}
// Search for an optimal mapping option for a given dataflow
void bottom_up_t::search(unsigned tid_,
                         unsigned begin_pos_,
                         unsigned end_pos_,
                         StrategyContainer& scheduling_candidates_,
                         std::vector<StrategyContainer>& output_candidates_,
                         std::mutex& m_) {
    float    cost_pm        = FLT_MAX;
    float    cost_sp        = FLT_MAX;
    unsigned largest_facto_degree = 0;
    unsigned spatial_pos = begin_pos_ + 1; 
    std::vector<std::vector<unsigned>> mapping_values_set;

    accelerator_t m_accelerator = accelerator_t(*accelerator);
    analyzer_t analyzer(&m_accelerator, network);

    scheduling_table_t scheduling_table_curr = scheduling_candidates_.scheduling_table;
    scheduling_table_t scheduling_table_pm = scheduling_candidates_.scheduling_table;
    scheduling_table_t scheduling_table_sp = scheduling_candidates_.scheduling_table;

    // Find a mapping values that are yet to be optimized
    std::vector<unsigned> undetermined_values = scheduling_table_curr.get_row_values(end_pos_);
    // Get num targeted levels
    unsigned num_targeted_levels = get_num_targeted_levels(spatial_pos, end_pos_);
    // Generate mapping space
    mapping_space_t mapping_space;
    mapping_space_t temporal_mapping_space;
    mapping_space.generate(num_targeted_levels, undetermined_values);
    // Traverse all possible mapping options in targeted levels
    while(!mapping_space.is_last()) {
        // Get new mapping values
        mapping_values_set = mapping_space.get_mapping_set();
        // Update mapping values of scheduling table
        scheduling_table_curr.update_set_of_rows(spatial_pos, end_pos_,
                                                 mapping_values_set);
        // Reset analyzer's variables
        analyzer.reset();
        // Load scheduling table to analyzer
        analyzer.init(scheduling_table_curr);
        // Check_Validity
        if(!analyzer.check_validity()) continue;
        //	Re-generate mapping space for the remain levels of target set 
        num_targeted_levels = 2;
        undetermined_values = scheduling_table_curr.get_row_values(end_pos_);
        temporal_mapping_space.generate(num_targeted_levels, undetermined_values);
        while(!temporal_mapping_space.is_last()) {
            mapping_values_set = temporal_mapping_space.get_mapping_set();
            scheduling_table_curr.update_row(begin_pos_, mapping_values_set.at(0));
            scheduling_table_curr.update_row(end_pos_, mapping_values_set.at(1));
            analyzer.reset();
            analyzer.init(scheduling_table_curr);
            if(!analyzer.check_validity()) continue;
            // Consider scheduling option based on primary strategy
            optimize_with_primary_strategy(end_pos_,
                                        analyzer, cost_pm,
                                        scheduling_table_curr,
                                        scheduling_table_pm);
            // If the target optimization set is the top-most set,
            // NeuroSpector doesn't have to consider supplementary strategy.
            if(begin_pos_ == 0) continue;
            // Consider scheduling option supplementary strategy
            optimize_with_supplementary_strategy(begin_pos_, end_pos_,
                                                analyzer,
                                                cost_sp,
                                                largest_facto_degree,
                                                scheduling_table_curr,
                                                scheduling_table_sp);
        }
        temporal_mapping_space.clear();
        scheduling_table_curr.clear_set_of_rows(begin_pos_, end_pos_);
    }
    // Update Output candidates
    StrategyContainer candidate;
    // Update PM strategy
    candidate.strategy = scheduling_candidates_.strategy;
    candidate.strategy.push_back(strategy_t::PM);
    // Update PM scheduling table
    candidate.scheduling_table = scheduling_table_pm;
    output_candidates_.push_back(candidate);
    // Check redundency of SP scheduling table
    if(scheduling_table_pm != scheduling_table_sp && begin_pos_ != 0) {
        // If not, add SP scheduling table
        candidate.strategy = scheduling_candidates_.strategy;
        candidate.strategy.push_back(strategy_t::SP);
        candidate.scheduling_table = scheduling_table_sp;
        output_candidates_.push_back(candidate);
    }
    mapping_space.clear();
    return;
}
// Find optimal scheudling option based on primary strategy (PM)
void bottom_up_t::optimize_with_primary_strategy(unsigned end_pos_,
                                                 analyzer_t& analyzer_,
                                                 float& lowest_cost_,
                                                 scheduling_table_t& curr_table_,
                                                 scheduling_table_t& pm_table_) {
    float curr_cost = 0.0;
    curr_cost = analyzer_.get_target_level_cost(end_pos_, metric);
    // If current working set is DRAM-GLB and cross-layer optimization is ON
    if(end_pos_ == curr_table_.get_num_rows() - 1 && is_cross_layer_opt 
       && !list_of_scheduling_table.empty()) {
        analyzer_.estimate_cross_layer_reuse(list_of_scheduling_table.back(), metric);
    }
    if(curr_cost < lowest_cost_) {
        // Update best cost and best scheduling table
        lowest_cost_ = curr_cost;
        pm_table_    = curr_table_;
    }
    // If the cost is the same, compare # of lower level baseline access count
    else if (curr_cost == lowest_cost_) {
        // Update baseline access count
        unsigned iter_curr = 1, iter_pm = 1;
        std::vector<unsigned> row_curr = curr_table_.get_row_values(end_pos_);
        std::vector<unsigned> row_pm   = pm_table_.get_row_values(end_pos_);
        for(auto value = row_curr.begin(); value != row_curr.end(); ++value) {
            iter_curr *= *value;
        }
        for(auto value = row_pm.begin(); value != row_pm.end(); ++value) {
            iter_pm *= *value;
        }
        // Compare access count
        if(iter_curr < iter_pm) { pm_table_ = curr_table_; }
        // If their access counts are the same, choose the option to send
        // larger stationary-related data tile. 
        else if (iter_curr == iter_pm) {
            iter_curr = curr_table_.get_dataflow_irrelevant_params_product(end_pos_);
            iter_pm = pm_table_.get_dataflow_irrelevant_params_product(end_pos_);
            if(iter_curr > iter_pm) { pm_table_ = curr_table_; }
        }
    }
    return;
}
// Find optimal scheudling option based on supplementary strategy (SP)
void bottom_up_t::optimize_with_supplementary_strategy(unsigned begin_pos_,
                                                       unsigned end_pos_,
                                                   analyzer_t& analyzer_,
                                                   float& lowest_cost_,
                                                   unsigned& largest_facto_degree_,
                                                   scheduling_table_t& curr_table_,
                                                   scheduling_table_t& sp_table_) {
    unsigned curr_facto_degree = 0;
    float    curr_cost = 0.0;
    curr_facto_degree = analyzer_.get_target_level_factorization(begin_pos_);
    curr_cost         = analyzer_.get_target_level_cost(end_pos_, metric);
    if(curr_facto_degree > largest_facto_degree_) {
        largest_facto_degree_ = curr_facto_degree;
        sp_table_ = curr_table_;
    }
    else if (curr_facto_degree == largest_facto_degree_) {
        // Compare cost of scheduling options
        if(curr_cost < lowest_cost_) { 
            lowest_cost_ = curr_cost;
            sp_table_ = curr_table_; 
        }
    }
    return;
}
// Find the optimal way to process layers which are branched off the same root layer in parallel
void bottom_up_t::multi_chip_partitioning(std::vector<scheduling_table_t>& tables_) {
    std::vector<std::vector<PartitioningInfo>> partition_comb_list;
    std::vector<PartitioningInfo> tmp_partition_comb;
    std::vector<PartitioningInfo> opt_partition_comb;
    
    unsigned case_count      = 1; 
    unsigned num_total_cases = 1;
    unsigned num_total_activated_chips = 0;
    float    cost_of_multiple_layers = 0;
    best_cost_of_multiple_layers = FLT_MAX;
    std::vector<unsigned> indices;

    // Collects all possible partitioning cases and costs for each case
    for(auto it = tables_.begin(); it != tables_.end(); ++it) {
        std::cerr << "[message] Collecting partitioning cases" << std::endl;
        partition_comb_list.push_back(collect_partition_comb(*it));
        num_total_cases *= partition_comb_list.back().size();
    }
    indices.assign(partition_comb_list.size(), 0);

    // Checking all possible cases
    while(case_count < num_total_cases) {
        case_count++;
        // Get new partitioning combination
        indices.at(0)++;
        for(unsigned i = 0; i < partition_comb_list.size() - 1; i++) {
            if(indices.at(i) > partition_comb_list.at(i).size() - 1) {
                indices.at(i) = 0;
                indices.at(i + 1)++;
            }
        }
        // For each case combination, compute total cost
        for(unsigned i = 0; i < partition_comb_list.size(); i++) {
            tmp_partition_comb.push_back(partition_comb_list.at(i).at(indices.at(i)));
        }
        // Check chip validity
        for(auto it = tmp_partition_comb.begin(); 
                 it != tmp_partition_comb.end(); 
                 ++it) {
            if(it->is_parallelized == true) {
                num_total_activated_chips 
                    += it->num_assigned_chips;
            }
        }
        if(num_total_activated_chips > accelerator->get_total_num_chips()) { 
            tmp_partition_comb.clear();
            num_total_activated_chips = 0;
            cost_of_multiple_layers = 0;
            continue;
        }
        // Compute cost
        for(auto it = tmp_partition_comb.begin(); 
                 it != tmp_partition_comb.end(); 
                 ++it) {
            cost_of_multiple_layers += it->cost;
        }
        // For all parallelize layers, substract cost depends on overlapped input size 
        unsigned input_tile_size = 0;
        unsigned minimum_tile_size = UINT_MAX;
        unsigned num_same_tile_size = 0;
        unsigned num_total_parallelized_layer = 0;
        unsigned access_count = 1;
        unsigned dram_index   = accelerator->get_num_components() - 1;
        float    input_access_energy = accelerator->get_component_energy(dram_index).at((unsigned)data_t::INPUT);
        for(auto it = tmp_partition_comb.begin(); 
                 it != tmp_partition_comb.end(); 
                 ++it) {
            if(it->is_parallelized == true) {
                num_total_parallelized_layer++;
                // Get input tile size
                input_tile_size = it->input_tile_size;
                // Overlapped size
                if(input_tile_size == minimum_tile_size) {
                    num_same_tile_size++;
                }
                else { 
                    minimum_tile_size = (minimum_tile_size > input_tile_size) 
                                      ? input_tile_size : minimum_tile_size; 
                    num_same_tile_size = 1;
                }
            }
        }
        if(num_same_tile_size == num_total_parallelized_layer
        && num_total_parallelized_layer > 1) { 
            access_count = tmp_partition_comb.at(0).input_access_count;
        }
        if(num_same_tile_size == 0) {num_same_tile_size = 1; }
        if(num_same_tile_size > 1) {num_same_tile_size--;}
        if(num_total_parallelized_layer > 1) {
            cost_of_multiple_layers -= float(input_tile_size 
                                    * access_count 
                                    * num_same_tile_size )
                                    * input_access_energy;
        }
        // If the cost of current case is minimum, change the combination
        if(cost_of_multiple_layers < best_cost_of_multiple_layers) {
            best_cost_of_multiple_layers = cost_of_multiple_layers;
            opt_partition_comb       = tmp_partition_comb;
        }
        tmp_partition_comb.clear();

        num_same_tile_size = 0;
        num_total_parallelized_layer = 0;
        num_total_activated_chips = 0;
        cost_of_multiple_layers = 0;
        minimum_tile_size = UINT_MAX;
    }
    std::clog << best_cost_of_multiple_layers << std::endl;
    for(unsigned i = 0; i < opt_partition_comb.size(); i++) {
        tables_.at(i) = opt_partition_comb.at(i).scheduling_table;
    }
    return;
}
// Collect all possible partitioning cases for a layer
std::vector<PartitioningInfo> bottom_up_t::collect_partition_comb(scheduling_table_t table_) {
    std::vector<PartitioningInfo> partiton_comb;
    std::map<unsigned, PartitioningInfo> list_of_partition;
    
    unsigned dram_pos = table_.get_num_rows() - 1;
    unsigned glb_pos  = table_.get_above_buffer_pos(dram_pos);
    unsigned num_targeted_levels = get_num_targeted_levels(glb_pos, dram_pos);
    std::vector<unsigned> undetermined_values = table_.get_row_values(dram_pos);
    std::vector<std::vector<unsigned>> mapping_values_set;

    unsigned num_activated_chips = 0; 
    unsigned best_num_activated_chips = 0;
    float    best_cost = FLT_MAX;
    mapping_space_t mapping_space;
    analyzer_t analyzer(accelerator, network);

    mapping_space.generate(num_targeted_levels, undetermined_values);
    // Traverse all possible mapping options in targeted levels
    while(!mapping_space.is_last()) {
        // Get new mapping values
        mapping_values_set = mapping_space.get_mapping_set();
        // Update mapping values of scheduling table
        table_.update_set_of_rows(glb_pos, dram_pos, 
                                  mapping_values_set);
        analyzer.reset();
        // Load scheduling table to analyzer
        analyzer.init(table_);
        // Check_validity
        if(!analyzer.check_validity()) continue;
        analyzer.estimate_cost();
        // Get num_activated_chips 
        num_activated_chips = analyzer.get_num_active_chips();
        // Generate partition_caseoral 
        PartitioningInfo partition_case; 
        // Get cost between DRAM and Global buffers
        partition_case.cost = analyzer.get_target_level_cost(dram_pos, metric);
        // Get input tile size that DRAM transfers
        partition_case.input_tile_size = analyzer.get_tile_size(dram_pos, 
                                                      data_t::INPUT);
        // Get tile-granular access count of input in DRAM 
        partition_case.input_access_count = analyzer.get_access_count(dram_pos, 
                                                            data_t::INPUT);
        partition_case.num_assigned_chips = num_activated_chips;
        partition_case.scheduling_table = table_;
        // Find the num_activated_chips already exist in map
        if(list_of_partition.find(num_activated_chips) != list_of_partition.end()) {
            // The num_activated_chips already exists compare their cost
            if(partition_case.cost < list_of_partition.at(num_activated_chips).cost) { 
                list_of_partition.at(num_activated_chips) = partition_case; 
            }
            else if (partition_case.cost == list_of_partition.at(num_activated_chips).cost) {
                // Compare # DRAM access count
                unsigned partition_case_dram_base_ac = 1;
                unsigned list_of_partition_dram_base_ac = 1;
                std::vector<unsigned> partition_case_dram_row
                        = partition_case.scheduling_table.get_row_values(dram_pos);
                std::vector<unsigned> list_of_partition_dram_row
                        = list_of_partition.at(num_activated_chips).scheduling_table.get_row_values(dram_pos);

                for(unsigned i = 0; i < partition_case_dram_row.size(); i++) {
                    partition_case_dram_base_ac *= partition_case_dram_row.at(i);
                    list_of_partition_dram_base_ac *= list_of_partition_dram_row.at(i);
                }
                if(partition_case_dram_base_ac < list_of_partition_dram_base_ac) {
                    list_of_partition.at(num_activated_chips) = partition_case;
                }
            }
            // Get the case that if tha layer does not parallelize with other layers
            if(partition_case.cost < best_cost) { 
                best_cost = partition_case.cost; best_num_activated_chips = num_activated_chips;
            }
        }
        else {
            // The num_activated_chips does not exist, insert as new element
            list_of_partition.insert(std::make_pair(num_activated_chips, partition_case));
        }
    }
    PartitioningInfo solely_run_case = list_of_partition.at(best_num_activated_chips);
    solely_run_case.is_parallelized = false;
    solely_run_case.scheduling_table = list_of_partition.at(best_num_activated_chips).scheduling_table;
    list_of_partition.insert(std::make_pair(0, solely_run_case));

    std::cerr << "[message] Collecting results\n";
    for(auto it = list_of_partition.begin(); it != list_of_partition.end(); ++it) {
        partiton_comb.push_back(it->second);
    }
    return partiton_comb;
}
