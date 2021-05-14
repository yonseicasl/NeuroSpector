#include "stats.h"

static handler_t handler;
static energy_ref_t energy_ref;
static cycle_ref_t cycle_ref;

/* Stats */
stats_t::stats_t(const accelerator_t *accelerator_, 
                 const mapping_table_t& mapping_table_)
    : l1_dataflow(accelerator_->l1_dataflow()),
      l2_dataflow(accelerator_->l2_dataflow()),
      accelerator(accelerator_), 
      mapping_table(mapping_table_) {

}

stats_t::stats_t(const accelerator_t *accelerator_, 
                 const mapping_table_t& mapping_table_,
                 const dataflow_t l1_dataflow_, 
                 const dataflow_t l2_dataflow_)
    : l1_dataflow(l1_dataflow_),
      l2_dataflow(l2_dataflow_),
      accelerator(accelerator_), 
      mapping_table(mapping_table_) {

}

stats_t::~stats_t() {

}

// stats_t's APIs
double stats_t::get_total_energy() const { return total_energy; }

double stats_t::get_energy(component_t U) const { 
    double rtn = 0;
    switch(U) {
        case component_t::L1: rtn = l1_energy_upper; break;
        case component_t::L2: rtn = l2_energy_upper + l1_energy_lower; break;
        case component_t::DRAM: rtn = dram_energy + l2_energy_lower; break;
        default: handler.print_err(err_type_t::INVAILD, "COMPONENT"); break;
    }
    return rtn; 
}

double stats_t::get_total_cycle() const { return total_cycle; }

double stats_t::get_total_edp() const { return total_edp; }

void stats_t::print_stats() const {
    handler.print_line(50, "*");
    std::cout << " - L1 TILE SIZE (I)  : " << l1_input_tile_size << "\n"
              << " - L1 TILE SIZE (F)  : " << l1_filter_tile_size << "\n"
              << " - L1 TILE SIZE (O)  : " << l1_output_tile_size << std::endl;
    std::cout << " - L2 TILE SIZE (I)  : " << l2_input_tile_size << "\n"
              << " - L2 TILE SIZE (F)  : " << l2_filter_tile_size << "\n"
              << " - L2 TILE SIZE (O)  : " << l2_output_tile_size << std::endl;
    handler.print_line(50, "*");
    std::cout << " - L1   ITERATION (I): " << l1_iteration.input_rd_it << "\n"
              << " - L1   ITERATION (F): " << l1_iteration.filter_rd_it << "\n"
              << " - L1   ITERATION (O): " << l1_iteration.output_rd_it + l1_iteration.output_wt_it << std::endl;
    std::cout << " - L2   ITERATION (I): " << l2_iteration.input_rd_it << "\n"
              << " - L2   ITERATION (F): " << l2_iteration.filter_rd_it << "\n"
              << " - L2   ITERATION (O): " << l2_iteration.output_rd_it + l2_iteration.output_wt_it << std::endl;
    std::cout << " - DRAM ITERATION (I): " << dram_iteration.input_rd_it << "\n"
              << " - DRAM ITERATION (F): " << dram_iteration.filter_rd_it << "\n"
              << " - DRAM ITERATION (O): " << dram_iteration.output_rd_it + dram_iteration.output_wt_it << std::endl;
    handler.print_line(50, "*");
    std::cout << " - # OF S0 HOSTS (I) : " << num_s0_input_hosts << "\n"
              << " - # OF S0 HOSTS (F) : " << num_s0_filter_hosts << "\n"
              << " - # OF S0 HOSTS (O) : " << num_s0_output_hosts << std::endl;
    std::cout << " - # OF S1 HOSTS (I) : " << num_s1_input_hosts << "\n"
              << " - # OF S1 HOSTS (F) : " << num_s1_filter_hosts << "\n"
              << " - # OF S1 HOSTS (O) : " << num_s1_output_hosts << std::endl;
    std::cout << " - # OF S2 HOSTS (I) : " << num_s2_input_hosts << "\n"
              << " - # OF S2 HOSTS (F) : " << num_s2_filter_hosts << "\n"
              << " - # OF S2 HOSTS (O) : " << num_s2_output_hosts << std::endl;
    handler.print_line(50, "*");
    std::cout << " - MAC   ENERGY      : " << mac_energy << " (" << float(mac_energy) / total_energy * 100 << "%)" << "\n"
              << " - L1    ENERGY      : " << l1_energy << " (" << float(l1_energy) / total_energy * 100 << "%)" << "\n"
              << " - L2    ENERGY      : " << l2_energy << " (" << float(l2_energy) / total_energy * 100 << "%)" << "\n"
              << " - DRAM  ENERGY      : " << dram_energy << " (" << float(dram_energy) / total_energy * 100 << "%)" << "\n"
              << " - TOTAL ENERGY      : " << total_energy << std::endl;
    handler.print_line(50, "*");
    std::cout << " - # OF ACTIVE MACs  : " << num_active_macs << "\n"
              << " - # OF ACTIVE  PEs  : " << num_active_pes << "\n"
              << " - # OF ACTIVE ACCs  : " << num_active_accs << std::endl;
    handler.print_line(50, "*");
    std::cout << " -   S0 UTILIZATION  : " << s0_utilization << " %" << "\n" 
              << " -   L1 UTILIZATION  : " << l1_utilization << " %" << "\n" 
              << " -   S1 UTILIZATION  : " << s1_utilization << " %" << "\n"
              << " -   L2 UTILIZATION  : " << l2_utilization << " %" << "\n"
              << " -   S2 UTILIZATION  : " << s2_utilization << " %" << std::endl;
    handler.print_line(50, "*");
    std::cout << " - MAC   CYCLE       : " << mac_cycle << " (" << float(mac_cycle) / total_cycle * 100 << "%)" << "\n"
              << " - L1    CYCLE       : " << l1_cycle << " (" << float(l1_cycle) / total_cycle * 100 << "%)" << "\n"
              << " - L2    CYCLE       : " << l2_cycle << " (" << float(l2_cycle) / total_cycle * 100 << "%)" << "\n"
              << " - DRAM  CYCLE       : " << dram_cycle << " (" << float(dram_cycle) / total_cycle * 100 << "%)" << "\n"
              << " - TOTAL CYCLE       : " << total_cycle << std::endl;
    handler.print_line(50, "*");
    std::cout << " - TOTAL EDP         : " << total_edp << " (J x CYCLE)" << std::endl;
    return;
}

void stats_t::print_csv() const {
    std::cout << "STATS,MAC ENERGY,L1 ENERGY,L2 ENERGY,DRAM ENERGY,TOTAL ENERGY,MAC CYCLE,L1 CYCLE,L2 CYCLE,DRAM CYCLE,TOTAL CYCLE,TOTAL EDP (JxCYCLE),S0 UTILIZATION,L1 UTILIZATION,S1 UTILIZATION,L2 UTILIZATION,S2 UTILIZATION\n" 
              << "," << std::fixed << std::setprecision(1) << mac_energy 
              << "," << std::fixed << std::setprecision(1) << l1_energy 
              << "," << std::fixed << std::setprecision(1) << l2_energy 
              << "," << std::fixed << std::setprecision(1) << dram_energy 
              << "," << std::fixed << std::setprecision(1) << total_energy 
              << "," << mac_cycle 
              << "," << l1_cycle 
              << "," << l2_cycle 
              << "," << dram_cycle 
              << "," << total_cycle 
              << "," << total_edp 
              << "," << s0_utilization 
              << "," << l1_utilization 
              << "," << s1_utilization 
              << "," << l2_utilization
              << "," << s2_utilization << std::endl;
    return;
}

void stats_t::update_stats() { 
    update_tile_size();
    update_iteration();
    update_active_components();
    update_noc();
    update_energy();
    update_utilization();
    update_cycle();
    update_edp();
    return;
}

void stats_t::update_tile_size() {
    // Temporal tile sizes
    mac_input_tile_size = mapping_table.get_input_tile_size(component_t::MAC);     // Size: 1
    mac_filter_tile_size = mapping_table.get_filter_tile_size(component_t::MAC);   // Size: 1
    mac_output_tile_size = mapping_table.get_filter_tile_size(component_t::MAC);   // Size: 1
    l1_input_tile_size = mapping_table.get_input_tile_size(component_t::L1);
    l1_input_tile_size_spatial = mapping_table.get_input_tile_size(component_t::S1_Y);
    l1_filter_tile_size = mapping_table.get_filter_tile_size(component_t::L1);
    l1_filter_tile_size_spatial = mapping_table.get_filter_tile_size(component_t::S1_Y);
    l1_output_tile_size = mapping_table.get_output_tile_size(component_t::L1);
    l1_output_tile_size_spatial = mapping_table.get_output_tile_size(component_t::S1_Y);
    l2_input_tile_size = mapping_table.get_input_tile_size(component_t::L2);
    l2_filter_tile_size = mapping_table.get_filter_tile_size(component_t::L2);
    l2_output_tile_size = mapping_table.get_output_tile_size(component_t::L2);
    // Bypass adjustment
    if(accelerator->l1_input_bypass())
        l1_input_tile_size = mac_input_tile_size; 
    if(accelerator->l1_filter_bypass())
        l1_filter_tile_size = mac_filter_tile_size; 
    if(accelerator->l1_output_bypass()) 
        l1_output_tile_size = mac_output_tile_size; 
    if(accelerator->l2_input_bypass())
        l2_input_tile_size = l1_input_tile_size; 
    if(accelerator->l2_filter_bypass())
        l2_filter_tile_size = l1_filter_tile_size; 
    if(accelerator->l2_output_bypass()) 
        l2_output_tile_size = l1_output_tile_size; 
}

void stats_t::update_iteration() {
    // Iteration without dataflow 
    size_t l1_iteration_tmp = mapping_table.get_iteration(component_t::L1);
    size_t l2_iteration_tmp = mapping_table.get_iteration(component_t::L2);
    size_t dram_iteration_tmp = mapping_table.get_iteration(component_t::DRAM);

    // TODO : Move this variables to header file
    size_t h_upper = 0;
    size_t h_lower = 0;
    size_t w_upper = 0;
    size_t w_lower = 0;
    size_t p_upper = 0;
    size_t q_upper = 0;
    size_t s_upper = 0;
    size_t r_upper = 0;
    int h_overlap_size = 0;
    int w_overlap_size = 0;
    size_t h_iteration = 0;
    size_t w_iteration = 0;

    size_t h_stride= mapping_table.get_stride();
    size_t w_stride= mapping_table.get_stride();
    //
    size_t gamma_dram_iteration = mapping_table.get_degree(parameter_t::C, component_t::DRAM)
                                * mapping_table.get_degree(parameter_t::S, component_t::DRAM)
                                * mapping_table.get_degree(parameter_t::R, component_t::DRAM);
    size_t gamma_l2_iteration = mapping_table.get_degree(parameter_t::C, component_t::L2)
                              * mapping_table.get_degree(parameter_t::S, component_t::L2)
                              * mapping_table.get_degree(parameter_t::R, component_t::L2);
    size_t gamma_l1_iteration = mapping_table.get_degree(parameter_t::C, component_t::L1)
                              * mapping_table.get_degree(parameter_t::S, component_t::L1)
                              * mapping_table.get_degree(parameter_t::R, component_t::L1);
    // L1 iteration with MAC dataflow
    l1_iteration.input_rd_it = l1_iteration_tmp;
    l1_iteration.filter_rd_it = l1_iteration_tmp;
    l1_iteration.output_wt_it = l1_iteration_tmp;
    // For conditional read
    l1_iteration.output_rd_it = (gamma_dram_iteration * gamma_l2_iteration * gamma_l1_iteration - 1)
                              * mapping_table.get_degree(parameter_t::G, component_t::L1)
                              * mapping_table.get_degree(parameter_t::K, component_t::L1)
                              * mapping_table.get_degree(parameter_t::B, component_t::L1)
                              * mapping_table.get_degree(parameter_t::P, component_t::L1)
                              * mapping_table.get_degree(parameter_t::Q, component_t::L1)
                              * mapping_table.get_degree(parameter_t::G, component_t::L2)
                              * mapping_table.get_degree(parameter_t::K, component_t::L2)
                              * mapping_table.get_degree(parameter_t::B, component_t::L2)
                              * mapping_table.get_degree(parameter_t::P, component_t::L2)
                              * mapping_table.get_degree(parameter_t::Q, component_t::L2)
                              * mapping_table.get_degree(parameter_t::G, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::K, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::B, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::P, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::Q, component_t::DRAM);
    switch(accelerator->mac_dataflow()) {
        case dataflow_t::IS: 
            /* INPUT STATIONARY FOR MAC DATAFLOW */
            h_upper = (mapping_table.get_temporal_product(parameter_t::P, component_t::S0) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::S, component_t::S0);
            w_upper = (mapping_table.get_temporal_product(parameter_t::Q, component_t::S0) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::R, component_t::S0);
            p_upper =  mapping_table.get_temporal_product(parameter_t::P, component_t::S0);
            q_upper =  mapping_table.get_temporal_product(parameter_t::Q, component_t::S0);
            s_upper =  mapping_table.get_temporal_product(parameter_t::S, component_t::S0);
            r_upper =  mapping_table.get_temporal_product(parameter_t::R, component_t::S0); 

            h_lower = (mapping_table.get_temporal_product(parameter_t::P, component_t::L1) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::S, component_t::L1);
            w_lower = (mapping_table.get_temporal_product(parameter_t::Q, component_t::L1) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::R, component_t::L1);

            // Suppose height/width filter size & height/width stride are same
            h_overlap_size = int(mapping_table.get_temporal_product(parameter_t::S, component_t::L1)) - int(mapping_table.get_stride());
            w_overlap_size = int(mapping_table.get_temporal_product(parameter_t::R, component_t::L1)) - int(mapping_table.get_stride());

            h_stride = mapping_table.get_temporal_product(parameter_t::S, component_t::S0);
            w_stride = mapping_table.get_temporal_product(parameter_t::R, component_t::S0); 

            // Step 1. 
            if(int(h_upper) < h_overlap_size && ((p_upper * mapping_table.get_stride()) % s_upper) == 0) {
              h_iteration = (h_lower- h_upper)/h_stride + 1;
              h_iteration *= mapping_table.get_iteration(parameter_t::P, component_t::L2);
              h_iteration *= mapping_table.get_iteration(parameter_t::S, component_t::L2);

            }
            // Step 2. 
            else {
              h_iteration = mapping_table.get_iteration(parameter_t::P, component_t::L1) 
                          * mapping_table.get_iteration(parameter_t::S, component_t::L1);
            }

            // Step 1. 
            if(int(w_upper) < w_overlap_size && ((q_upper * mapping_table.get_stride()) % r_upper) == 0) {
              w_iteration = (w_lower- w_upper)/w_stride + 1;
              w_iteration *= mapping_table.get_iteration(parameter_t::Q, component_t::L2);
              w_iteration *= mapping_table.get_iteration(parameter_t::R, component_t::L2);
            }
            // Step 2. 
            else {
               w_iteration = mapping_table.get_iteration(parameter_t::Q, component_t::L1) 
                          * mapping_table.get_iteration(parameter_t::R, component_t::L1);             
            }
            l1_iteration.input_rd_it = mapping_table.get_iteration(parameter_t::G, component_t::L1)
                                     * mapping_table.get_iteration(parameter_t::B, component_t::L1)
                                     * mapping_table.get_iteration(parameter_t::C, component_t::L1)
                                     * h_iteration * w_iteration
                                     * mapping_table.get_iteration(parameter_t::K, component_t::L1); 
            l1_iteration.input_rd_it /= (mapping_table.get_degree(parameter_t::K, component_t::L1));


            break;
        case dataflow_t::WS: 
            l1_iteration.filter_rd_it /= (mapping_table.get_degree(parameter_t::B, component_t::L1) 
                                          * mapping_table.get_degree(parameter_t::P, component_t::L1) 
                                          * mapping_table.get_degree(parameter_t::Q, component_t::L1));
            break;

        case dataflow_t::NONE: // Nothing to do
            break;
        default: handler.print_err(err_type_t::INVAILD, "MAC DATAFLOW"); break;
    }
    // L2 iteration with L1 dataflow 
    l2_iteration.input_rd_it = l2_iteration_tmp;
    l2_iteration.filter_rd_it = l2_iteration_tmp;
    l2_iteration.output_wt_it = l2_iteration_tmp;
    // For conditional read
    l2_iteration.output_rd_it = (gamma_dram_iteration * gamma_l2_iteration - 1)
                              * mapping_table.get_degree(parameter_t::G, component_t::L2)
                              * mapping_table.get_degree(parameter_t::K, component_t::L2)
                              * mapping_table.get_degree(parameter_t::B, component_t::L2)
                              * mapping_table.get_degree(parameter_t::P, component_t::L2)
                              * mapping_table.get_degree(parameter_t::Q, component_t::L2)
                              * mapping_table.get_degree(parameter_t::G, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::K, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::B, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::P, component_t::DRAM)
                              * mapping_table.get_degree(parameter_t::Q, component_t::DRAM);
    switch(l1_dataflow) {
        case dataflow_t::IS: 
            /* INFO: IS
            **Newly Begin the input stationary**
            ::Step-by-Step Explaination::
            Step 0. Prepare all variables which are needed to compute #iteration
                    * XX_upper, XX_lower means upper/lower level XX tile size each.
                    h_upper, w_upper
                    p_upper, q_upper
                    s_upper, r_upper
                    h_overlap_size, w_overlap_size
                    h_stride, w_stride

            Step 1. If there exists some of input tile reuse -> compute with using filter striding
            Step 2. Else, there's no unique input tile reuse -> compute #iteration as done in WS OS
            */
            // Step 0.
            // TODO: Substitute get_temporal_product function. This function will be inappropriate if there exists multiple MAC (S0 Spatial)
            h_upper = (mapping_table.get_temporal_product(parameter_t::P, component_t::L1) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::S, component_t::L1);
            w_upper = (mapping_table.get_temporal_product(parameter_t::Q, component_t::L1) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::R, component_t::L1);
            p_upper =  mapping_table.get_temporal_product(parameter_t::P, component_t::L1);
            q_upper =  mapping_table.get_temporal_product(parameter_t::Q, component_t::L1);
            s_upper =  mapping_table.get_temporal_product(parameter_t::S, component_t::L1);
            r_upper =  mapping_table.get_temporal_product(parameter_t::R, component_t::L1); 

            h_lower = (mapping_table.get_temporal_product(parameter_t::P, component_t::L2) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::S, component_t::L2);
            w_lower = (mapping_table.get_temporal_product(parameter_t::Q, component_t::L2) - 1) * mapping_table.get_stride() + mapping_table.get_temporal_product(parameter_t::R, component_t::L2);

            // Suppose height/width filter size & height/width stride are same
            h_overlap_size = int(mapping_table.get_temporal_product(parameter_t::S, component_t::L2)) - int(mapping_table.get_stride());
            w_overlap_size = int(mapping_table.get_temporal_product(parameter_t::R, component_t::L2)) - int(mapping_table.get_stride());

            h_stride = mapping_table.get_temporal_product(parameter_t::S, component_t::L1);
            w_stride = mapping_table.get_temporal_product(parameter_t::R, component_t::L1); 

            // Step 1. 
            if(int(h_upper) < h_overlap_size && ((p_upper * mapping_table.get_stride()) % s_upper) == 0) {
              h_iteration = (h_lower- h_upper)/h_stride + 1;
              h_iteration *= mapping_table.get_iteration(parameter_t::P, component_t::DRAM);
              h_iteration *= mapping_table.get_iteration(parameter_t::S, component_t::DRAM);

            }
            // Step 2. 
            else {
              h_iteration = mapping_table.get_iteration(parameter_t::P, component_t::L2) 
                          * mapping_table.get_iteration(parameter_t::S, component_t::L2);
            }

            // Step 1. 
            if(int(w_upper) < w_overlap_size && ((q_upper * mapping_table.get_stride()) % r_upper) == 0) {
              w_iteration = (w_lower- w_upper)/w_stride + 1;
              w_iteration *= mapping_table.get_iteration(parameter_t::Q, component_t::DRAM);
              w_iteration *= mapping_table.get_iteration(parameter_t::R, component_t::DRAM);
            }
            // Step 2. 
            else {
               w_iteration = mapping_table.get_iteration(parameter_t::Q, component_t::L2) 
                          * mapping_table.get_iteration(parameter_t::R, component_t::L2);             
            }
            l2_iteration.input_rd_it = mapping_table.get_iteration(parameter_t::G, component_t::L2)
                                     * mapping_table.get_iteration(parameter_t::B, component_t::L2)
                                     * mapping_table.get_iteration(parameter_t::C, component_t::L2)
                                     * h_iteration * w_iteration
                                     * mapping_table.get_iteration(parameter_t::K, component_t::DRAM);


            break;
        case dataflow_t::WS: 
            l2_iteration.filter_rd_it /= (mapping_table.get_degree(parameter_t::B, component_t::L2) 
                                        * mapping_table.get_degree(parameter_t::P, component_t::L2) 
                                        * mapping_table.get_degree(parameter_t::Q, component_t::L2));
            break;
        case dataflow_t::OS: 
            if(gamma_dram_iteration > 1) {
                l2_iteration.output_rd_it = (gamma_dram_iteration - 1)
                                          * mapping_table.get_degree(parameter_t::G, component_t::L2)
                                          * mapping_table.get_degree(parameter_t::K, component_t::L2)
                                          * mapping_table.get_degree(parameter_t::B, component_t::L2)
                                          * mapping_table.get_degree(parameter_t::P, component_t::L2)
                                          * mapping_table.get_degree(parameter_t::Q, component_t::L2)
                                          * mapping_table.get_degree(parameter_t::G, component_t::DRAM)
                                          * mapping_table.get_degree(parameter_t::K, component_t::DRAM)
                                          * mapping_table.get_degree(parameter_t::B, component_t::DRAM)
                                          * mapping_table.get_degree(parameter_t::P, component_t::DRAM)
                                          * mapping_table.get_degree(parameter_t::Q, component_t::DRAM);
            }
            else 
                l2_iteration.output_rd_it = 0;
            l2_iteration.output_wt_it /= (mapping_table.get_degree(parameter_t::C, component_t::L2) 
                                        * mapping_table.get_degree(parameter_t::S, component_t::L2) 
                                        * mapping_table.get_degree(parameter_t::R, component_t::L2));
            break;
        default: handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW"); break;
    }
    // DRAM iteration with L2 dataflow 
    dram_iteration.input_rd_it = dram_iteration_tmp;
    dram_iteration.filter_rd_it = dram_iteration_tmp;
    dram_iteration.output_wt_it = dram_iteration_tmp;
    // For conditional read
    dram_iteration.output_rd_it = (gamma_dram_iteration - 1) 
                                * mapping_table.get_degree(parameter_t::G, component_t::DRAM)
                                * mapping_table.get_degree(parameter_t::K, component_t::DRAM)
                                * mapping_table.get_degree(parameter_t::B, component_t::DRAM)
                                * mapping_table.get_degree(parameter_t::P, component_t::DRAM)
                                * mapping_table.get_degree(parameter_t::Q, component_t::DRAM);
    switch(l2_dataflow) {
        case dataflow_t::IS: 
            /* Skip the explanation about code as explain in above code (l1 dataflow) */
            h_upper = (mapping_table.get_product(parameter_t::P, component_t::L2) - 1) * mapping_table.get_stride() + mapping_table.get_product(parameter_t::S, component_t::L2);
            w_upper = (mapping_table.get_product(parameter_t::Q, component_t::L2) - 1) * mapping_table.get_stride() + mapping_table.get_product(parameter_t::R, component_t::L2);
            p_upper =  mapping_table.get_product(parameter_t::P, component_t::L2);
            q_upper =  mapping_table.get_product(parameter_t::Q, component_t::L2);
            s_upper =  mapping_table.get_product(parameter_t::S, component_t::L2);
            r_upper =  mapping_table.get_product(parameter_t::R, component_t::L2); 

            h_lower = (mapping_table.get_product(parameter_t::P, component_t::DRAM) - 1) * mapping_table.get_stride() + mapping_table.get_product(parameter_t::S, component_t::DRAM);
            w_lower = (mapping_table.get_product(parameter_t::Q, component_t::DRAM) - 1) * mapping_table.get_stride() + mapping_table.get_product(parameter_t::R, component_t::DRAM);

            // Suppose height/width filter size & height/width stride are same
            h_overlap_size = int(mapping_table.get_product(parameter_t::S, component_t::DRAM)) - int(mapping_table.get_stride());
            w_overlap_size = int(mapping_table.get_product(parameter_t::R, component_t::DRAM)) - int(mapping_table.get_stride());

            h_stride = mapping_table.get_product(parameter_t::S, component_t::L2);
            w_stride = mapping_table.get_product(parameter_t::R, component_t::L2); 

            // Step 1. 
            if(int(h_upper) < h_overlap_size && ((p_upper * mapping_table.get_stride()) % s_upper) == 0) {
              h_iteration = (h_lower - h_upper)/h_stride + 1;
            }
            // Step 2. 
            else {
              h_iteration = mapping_table.get_iteration(parameter_t::P, component_t::DRAM) 
                          * mapping_table.get_iteration(parameter_t::S, component_t::DRAM);
            }

            // Step 1. 
            if(int(w_upper) < w_overlap_size && ((q_upper * mapping_table.get_stride()) % r_upper) == 0) {
              w_iteration = (w_lower- w_upper)/w_stride + 1;
            }
            // Step 2. 
            else {
               w_iteration = mapping_table.get_iteration(parameter_t::Q, component_t::DRAM) 
                          * mapping_table.get_iteration(parameter_t::R, component_t::DRAM);             
            }
            dram_iteration.input_rd_it = mapping_table.get_iteration(parameter_t::G, component_t::DRAM)
                                     * mapping_table.get_iteration(parameter_t::B, component_t::DRAM)
                                     * mapping_table.get_iteration(parameter_t::C, component_t::DRAM)
                                     * h_iteration * w_iteration;
            break;
        case dataflow_t::WS: 
            dram_iteration.filter_rd_it /= (mapping_table.get_degree(parameter_t::B, component_t::DRAM) 
                                          * mapping_table.get_degree(parameter_t::P, component_t::DRAM) 
                                          * mapping_table.get_degree(parameter_t::Q, component_t::DRAM));
            break;
        case dataflow_t::OS: 
            dram_iteration.output_rd_it = 0;
            dram_iteration.output_wt_it /= (mapping_table.get_degree(parameter_t::C, component_t::DRAM) 
                                          * mapping_table.get_degree(parameter_t::S, component_t::DRAM) 
                                          * mapping_table.get_degree(parameter_t::R, component_t::DRAM));
            break;
        default: handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW"); break;
    }
    // Corner case adjustment
    /*
    1. Get input filter output tile size in all level
    2. if(l1 tile size_spatial == l2 tile size)
       -> l2 iteration = dram iteration
    */
    if(l1_input_tile_size_spatial == l2_input_tile_size) {
      l2_iteration.input_rd_it = dram_iteration.input_rd_it; 
    }
    if(l1_filter_tile_size_spatial == l2_filter_tile_size) {
      l2_iteration.filter_rd_it = dram_iteration.filter_rd_it;
    }
    if(l1_output_tile_size_spatial == l2_output_tile_size) {
      l2_iteration.output_wt_it = dram_iteration.output_wt_it;
      l2_iteration.output_rd_it = dram_iteration.output_rd_it;
    }

    // Bypass adjustment
    if(accelerator->l1_input_bypass())
        l2_iteration.input_rd_it = l1_iteration.input_rd_it;
    if(accelerator->l1_filter_bypass())
        l2_iteration.filter_rd_it = l1_iteration.filter_rd_it;
    if(accelerator->l1_output_bypass()) {
        l2_iteration.output_rd_it = l1_iteration.output_rd_it;
        l2_iteration.output_wt_it = l1_iteration.output_wt_it;
    }
    if(accelerator->l2_input_bypass())
        dram_iteration.input_rd_it = l2_iteration.input_rd_it;
    if(accelerator->l2_filter_bypass())
        dram_iteration.filter_rd_it = l2_iteration.filter_rd_it;
    if(accelerator->l2_output_bypass()) {
        dram_iteration.output_rd_it = l2_iteration.output_rd_it;
        dram_iteration.output_wt_it = l2_iteration.output_wt_it;
    }
    return;
}

void stats_t::update_active_components() {
    num_active_macs = mapping_table.get_degree(parameter_t::G, component_t::S0)
                    * mapping_table.get_degree(parameter_t::K, component_t::S0)
                    * mapping_table.get_degree(parameter_t::B, component_t::S0)
                    * mapping_table.get_degree(parameter_t::P, component_t::S0)
                    * mapping_table.get_degree(parameter_t::Q, component_t::S0)
                    * mapping_table.get_degree(parameter_t::C, component_t::S0)
                    * mapping_table.get_degree(parameter_t::S, component_t::S0)
                    * mapping_table.get_degree(parameter_t::R, component_t::S0);
    num_active_pes = mapping_table.get_degree(parameter_t::G, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::K, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::B, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::P, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::Q, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::C, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::S, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::R, component_t::S1_X)
                   * mapping_table.get_degree(parameter_t::G, component_t::S1_Y)
                   * mapping_table.get_degree(parameter_t::K, component_t::S1_Y)
                   * mapping_table.get_degree(parameter_t::B, component_t::S1_Y)
                   * mapping_table.get_degree(parameter_t::P, component_t::S1_Y)
                   * mapping_table.get_degree(parameter_t::Q, component_t::S1_Y)
                   * mapping_table.get_degree(parameter_t::C, component_t::S1_Y)
                   * mapping_table.get_degree(parameter_t::S, component_t::S1_Y)
                   * mapping_table.get_degree(parameter_t::R, component_t::S1_Y);
    num_active_accs = mapping_table.get_degree(parameter_t::K, component_t::S2)
                    * mapping_table.get_degree(parameter_t::K, component_t::S2)
                    * mapping_table.get_degree(parameter_t::B, component_t::S2)
                    * mapping_table.get_degree(parameter_t::P, component_t::S2)
                    * mapping_table.get_degree(parameter_t::Q, component_t::S2)
                    * mapping_table.get_degree(parameter_t::C, component_t::S2)
                    * mapping_table.get_degree(parameter_t::S, component_t::S2)
                    * mapping_table.get_degree(parameter_t::R, component_t::S2);
}

void stats_t::update_noc() {
    // S0 input
    num_s0_input_hosts = mapping_table.get_degree(parameter_t::G, component_t::S0)
                       * mapping_table.get_degree(parameter_t::B, component_t::S0)
                       * mapping_table.get_degree(parameter_t::C, component_t::S0);
    size_t P_s0 = mapping_table.get_degree(parameter_t::P, component_t::S0);
    size_t S_s0 = mapping_table.get_degree(parameter_t::S, component_t::S0);
    size_t Q_s0 = mapping_table.get_degree(parameter_t::Q, component_t::S0);
    size_t R_s0 = mapping_table.get_degree(parameter_t::R, component_t::S0);
    if(P_s0 > 1 && S_s0 > 1) 
        num_s0_input_hosts *= ((P_s0 - 1) * mapping_table.get_stride() + S_s0);
    else 
        num_s0_input_hosts *= (P_s0 * S_s0);
    if(Q_s0 > 1 && R_s0 > 1) 
        num_s0_input_hosts *= ((Q_s0 - 1) * mapping_table.get_stride() + R_s0);
    else 
        num_s0_input_hosts *= (Q_s0 * R_s0);
    // S0 filter & output
    num_s0_filter_hosts = mapping_table.get_degree(parameter_t::G, component_t::S0)
                        * mapping_table.get_degree(parameter_t::K, component_t::S0)
                        * mapping_table.get_degree(parameter_t::C, component_t::S0)
                        * mapping_table.get_degree(parameter_t::S, component_t::S0)
                        * mapping_table.get_degree(parameter_t::R, component_t::S0);
    num_s0_output_hosts = mapping_table.get_degree(parameter_t::G, component_t::S0)
                        * mapping_table.get_degree(parameter_t::K, component_t::S0)
                        * mapping_table.get_degree(parameter_t::B, component_t::S0)
                        * mapping_table.get_degree(parameter_t::P, component_t::S0)
                        * mapping_table.get_degree(parameter_t::Q, component_t::S0);
    // S1_X & S1_Y input
    num_s1_input_hosts = mapping_table.get_degree(parameter_t::G, component_t::S1_X)
                       * mapping_table.get_degree(parameter_t::B, component_t::S1_X)
                       * mapping_table.get_degree(parameter_t::C, component_t::S1_X)
                       * mapping_table.get_degree(parameter_t::G, component_t::S1_Y)
                       * mapping_table.get_degree(parameter_t::B, component_t::S1_Y)
                       * mapping_table.get_degree(parameter_t::C, component_t::S1_Y);
    size_t P_s1 = mapping_table.get_degree(parameter_t::P, component_t::S1_X)
                * mapping_table.get_degree(parameter_t::P, component_t::S1_Y);
    size_t S_s1 = mapping_table.get_degree(parameter_t::S, component_t::S1_X)
                * mapping_table.get_degree(parameter_t::S, component_t::S1_Y);
    size_t Q_s1 = mapping_table.get_degree(parameter_t::Q, component_t::S1_X)
                * mapping_table.get_degree(parameter_t::Q, component_t::S1_Y);
    size_t R_s1 = mapping_table.get_degree(parameter_t::R, component_t::S1_X)
                * mapping_table.get_degree(parameter_t::R, component_t::S1_Y);
    if(P_s1 > 1 && S_s1 > 1) 
        num_s1_input_hosts *= ((P_s1 - 1) * mapping_table.get_stride() + S_s1);
    else 
        num_s1_input_hosts *= (P_s1 * S_s1);
    if(Q_s1 > 1 && R_s1 > 1) 
        num_s1_input_hosts *= ((Q_s1 - 1) * mapping_table.get_stride() + R_s1);
    else 
        num_s1_input_hosts *= (Q_s1 * R_s1);
    // S1_X & S1_Y filter & output
    num_s1_filter_hosts = mapping_table.get_degree(parameter_t::G, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::K, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::C, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::S, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::R, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::G, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::K, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::C, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::S, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::R, component_t::S1_Y);
    num_s1_output_hosts = mapping_table.get_degree(parameter_t::G, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::K, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::B, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::P, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::Q, component_t::S1_X)
                        * mapping_table.get_degree(parameter_t::G, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::K, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::B, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::P, component_t::S1_Y)
                        * mapping_table.get_degree(parameter_t::Q, component_t::S1_Y);
    // S2 input
    num_s2_input_hosts = mapping_table.get_degree(parameter_t::G, component_t::S2)
                       * mapping_table.get_degree(parameter_t::B, component_t::S2)
                       * mapping_table.get_degree(parameter_t::C, component_t::S2);
    size_t P_s2 = mapping_table.get_degree(parameter_t::P, component_t::S2);
    size_t S_s2 = mapping_table.get_degree(parameter_t::S, component_t::S2);
    size_t Q_s2 = mapping_table.get_degree(parameter_t::Q, component_t::S2);
    size_t R_s2 = mapping_table.get_degree(parameter_t::R, component_t::S2);
    if(P_s2 > 1 && S_s2 > 1) 
        num_s2_input_hosts *= ((P_s2 - 1) * mapping_table.get_stride() + S_s2);
    else 
        num_s2_input_hosts *= (P_s2 * S_s2);
    if(Q_s2 > 1 && R_s2 > 1) 
        num_s2_input_hosts *= ((Q_s2 - 1) * mapping_table.get_stride() + R_s2);
    else 
        num_s2_input_hosts *= (Q_s2 * R_s2);
    // S2 filter
    num_s2_filter_hosts = mapping_table.get_degree(parameter_t::G, component_t::S2)
                        * mapping_table.get_degree(parameter_t::K, component_t::S2)
                        * mapping_table.get_degree(parameter_t::C, component_t::S2)
                        * mapping_table.get_degree(parameter_t::S, component_t::S2)
                        * mapping_table.get_degree(parameter_t::R, component_t::S2);
    num_s2_output_hosts = mapping_table.get_degree(parameter_t::G, component_t::S2)
                        * mapping_table.get_degree(parameter_t::K, component_t::S2)
                        * mapping_table.get_degree(parameter_t::B, component_t::S2)
                        * mapping_table.get_degree(parameter_t::P, component_t::S2)
                        * mapping_table.get_degree(parameter_t::Q, component_t::S2);
}

void stats_t::update_energy() {
    // Bypass Adjustment
    if(accelerator->l1_input_bypass())
        l2_iteration.input_rd_it = l1_iteration.input_rd_it;
    if(accelerator->l1_filter_bypass())
        l2_iteration.filter_rd_it = l1_iteration.filter_rd_it;
    if(accelerator->l1_output_bypass()) {
        l2_iteration.output_rd_it = l1_iteration.output_rd_it;
        l2_iteration.output_wt_it = l1_iteration.output_wt_it;
    }
    if(accelerator->l2_input_bypass()) {
        dram_iteration.input_rd_it = l2_iteration.input_rd_it * num_s1_input_hosts;
        l2_iteration.input_rd_it = 0;
    }
    if(accelerator->l2_filter_bypass()) {
        dram_iteration.filter_rd_it = l2_iteration.filter_rd_it * num_s1_filter_hosts;
        l2_iteration.filter_rd_it = 0;
    }
    if(accelerator->l2_output_bypass()) {
        dram_iteration.output_rd_it = l2_iteration.output_rd_it * num_s1_output_hosts;
        dram_iteration.output_wt_it = l2_iteration.output_wt_it * num_s1_output_hosts;
        l2_iteration.output_rd_it = 0;
        l2_iteration.output_wt_it = 0;
    }
    // Between MAC and L1 with 'l1 iteration' and S0 NoC
    mac_energy = mapping_table.get_num_macs() * energy_ref.mac_operation;
    l1_energy_upper = mac_input_tile_size * l1_iteration.input_rd_it * num_s0_input_hosts * energy_ref.l1_input_egress
              + mac_filter_tile_size * l1_iteration.filter_rd_it * num_s0_filter_hosts * energy_ref.l1_filter_egress
              + mac_output_tile_size * l1_iteration.output_rd_it * num_s0_output_hosts * energy_ref.l1_output_egress
              + mac_output_tile_size * l1_iteration.output_wt_it * num_s0_output_hosts * energy_ref.l1_output_ingress;
    // Between L1 and L2 with 'l2_iteration' and S1 NoC
    l1_energy_lower = l1_input_tile_size * l2_iteration.input_rd_it * energy_ref.l1_input_ingress 
              + l1_filter_tile_size * l2_iteration.filter_rd_it * energy_ref.l1_filter_ingress
              + l1_output_tile_size * l2_iteration.output_rd_it * energy_ref.l1_output_ingress
              + l1_output_tile_size * l2_iteration.output_wt_it * energy_ref.l1_output_egress;
    l1_energy_upper *= num_active_pes * num_active_accs;
    l1_energy_lower *= num_active_pes * num_active_accs;
    l1_energy = l1_energy_upper + l1_energy_lower;
    l2_energy_upper = l1_input_tile_size_spatial * l2_iteration.input_rd_it * energy_ref.l2_input_egress 
              + l1_filter_tile_size * l2_iteration.filter_rd_it * num_s1_filter_hosts * energy_ref.l2_filter_egress
              + l1_output_tile_size * l2_iteration.output_rd_it * num_s1_output_hosts * energy_ref.l2_output_egress
              + l1_output_tile_size * l2_iteration.output_wt_it * num_s1_output_hosts * energy_ref.l2_output_ingress; 
    // Between L2 and DRAM with 'dram_iteration' and S2 NoC
    l2_energy_lower = l2_input_tile_size * dram_iteration.input_rd_it * energy_ref.l2_input_ingress
               + l2_filter_tile_size * dram_iteration.filter_rd_it * energy_ref.l2_filter_ingress
               + l2_output_tile_size * dram_iteration.output_rd_it * energy_ref.l2_output_ingress
               + l2_output_tile_size * dram_iteration.output_wt_it * energy_ref.l2_output_egress;
    l2_energy_upper *= num_active_accs;
    l2_energy_lower *= num_active_accs;
    l2_energy = l2_energy_upper + l2_energy_lower;
    dram_energy = l2_input_tile_size * dram_iteration.input_rd_it * energy_ref.dram_egress
                + l2_filter_tile_size * dram_iteration.filter_rd_it * energy_ref.dram_egress
                + l2_output_tile_size * dram_iteration.output_rd_it * energy_ref.dram_egress
                + l2_output_tile_size * dram_iteration.output_wt_it * energy_ref.dram_ingress;
    total_energy = mac_energy + l1_energy + l2_energy + dram_energy;
    return;
}

void stats_t::update_utilization() {
    s0_utilization = float(num_active_macs) / (accelerator->macs_per_pe() * accelerator->mac_width()) * 100;
    l1_utilization = float(l1_input_tile_size + l1_filter_tile_size + l1_output_tile_size) 
                   / (accelerator->l1_input_size() + accelerator->l1_filter_size() + accelerator->l1_output_size() + accelerator->l1_shared_size()) * 100;
    s1_utilization = float(num_active_pes) / (accelerator->s1_size_x() * accelerator->s1_size_y()) * 100;
    l2_utilization = float(l2_input_tile_size + l2_filter_tile_size + l2_output_tile_size) 
                   / (accelerator->l2_input_size() + accelerator->l2_filter_size() + accelerator->l2_output_size() + accelerator->l2_shared_size()) * 100;
    s2_utilization = float(num_active_accs) / accelerator->s2_size() * 100;
    return;
}

void stats_t::update_cycle() {
    mac_cycle = mapping_table.get_iteration(component_t::L1) * cycle_ref.mac_operation;
    l1_cycle = (l1_iteration.input_rd_it + l1_iteration.filter_rd_it + l1_iteration.output_rd_it + l1_iteration.output_wt_it) * cycle_ref.l1_access;
    l2_cycle = (l2_iteration.input_rd_it + l2_iteration.filter_rd_it + l2_iteration.output_rd_it + l2_iteration.output_wt_it) * cycle_ref.l2_access;
    dram_cycle = (dram_iteration.input_rd_it + dram_iteration.filter_rd_it + dram_iteration.output_rd_it + dram_iteration.output_wt_it) * cycle_ref.dram_access;
    // L1 separated buffer adjustment
    if(accelerator->l1_type() == buffer_type_t::SEPARATED) {
        if(l1_iteration.input_rd_it >= l1_iteration.filter_rd_it)
            l1_cycle -= l1_iteration.filter_rd_it * cycle_ref.l1_access;
        else
            l1_cycle -= l1_iteration.input_rd_it * cycle_ref.l1_access;
    }
    // TODO: support more buffer types 
    
    // Bypass adjustment
    if(accelerator->l1_input_bypass())
        l2_cycle -= l2_iteration.input_rd_it * cycle_ref.l2_access;
    if(accelerator->l1_filter_bypass())
        l2_cycle -= l2_iteration.filter_rd_it * cycle_ref.l2_access;
    if(accelerator->l1_output_bypass()) 
        l2_cycle -= (l2_iteration.output_rd_it + l2_iteration.output_wt_it) * cycle_ref.l2_access;
    if(accelerator->l2_input_bypass())
        dram_cycle -= dram_iteration.input_rd_it * cycle_ref.dram_access;
    if(accelerator->l2_filter_bypass())
        dram_cycle -= dram_iteration.filter_rd_it * cycle_ref.dram_access;
    if(accelerator->l2_output_bypass()) 
        dram_cycle -= (dram_iteration.output_rd_it + dram_iteration.output_wt_it) * cycle_ref.dram_access;
    total_cycle = mac_cycle + l1_cycle + l2_cycle + dram_cycle;
    return;
}

void stats_t::update_edp() {
    total_edp = total_energy / 1000000000000; // pJ -> J
    total_edp *= total_cycle;
}

/* Energy reference */
energy_ref_t::energy_ref_t() {

}

energy_ref_t::~energy_ref_t() {

}

/* Cycle reference */
cycle_ref_t::cycle_ref_t() {

}

cycle_ref_t::~cycle_ref_t() {

}
