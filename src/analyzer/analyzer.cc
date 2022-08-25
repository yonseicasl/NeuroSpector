#include <cassert>
#include <climits>
#include <cmath>
#include <iomanip>

#include "analyzer.h"
#define BYTE 8

analyzer_t::analyzer_t(const std::string& accelerator_pth_,
                       const std::string& network_pth_,
                       const std::string& scheduling_table_pth_) 
    : accelerator(new accelerator_t(accelerator_pth_)),
      network(new network_t(network_pth_)) {
        accelerator->init_accelerator();
        accelerator->print_spec();
        network->init_network();
        scheduling_table = scheduling_table_t(accelerator, 
                                              network,
                                              scheduling_table_pth_);
        init_mac_array();
        init_local_buffer();
        init_pe_array();
        init_global_buffer();
        init_multi_chips();
        init_dram();

        dram_idx = scheduling_table.get_num_rows() - 1;
        gb_idx = scheduling_table.get_above_buffer_pos(dram_idx);
        lb_idx = scheduling_table.get_above_buffer_pos(gb_idx);
}
analyzer_t::analyzer_t(accelerator_t *accelerator_,
                       network_t    *network_)
    :accelerator(accelerator_),
     network(network_) {
        init_mac_array();
        init_local_buffer();
        init_pe_array();
        init_global_buffer();
        init_multi_chips();
        init_dram();
}
analyzer_t::~analyzer_t() { }
// Run analyzer 
void analyzer_t::run() {
    update_tile_size();
    update_active_components();
    // Check scheduling table's validity
    if(!check_validity()) {
        std::cerr << "Invalid scheduling table" << std::endl;
        exit(0);
    }
    update_access_count();
    // Estimate accelerator cost
    estimate_cost();
    // Print out analysis results
    print_results();
}
// Initialize analyzer to evaluate a given scheduling table
void analyzer_t::init(scheduling_table_t scheduling_table_) {
    // Copy scheduling table
    scheduling_table = scheduling_table_;
    dram_idx = scheduling_table.get_num_rows() - 1;
    gb_idx = scheduling_table.get_above_buffer_pos(dram_idx);
    lb_idx = scheduling_table.get_above_buffer_pos(gb_idx);
    update_tile_size();
    update_access_count();
    update_active_components();
    return;
}
// Check scheduling table's validity
bool analyzer_t::check_validity() {
    bool rtn = true;
    // Traverse all rows and check hardware constraints. 
    rtn *= check_hardware_constraints();
    // Traverse all columns and check network constraints. 
    rtn *= check_network_constraints();
    return rtn;
}
// Check hardware constraints
bool analyzer_t::check_hardware_constraints() {
    bool rtn = true;
    unsigned shared_tile_size = 0;
    if(scheduling_table.get_above_buffer_pos(lb_idx) != lb_idx) {
        if(scheduling_table.get_row_product(0) != 1) { return false; }
    }
    // Check MAC array validity
    if(macs_actived.dim_x > macs_capacity.dim_x 
    || macs_actived.dim_y > macs_capacity.dim_y) {
        rtn *= false;
    }

    // Check Local Buffer validity
    if(is_lb_shared) {
        shared_tile_size  = lb_bypass.input  ?  0 : lb_tile_size_alloc.input ;
        shared_tile_size += lb_bypass.weight ?  0 : lb_tile_size_alloc.weight;
        shared_tile_size += lb_bypass.output ?  0 : lb_tile_size_alloc.output;
        if(shared_tile_size > lb_capacity.shared ) {
            rtn *= false;
        }
    }
    else {
        rtn *= lb_tile_size_alloc.input  < lb_capacity.input;
        rtn *= lb_tile_size_alloc.weight < lb_capacity.weight;
        rtn *= lb_tile_size_alloc.output < lb_capacity.output;
    }
    // Check PE array validity
    if(pes_actived.dim_x > pes_capacity.dim_x 
    || pes_actived.dim_y > pes_capacity.dim_y) {
        rtn *= false;
    }
    // Check Global Buffer validity
    if(is_gb_shared) {
        shared_tile_size  = gb_bypass.input  ? 0 : gb_tile_size_alloc.input;
        shared_tile_size += gb_bypass.weight ? 0 : gb_tile_size_alloc.weight;
        shared_tile_size += gb_bypass.output ? 0 : gb_tile_size_alloc.output;
        if(shared_tile_size > gb_capacity.shared ) {
            rtn *= false;
        }
    }
    else {
        rtn *= gb_tile_size_alloc.input  < gb_capacity.input;
        rtn *= gb_tile_size_alloc.weight < gb_capacity.weight;
        rtn *= gb_tile_size_alloc.output < gb_capacity.output;
    }
    // Check Multi chips validity
    if(chips_actived.dim_x > chips_capacity.dim_x 
    || chips_actived.dim_y > chips_capacity.dim_y) {
        rtn *= false;
    }
    return rtn;
}
// Check parameter constraints
bool analyzer_t::check_network_constraints() {
    bool rtn = true;
    return rtn;
}
// Update tile size 
void analyzer_t::update_tile_size() {
    // Update   LB tile size
    lb_tile_size_send.input     =  update_input_tile_size(buffer_t::LB, direction_t::UPPER);
    lb_tile_size_send.weight    = update_weight_tile_size(buffer_t::LB, direction_t::UPPER);
    lb_tile_size_send.output    = update_output_tile_size(buffer_t::LB, direction_t::UPPER);
    lb_tile_size_alloc.input    =  update_input_tile_size(buffer_t::LB, direction_t::LOWER);
    lb_tile_size_alloc.weight   = update_weight_tile_size(buffer_t::LB, direction_t::LOWER);
    lb_tile_size_alloc.output   = update_output_tile_size(buffer_t::LB, direction_t::LOWER);
    
    // Update   GB tile size
    gb_tile_size_send.input     =  update_input_tile_size(buffer_t::GB, direction_t::UPPER);
    gb_tile_size_send.weight    = update_weight_tile_size(buffer_t::GB, direction_t::UPPER);
    gb_tile_size_send.output    = update_output_tile_size(buffer_t::GB, direction_t::UPPER);
    gb_tile_size_alloc.input    =  update_input_tile_size(buffer_t::GB, direction_t::LOWER);
    gb_tile_size_alloc.weight   = update_weight_tile_size(buffer_t::GB, direction_t::LOWER);
    gb_tile_size_alloc.output   = update_output_tile_size(buffer_t::GB, direction_t::LOWER);

    // Update DRAM tile size
    dram_tile_size_send.input   =  update_input_tile_size(buffer_t::DRAM, direction_t::UPPER);
    dram_tile_size_send.weight  = update_weight_tile_size(buffer_t::DRAM, direction_t::UPPER);
    dram_tile_size_send.output  = update_output_tile_size(buffer_t::DRAM, direction_t::UPPER);
    dram_tile_size_alloc.input  =  update_input_tile_size(buffer_t::DRAM, direction_t::LOWER);
    dram_tile_size_alloc.weight = update_weight_tile_size(buffer_t::DRAM, direction_t::LOWER);
    dram_tile_size_alloc.output = update_output_tile_size(buffer_t::DRAM, direction_t::LOWER);

    return;
}
// Update input tile size to allocate (or send)
unsigned analyzer_t::update_input_tile_size(buffer_t buffer_,
                                            direction_t direction_) {
    unsigned tile_size = 1;
    unsigned buffer_idx = 0;
    switch (buffer_) {
    case buffer_t::LB:
        buffer_idx = lb_idx;
        break;
    case buffer_t::GB:
        buffer_idx = gb_idx;
        break;
    case buffer_t::DRAM:
        buffer_idx = dram_idx;
        break;
    default:
        break;
    }
    if(direction_ == direction_t::UPPER) { buffer_idx--; }
    std::vector<unsigned> values = scheduling_table.get_row_wise_product(0, buffer_idx);
    // Compute input height & width
    unsigned input_height = 1;
    unsigned input_width  = 1;
    unsigned layer_idx = scheduling_table.get_layer_index();
    unsigned stride = network->get_stride(layer_idx);
    input_height = (values.at((unsigned)parameter_t::P) - 1) * stride 
                 + values.at((unsigned)parameter_t::R);
    input_width  = (values.at((unsigned)parameter_t::Q) - 1) * stride 
                 + values.at((unsigned)parameter_t::S);
    // Compute input tile size
    tile_size *= values.at((unsigned)parameter_t::B) 
               * values.at((unsigned)parameter_t::C)
               * values.at((unsigned)parameter_t::G)
               * input_height * input_width;
    return tile_size;
}
// Update weight tile size to allocate (or send)
unsigned analyzer_t::update_weight_tile_size(buffer_t buffer_,
                                             direction_t direction_) {
    unsigned tile_size = 1;
    unsigned buffer_idx = 0;
    switch (buffer_) {
    case buffer_t::LB:
        buffer_idx = lb_idx;
        break;
    case buffer_t::GB:
        buffer_idx = gb_idx;
        break;
    case buffer_t::DRAM:
        buffer_idx = dram_idx;
        break;
    default:
        break;
    }
    if(direction_ == direction_t::UPPER) { buffer_idx--; }
    std::vector<unsigned> values = scheduling_table.get_row_wise_product(0, buffer_idx);
    // Compute weight tile size
    tile_size *= values.at((unsigned)parameter_t::K) 
               * values.at((unsigned)parameter_t::C)
               * values.at((unsigned)parameter_t::R) 
               * values.at((unsigned)parameter_t::S)
               * values.at((unsigned)parameter_t::G);
    return tile_size;
}
// Update output tile size to allocate (or send)
unsigned analyzer_t::update_output_tile_size(buffer_t buffer_,
                                             direction_t direction_) {
    unsigned tile_size = 1;
    unsigned buffer_idx = 0;
    switch (buffer_) {
    case buffer_t::LB:
        buffer_idx = lb_idx;
        break;
    case buffer_t::GB:
        buffer_idx = gb_idx;
        break;
    case buffer_t::DRAM:
        buffer_idx = dram_idx;
        break;
    default:
        break;
    }
    if(direction_ == direction_t::UPPER) { buffer_idx--; }
    std::vector<unsigned> values = scheduling_table.get_row_wise_product(0, buffer_idx);
    // Compute output tile size/ab
    tile_size *= values.at((unsigned)parameter_t::K) 
               * values.at((unsigned)parameter_t::B)
               * values.at((unsigned)parameter_t::P) 
               * values.at((unsigned)parameter_t::Q)
               * values.at((unsigned)parameter_t::G);
    return tile_size;
}
// Update tile access count
void analyzer_t::update_access_count() {
    unsigned value = scheduling_table.get_temporal_row_wise_product(lb_idx, dram_idx);
    dataflow = dataflow_t::NONE;
    // Update local buffer access count
    lb_access_count.input_rd  = update_input_access_count(lb_idx, value);
    lb_access_count.weight_rd = update_weight_access_count(lb_idx, value);
    lb_access_count.output_rd = update_output_access_count(lb_idx, value, operation_t::READ);
    lb_access_count.output_wt = update_output_access_count(lb_idx, value, operation_t::WRITE);
    
    // Update global buffer access count
    value = scheduling_table.get_temporal_row_wise_product(gb_idx, dram_idx);
    dataflow = scheduling_table.get_dataflow(lb_idx);
    gb_access_count.input_rd  = update_input_access_count(gb_idx, value);
    gb_access_count.weight_rd = update_weight_access_count(gb_idx, value);
    gb_access_count.output_rd = update_output_access_count(gb_idx, value, operation_t::READ);
    gb_access_count.output_wt = update_output_access_count(gb_idx, value, operation_t::WRITE);

    // Update dram access count
    dataflow = scheduling_table.get_dataflow(gb_idx);
    value = scheduling_table.get_temporal_row_wise_product(dram_idx, dram_idx);
    dram_access_count.input_rd  = update_input_access_count(dram_idx, value); 
    dram_access_count.weight_rd = update_weight_access_count(dram_idx, value);
    dram_access_count.output_rd = update_output_access_count(dram_idx, value, operation_t::READ);
    dram_access_count.output_wt = update_output_access_count(dram_idx, value, operation_t::WRITE);

    // Read once adjustment
    if(dram_tile_size_alloc.input  == dram_tile_size_send.input)  {
        dram_access_count.input_rd  = 1;
    }
    if(dram_tile_size_alloc.weight == dram_tile_size_send.weight) {
        dram_access_count.weight_rd = 1;
    }
    if(dram_tile_size_alloc.output == dram_tile_size_send.output) {
        dram_access_count.output_rd = 0;
        dram_access_count.output_wt = 1;
    }
    if(gb_tile_size_alloc.input  == gb_tile_size_send.input)  {
        gb_access_count.input_rd = dram_access_count.input_rd;
    }
    if(gb_tile_size_alloc.weight == gb_tile_size_send.weight) {
        gb_access_count.weight_rd = dram_access_count.weight_rd;
    }
    if(gb_tile_size_alloc.output == gb_tile_size_send.output) {
        gb_access_count.output_rd = dram_access_count.output_rd;
        gb_access_count.output_wt = dram_access_count.output_wt;
    }
    return;
}
// Update tile-granular input access count
unsigned analyzer_t::update_input_access_count(unsigned idx_,
                                               unsigned value_) {
    unsigned access_count = value_;
    // Dataflow adjustment 
    if(dataflow == dataflow_t::IS) {
        access_count /= update_irrelevant_mapping_value(idx_, data_t::INPUT);
    }
    return access_count;
}
// Update tile-granular weight access count
unsigned analyzer_t::update_weight_access_count(unsigned idx_,
                                                unsigned value_) {
    unsigned access_count = value_;
    // Dataflow adjustment 
    if(dataflow == dataflow_t::WS) {
        access_count /= update_irrelevant_mapping_value(idx_, data_t::WEIGHT);
    }
    // weight_access_count.emplace_back(access_count);
    return access_count;
}
// Update tile-granular output access count
unsigned analyzer_t::update_output_access_count(unsigned idx_,
                                                unsigned value_,
                                                operation_t oper_) {
    unsigned lower_level_idx    = scheduling_table.get_below_buffer_pos(idx_);
    unsigned access_count = 1;

    switch(oper_) {
        case operation_t::READ:
            access_count = scheduling_table.get_correlation_product(idx_, correlation_t::IWO)
                         * scheduling_table.get_correlation_product(idx_, correlation_t::WO)
                         * scheduling_table.get_correlation_product(idx_, correlation_t::OI)
                         * (scheduling_table.get_correlation_product(idx_, correlation_t::IW) - 1);
            if(dataflow == dataflow_t::OS) {
                access_count = scheduling_table.get_correlation_product(idx_, correlation_t::IWO)
                             * scheduling_table.get_correlation_product(idx_, correlation_t::WO)
                             * scheduling_table.get_correlation_product(idx_, correlation_t::OI)
                             * (scheduling_table.get_correlation_product(lower_level_idx, correlation_t::IW) - 1);
                if(idx_ == dram_idx) access_count = 0;
            }
            break;
        case operation_t::WRITE:
            access_count = value_;
            if(dataflow == dataflow_t::OS) {
                write_access_count /= update_irrelevant_mapping_value(idx_, data_t::OUTPUT); 
            }
            break;
        default:
            break;
    }
    return access_count;
}
unsigned analyzer_t::update_irrelevant_mapping_value(unsigned row_idx_,
                                                      data_t stationary_data_) {
    unsigned irrelevant_mapping_value = 1;
    
    // Product all irrelevant mapping values with stationary data
    switch(stationary_data_) {
    case data_t::INPUT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::K);
        break;
    case data_t::WEIGHT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::B)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::P)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::Q);
        break;
    case data_t::OUTPUT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::C)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::R)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::S);
        break;
    default:
        std::cerr << "Error in irrelevant loop calculation" << std::endl;
        exit(0);
    }
    return irrelevant_mapping_value;
}
// Update num. activated components
void analyzer_t::update_active_components() {
    // Get num active MACs 
    if(macs_capacity.dim_x * macs_capacity.dim_y == 1) {
        macs_actived.dim_x = 1;
        macs_actived.dim_y = 1;
    }
    else {
        macs_actived.dim_x = scheduling_table.get_row_product(lb_idx -2);
        macs_actived.dim_y = scheduling_table.get_row_product(lb_idx -1);
    }
    num_active_macs  = macs_actived.dim_x * macs_actived.dim_y;
    // Get num active PEs
    if(pes_capacity.dim_x * pes_capacity.dim_y == 1) {
        pes_actived.dim_x = 1;
        pes_actived.dim_y = 1;
    }
    else {
        pes_actived.dim_x = scheduling_table.get_row_product(gb_idx -2);
        pes_actived.dim_y = scheduling_table.get_row_product(gb_idx -1);
    }
    num_active_pes   = pes_actived.dim_x * pes_actived.dim_y;

    // Get num active Chips
    if(chips_capacity.dim_x * chips_capacity.dim_y == 1) {
        chips_actived.dim_x = 1;
        chips_actived.dim_y = 1;
    }
    else {
        chips_actived.dim_x = scheduling_table.get_row_product(dram_idx -2);
        chips_actived.dim_y = scheduling_table.get_row_product(dram_idx -1);
    }
    num_active_chips = chips_actived.dim_x * chips_actived.dim_y;
    return;
}
// Estimate accelerator's cost for a given scheduling table
void analyzer_t::estimate_cost() {
    update_cycle(); 
    update_energy(); 
    return;
}
void analyzer_t::estimate_cross_layer_reuse(scheduling_table_t prev_table_,
                                            metric_t      metric_) {

    /* 
     * NOTE, Accelerator doesn't have to write back OUTPUT data to DRAM
     * when processing previous layer but also doesn't have to read INPUT data
     * from DRAM when processing current layer for overlapped data size.
     * That's the reason for considering both `INPUT` and `OUTPUT` access cost. 
     */
    unsigned dram_idx        = accelerator->get_num_components() - 1;
	unsigned overlapped_size = 0;
	float    reduced_cost    = 0.0f;
    float    bandwidth       = accelerator->get_bandwidth(dram_idx)
                             / accelerator->get_precision();
 
    std::vector<float> unit_energy = accelerator->get_component_energy(dram_idx);
    std::vector<float> unit_cycle  = accelerator->get_component_cycle(dram_idx);
    float  input_access_energy = unit_energy.at((unsigned)data_t::INPUT);
    float output_access_energy = unit_energy.at((unsigned)data_t::INPUT);
    float   input_access_cycle = unit_cycle.at((unsigned)data_t::INPUT);
    float  output_access_cycle = unit_cycle.at((unsigned)data_t::INPUT);
	// Compute overlapped tile size
	overlapped_size = compute_overlapped_size(prev_table_);
	// Compute the amount of reduced cost
	if(metric_ == metric_t::ENERGY) {
        // Energy
        reduced_cost += overlapped_size
                     * (input_access_energy + output_access_energy);
        // Change the cost of target scheduling_option
	    total_energy -= reduced_cost;
	}
	else if(metric_ == metric_t::CYCLE) {
		reduced_cost = ceil(overlapped_size / bandwidth)
                     * (input_access_cycle + output_access_cycle); 
	    total_cycle -= reduced_cost;
	}
	return;
}
// Compute overlapped data size between previous scheduling table and current table
unsigned analyzer_t::compute_overlapped_size(scheduling_table_t prev_table_) {
    scheduling_table_t prev_table = prev_table_;
    scheduling_table_t curr_table = scheduling_table;

    unsigned layer_idx = scheduling_table.get_layer_index();
    unsigned stride = network->get_stride(layer_idx);
    unsigned dram_idx = scheduling_table.get_num_rows() - 1;
    // Compute output size of prev_layer
    unsigned prev_output_b = prev_table.get_column_wise_product(parameter_t::B, 0, dram_idx - 1);
    unsigned prev_output_h = prev_table.get_column_wise_product(parameter_t::P, 0, dram_idx - 1);
    unsigned prev_output_w = prev_table.get_column_wise_product(parameter_t::Q, 0, dram_idx - 1);
    unsigned prev_output_c = prev_table.get_column_wise_product(parameter_t::C, 0, dram_idx - 1);
    // Compute input size of current_layer
    unsigned curr_input_b  = curr_table.get_column_wise_product(parameter_t::B, 0, dram_idx - 1);
    unsigned curr_input_h  = (curr_table.get_column_wise_product(parameter_t::P, 0, dram_idx - 1) - 1)
                           * stride
                           + curr_table.get_column_wise_product(parameter_t::R, 0, dram_idx - 1);
    unsigned curr_input_w  = (curr_table.get_column_wise_product(parameter_t::Q, 0, dram_idx - 1) - 1)
                           * stride
                           + curr_table.get_column_wise_product(parameter_t::S, 0, dram_idx - 1);
    unsigned curr_input_c  = curr_table.get_column_wise_product(parameter_t::C, 0, dram_idx - 1);
    // Compute overlapped data size
    unsigned overlapped_b  = prev_output_b >= curr_input_b 
                           ? curr_input_b : prev_output_b;
    unsigned overlapped_h  = prev_output_h >= curr_input_h 
                           ? curr_input_h : prev_output_h;
    unsigned overlapped_w  = prev_output_w >= curr_input_w 
                           ? curr_input_w : prev_output_w;
    unsigned overlapped_c  = prev_output_c >= curr_input_c 
                           ? curr_input_c : prev_output_c;

    return overlapped_b * overlapped_h * overlapped_w * overlapped_c;
}
float max(float cost_i_, float cost_w_, float cost_o_) {
    float rtn = 0.0f;
    if(cost_i_ > cost_w_) {
        rtn = cost_i_;
    }
    else { rtn = cost_w_; }
    if(cost_o_ > rtn) {
        rtn = cost_o_;
    }
    return rtn;
}
// Update accelerator's cycle
void analyzer_t::update_cycle() {
    float cycle_i = 0.0f;
    float cycle_w = 0.0f;
    float cycle_o = 0.0f;
    // Get MAC cycle
    mac_cycle = scheduling_table.get_num_mac_operations()
              / (num_active_macs * num_active_pes * num_active_chips);
    //  Get local buffer cycle
    cycle_i = std::ceil(lb_tile_size_send.input * lb_access_count.input_rd
            / lb_bandwidth)
            * lb_unit_cycle.input;
    cycle_w = std::ceil(lb_tile_size_send.weight * lb_access_count.weight_rd
            / lb_bandwidth)
            * lb_unit_cycle.weight;
    cycle_o = std::ceil(lb_tile_size_send.output 
            * (lb_access_count.output_rd + lb_access_count.output_wt) / lb_bandwidth)
            * lb_unit_cycle.output;
    if(is_lb_shared) {
        local_buffer_cycle = cycle_i + cycle_w + cycle_o;
    }
    else {
        local_buffer_cycle = max(cycle_i, cycle_w, cycle_o);
    }
    // Get global buffer cycle
    cycle_i = std::ceil(gb_tile_size_send.input * gb_access_count.input_rd
            / gb_bandwidth)
            * gb_unit_cycle.input;
    cycle_w = std::ceil(gb_tile_size_send.weight * gb_access_count.weight_rd
            / gb_bandwidth)
            * gb_unit_cycle.weight;
    cycle_o = std::ceil(gb_tile_size_send.output 
            * (gb_access_count.output_rd + gb_access_count.output_wt) / gb_bandwidth)
            * gb_unit_cycle.output;
    if(is_gb_shared) {
        global_buffer_cycle = cycle_i + cycle_w + cycle_o;
    }
    else {
        global_buffer_cycle = max(cycle_i, cycle_w, cycle_o);
    }
    // Get dram cycle
    cycle_i = std::ceil(dram_tile_size_send.input * dram_access_count.input_rd 
            / dram_bandwidth)
            * dram_unit_cycle.input;
    cycle_w = std::ceil(dram_tile_size_send.weight * dram_access_count.weight_rd 
            / dram_bandwidth)
            * dram_unit_cycle.weight;
    cycle_o = std::ceil(dram_tile_size_send.output 
            * (dram_access_count.output_rd + dram_access_count.output_wt) / dram_bandwidth)
            * dram_unit_cycle.output;
    dram_cycle = cycle_i + cycle_w + cycle_o;

    // Update total cycle 
    total_cycle = mac_cycle + local_buffer_cycle + global_buffer_cycle + dram_cycle;
}
// Update accelerator's energy 
void analyzer_t::update_energy() {
    // Accumulate MAC unit cost
    mac_energy = scheduling_table.get_num_mac_operations()
               * accelerator->get_mac_energy();
    // Get local buffer energy
    lb_energy_upper  = lb_tile_size_send.input  * lb_access_count.input_rd  
                     * lb_unit_energy.input;
    lb_energy_upper += lb_tile_size_send.weight * lb_access_count.weight_rd 
                     * lb_unit_energy.weight;
    lb_energy_upper += lb_tile_size_send.output 
                     * (lb_access_count.output_rd + lb_access_count.output_wt) 
                     * lb_unit_energy.output;
    lb_energy_upper *= num_active_pes * num_active_chips;

    lb_energy_lower  = lb_tile_size_alloc.input  * gb_access_count.input_rd  
                     * lb_unit_energy.input;
    lb_energy_lower += lb_tile_size_alloc.weight * gb_access_count.weight_rd 
                     * lb_unit_energy.weight;
    lb_energy_lower += lb_tile_size_alloc.output 
                     * (gb_access_count.output_rd + gb_access_count.output_wt) 
                     * lb_unit_energy.output;
    lb_energy_lower *= num_active_pes * num_active_chips;

    local_buffer_energy = lb_energy_upper + lb_energy_lower;
    // Get global buffer energy
    gb_energy_upper  = gb_tile_size_send.input  * gb_access_count.input_rd  
                     * gb_unit_energy.input;
    gb_energy_upper += gb_tile_size_send.weight * gb_access_count.weight_rd 
                     * gb_unit_energy.weight;
    gb_energy_upper += gb_tile_size_send.output 
                     * (gb_access_count.output_rd + gb_access_count.output_wt) 
                     * gb_unit_energy.output;
    gb_energy_upper *= num_active_chips;
    gb_energy_lower  = gb_tile_size_alloc.input  * dram_access_count.input_rd  
                     * gb_unit_energy.input;
    gb_energy_lower += gb_tile_size_alloc.weight * dram_access_count.weight_rd 
                     * gb_unit_energy.weight;
    gb_energy_lower += gb_tile_size_alloc.output 
                     * (dram_access_count.output_rd + dram_access_count.output_wt) 
                     * gb_unit_energy.output;
    gb_energy_lower *= num_active_chips;
    global_buffer_energy = gb_energy_upper + gb_energy_lower;
    // Get dram energy
    dram_energy      = dram_tile_size_send.input  * dram_access_count.input_rd  
                     * dram_unit_energy.input;
    dram_energy     += dram_tile_size_send.weight * dram_access_count.weight_rd 
                     * dram_unit_energy.weight;
    dram_energy     += dram_tile_size_send.output 
                     * (dram_access_count.output_rd + dram_access_count.output_wt) 
                     * dram_unit_energy.output;
    // Calcue total energy
    total_energy = mac_energy + local_buffer_energy + global_buffer_energy + dram_energy;
    
    // /**
    //  * @brief Total static energy (pJ) 
    //  * = Unit static power (mW) x clock time (ns) x total cycle count
    //  */
    mac_static = accelerator->get_mac_static_power()
                * accelerator->get_clock_time()
                * accelerator->get_total_num_MACs()
                * total_cycle;
    local_buffer_static  = (lb_unit_static.input + lb_unit_static.weight +lb_unit_static.output)
                         * accelerator->get_clock_time()
                         * pes_capacity.dim_x * pes_capacity.dim_y
                         * chips_capacity.dim_x * chips_capacity.dim_y
                         * total_cycle;
    global_buffer_static = (gb_unit_static.input + gb_unit_static.weight +gb_unit_static.output)
                         * accelerator->get_clock_time()
                         * chips_capacity.dim_x * chips_capacity.dim_y
                         * total_cycle;
    dram_static          = (dram_unit_static.input + dram_unit_static.weight + dram_unit_static.output)
                         * accelerator->get_clock_time()
                         * total_cycle;
    total_static_energy  = mac_static + local_buffer_static + global_buffer_static + dram_static;
}
// Print out analysis result
void analyzer_t::print_results() {
    std::cout << "[Scheduling Table]"<< std::endl;
    scheduling_table.print_stats();
    std::cout << "[Analyze Results]"<< std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "  LB I  transfer tile size : " << lb_tile_size_send.input    << "\n"
              << "  LB W  transfer tile size : " << lb_tile_size_send.weight   << "\n"
              << "  LB O  transfer tile size : " << lb_tile_size_send.output   << "\n"
              << "  GB I  transfer tile size : " << gb_tile_size_send.input    << "\n"
              << "  GB W  transfer tile size : " << gb_tile_size_send.weight   << "\n"
              << "  GB O  transfer tile size : " << gb_tile_size_send.output   << "\n"
              << "DRAM I  transfer tile size : " << dram_tile_size_send.input  << "\n"
              << "DRAM W  transfer tile size : " << dram_tile_size_send.weight << "\n"
              << "DRAM O  transfer tile size : " << dram_tile_size_send.output << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "  LB I allocated tile size : " << lb_tile_size_alloc.input    << "\n"
              << "  LB W allocated tile size : " << lb_tile_size_alloc.weight   << "\n"
              << "  LB O allocated tile size : " << lb_tile_size_alloc.output   << "\n"
              << "  GB I allocated tile size : " << gb_tile_size_alloc.input    << "\n"
              << "  GB W allocated tile size : " << gb_tile_size_alloc.weight   << "\n"
              << "  GB O allocated tile size : " << gb_tile_size_alloc.output   << "\n"
              << "DRAM I allocated tile size : " << dram_tile_size_alloc.input  << "\n"
              << "DRAM W allocated tile size : " << dram_tile_size_alloc.weight << "\n"
              << "DRAM O allocated tile size : " << dram_tile_size_alloc.output << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "  LB I  read access count : " << lb_access_count.input_rd     << "\n"
              << "  LB W  read access count : " << lb_access_count.weight_rd    << "\n"
              << "  LB O  read access count : " << lb_access_count.output_rd << "\n"
              << "  LB O write access count : " << lb_access_count.output_wt << "\n"
              << "  GB I  read access count : " << gb_access_count.input_rd     << "\n"
              << "  GB W  read access count : " << gb_access_count.weight_rd    << "\n"
              << "  GB O  read access count : " << gb_access_count.output_rd << "\n"
              << "  GB O write access count : " << gb_access_count.output_wt << "\n"
              << "DRAM I  read access count : " << dram_access_count.input_rd     << "\n"
              << "DRAM W  read access count : " << dram_access_count.weight_rd    << "\n"
              << "DRAM O  read access count : " << dram_access_count.output_rd << "\n"
              << "DRAM O write access count : " << dram_access_count.output_wt << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "     NUM ACTIVE MAC ARRAY : " << num_active_macs << std::endl;
    std::cout << "      NUM ACTIVE PE ARRAY : " << num_active_pes << std::endl;
    std::cout << "   NUM ACTIVE MULTI CHIPS : " << num_active_chips << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "  MAC DYNAMIC ENERGY : " << mac_energy           << "\n"
              << "   LB DYNAMIC ENERGY : " << local_buffer_energy  << "\n"
              << "   GB DYNAMIC ENERGY : " << global_buffer_energy << "\n"
              << " DRAM DYNAMIC ENERGY : " << dram_energy          << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "   MAC STATIC ENERGY : " << mac_static           << "\n"
              << "    LB STATIC ENERGY : " << local_buffer_static  << "\n"
              << "    GB STATIC ENERGY : " << global_buffer_static << "\n"
              << "  DRAM STATIC ENERGY : " << dram_static          << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "          MAC ENERGY : " << mac_energy + mac_static << "\n"
              << "           LB ENERGY : " << local_buffer_energy + local_buffer_static << "\n"
              << "           GB ENERGY : " << global_buffer_energy + global_buffer_static << "\n"
              << "         DRAM ENERGY : " << dram_energy  + dram_static << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "           MAC CYCLE : " << mac_cycle              << "\n"
              << "            LB CYCLE : " << local_buffer_cycle     << "\n"
              << "            GB CYCLE : " << global_buffer_cycle    << "\n"
              << "          DRAM CYCLE : " << dram_cycle             << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "TOTAL DYNAMIC ENERGY : " << total_energy << std::endl;
    std::cout << " TOTAL STATIC ENERGY : " << total_static_energy << std::endl;
    std::cout << "        TOTAL ENERGY : " << total_energy + total_static_energy << std::endl;
    std::cout << "         TOTAL CYCLE : " << total_cycle            << std::endl;
    std::cout << "**************************" << std::endl;
    return;
}

void analyzer_t::reset() {
    return;
}

float analyzer_t::get_total_cost(metric_t  metric_) {
    float rtn = 0;
    switch(metric_) {
        case metric_t::ENERGY:
            rtn = total_energy;
            break;
        case metric_t::CYCLE:
            rtn = total_cycle;
            break;
        default:
            std::cerr << "Error invalid metric" << std::endl;
            exit(0);
    }
    return rtn;
}
float analyzer_t::get_target_level_cost(unsigned idx_, metric_t metric_) {
    float rtn = 0;
    // Set Scheduling table row index
    unsigned lower_idx = idx_;
    if(metric_ == metric_t::ENERGY) {
        if(lower_idx == dram_idx) {
            if(is_gb_exist) {
                rtn = dram_energy + lb_energy_lower;
            }
            else {
                rtn = dram_energy + gb_energy_lower;
            }
        }
        else if(lower_idx == gb_idx) {
            if(macs_capacity.dim_x * macs_capacity.dim_y == 1) {
                rtn = gb_energy_upper + lb_energy_lower;
            }
            else {
                rtn = gb_energy_upper + lb_energy_lower + lb_energy_upper;
            }
        }
        else if(lower_idx == lb_idx) {
            rtn = lb_energy_upper;
        }
        else {
            std::cerr << "Invalid target level " << idx_ << std::endl;
            exit(0);
        }
    }
    else if(metric_ == metric_t::CYCLE) {
        if(lower_idx == dram_idx) {
            rtn = dram_cycle + mac_cycle;
        }
        else if(lower_idx == gb_idx) {
            rtn = global_buffer_cycle + local_buffer_cycle + mac_cycle;
        }
        else if(lower_idx == lb_idx) {
            rtn = local_buffer_cycle + mac_cycle;
        }
        else {
            std::cerr << "Invalid target level " << idx_ << std::endl;
            exit(0);
        }
    }
    else { std::cerr << "Error invalid metric type" << std::endl; exit(0); }
    return rtn;
}
unsigned analyzer_t::get_target_level_factorization(unsigned idx_) {
    return get_factorization_degrees(idx_);
}
unsigned analyzer_t::get_num_active_chips() {
    unsigned rtn = 1;
    unsigned component_idx = 0;
    const unsigned dim_x = (unsigned)dimension_t::DIM_X;
    const unsigned dim_y = (unsigned)dimension_t::DIM_Y;

    // Find spatial component level that placed above DRAM
    for(int i = scheduling_table.get_num_rows() - 1; i > 0; i--) {
        if(scheduling_table.get_component_type(i) == component_type_t::SPATIAL) {
            component_idx = scheduling_table.get_component_index(i);
            if(component_idx == UINT_MAX) { rtn *= 1; }
            else {
                rtn *= accelerator->get_active_components(component_idx).at(dim_x)
                    * accelerator->get_active_components(component_idx).at(dim_y);
            }
            break;
        }
    }
    return rtn;
}
unsigned analyzer_t::get_tile_size(unsigned idx_, data_t data_type_) {
    unsigned rtn;
    std::vector<unsigned> tile_size;
    unsigned component_idx = scheduling_table.get_component_index(idx_);
    // Get tile size 
    tile_size = accelerator->get_allocated_size(component_idx, 
                                                direction_t::UPPER);
    assert(tile_size.size() == (unsigned)data_t::SIZE);
    rtn = tile_size.at((unsigned)data_type_);
    return rtn;
}
unsigned analyzer_t::get_access_count(unsigned idx_, operation_t op_, data_t data_type_) {
    unsigned rtn;
    std::vector<unsigned> access_count;
    if(component_idx_curr == idx_) { 
        if(data_type_ == data_t::OUTPUT && op_ == operation_t::READ) { rtn = 0; }
        else{ rtn = 1; }
    }
    else {
        // Get access count and unit access energy 
        access_count = accelerator->get_tile_access_count(idx_, op_,
                                                        direction_t::UPPER);
        assert(access_count.size() == (unsigned)data_t::SIZE);
        rtn = access_count.at((unsigned)data_type_);
    }
    return rtn;
}
float analyzer_t::get_energy_consumption(unsigned idx_,
                                         direction_t direction_) {
    float rtn = 0.0;
    
    std::vector<unsigned> tile_size(3,0);
    std::vector<unsigned> access_count(3,0);
    std::vector<float> unit_energy(3,0);
    const unsigned dim_x = (unsigned)dimension_t::DIM_X;
    const unsigned dim_y = (unsigned)dimension_t::DIM_Y;

    unsigned component_idx = scheduling_table.get_component_index(idx_);
    for (unsigned op_type = 0; op_type < (unsigned)operation_t::SIZE; op_type++) {
        // Get tile size and access count and unit access energy 
        tile_size = accelerator->get_allocated_size(component_idx, direction_);
        access_count = accelerator->get_tile_access_count(component_idx, 
                                                         (operation_t)op_type, 
                                                          direction_);
        unit_energy = accelerator->get_component_energy(component_idx);
        // Calculate energy consumption
        for (unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
            rtn += (float)tile_size.at(i) * (float)access_count.at(i) 
                 * unit_energy.at(i);
        }
    }
    // Consider energy consumption of all active components
    for(unsigned i = idx_ + 1; i < scheduling_table.get_num_rows(); i++) {
        if(scheduling_table.get_component_type(i) == component_type_t::SPATIAL) {
            component_idx = scheduling_table.get_component_index(i);
            if(component_idx == UINT_MAX) { rtn *= 1; }
            else {
                rtn *= accelerator->get_active_components(component_idx).at(dim_x)
                    * accelerator->get_active_components(component_idx).at(dim_y);
            }
            i++; // Skip Y
        }
    }
    return rtn;
}
float analyzer_t::get_cycle_consumption(unsigned idx_,
                                        direction_t direction_) {
    float rtn = 0.0;

    std::vector<unsigned> tile_size(3,0);
    std::vector<unsigned> access_count(3,0);
    std::vector<float> unit_cycle(3,0);
    unsigned bandwidth = 1;

    unsigned component_idx = scheduling_table.get_component_index(idx_);
    for (unsigned op_type = 0; op_type < (unsigned)operation_t::SIZE; op_type++) {
        // Get tile size and access count and unit access energy 
        tile_size = accelerator->get_allocated_size(component_idx, direction_);
        access_count = accelerator->get_tile_access_count(component_idx, 
                                                         (operation_t)op_type, 
                                                          direction_);
        unit_cycle = accelerator->get_component_cycle(component_idx);
        bandwidth  = accelerator->get_bandwidth(component_idx);
        // Calculate cycle count
        for (unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
            assert(tile_size.size() == (unsigned)data_t::SIZE
                && access_count.size() == (unsigned)data_t::SIZE
                && unit_cycle.size() == (unsigned)data_t::SIZE);
            // Buffer type is unified
            if(accelerator->get_size(component_idx).size()==1) {
                rtn += std::ceil((float)tile_size.at(i) / (float)bandwidth) 
                     * (float)access_count.at(i) * unit_cycle.at(i);
            }
            // Buffer type is seperated
            else {
                unsigned data_transfer_cycle 
                    = std::ceil((float)tile_size.at(i) / (float)bandwidth) 
                    * (float)access_count.at(i) * unit_cycle.at(i);
                // Return maximum cycle 
                if(data_transfer_cycle > rtn) { rtn = data_transfer_cycle; }
            }
        }
    }
    return rtn;
}
unsigned analyzer_t::get_factorization_degrees(unsigned idx_) {
    unsigned factorization_degrees = 1;
    std::vector<unsigned> num_factors;
    std::vector<unsigned> parameters = scheduling_table.get_row_values(idx_);
    for(auto value = parameters.begin(); value != parameters.end(); ++value) {
        for(unsigned i = 2; i < *value; i++) {
            while(1) {
                if(*value % i == 0) {
                    num_factors.emplace_back(i);
                    *value/=i;
                }
                else { break; }
            }
        }
        factorization_degrees *= num_factors.size() + 1;
        num_factors.clear();
    }
    return factorization_degrees;
}

void analyzer_t::init_mac_array() {
    // Init MAC unit
    mac_unit_energy     = accelerator->get_mac_energy();
    mac_unit_cycle      = 1;
    mac_unit_static     = accelerator->get_mac_static_power();
    
    // Init MAC array
    macs_capacity.dim_x = accelerator->get_mac_array_size(dimension_t::DIM_X); 
    macs_capacity.dim_y = accelerator->get_mac_array_size(dimension_t::DIM_Y);
}
void analyzer_t::init_local_buffer() {
    // Init unit access cost
    std::vector<float> arr_unit_energy = accelerator->get_energy(buffer_t::LB);
    std::vector<float> arr_unit_static = accelerator->get_static(buffer_t::LB);
    std::vector<float> arr_unit_cycle  = accelerator->get_cycle(buffer_t::LB);
    lb_unit_energy.input    = arr_unit_energy.at((unsigned)data_t::INPUT);
    lb_unit_energy.weight   = arr_unit_energy.at((unsigned)data_t::WEIGHT);
    lb_unit_energy.output   = arr_unit_energy.at((unsigned)data_t::OUTPUT);
    lb_unit_static.input    = arr_unit_static.at((unsigned)data_t::INPUT);
    lb_unit_static.weight   = arr_unit_static.at((unsigned)data_t::WEIGHT);
    lb_unit_static.output   = arr_unit_static.at((unsigned)data_t::OUTPUT);
    lb_unit_cycle.input     = arr_unit_cycle.at((unsigned)data_t::INPUT);
    lb_unit_cycle.weight    = arr_unit_cycle.at((unsigned)data_t::WEIGHT);
    lb_unit_cycle.output    = arr_unit_cycle.at((unsigned)data_t::OUTPUT);

    // Init buffer size
    std::vector<float> arr_buffer_size = accelerator->get_size(buffer_t::LB);
    // If shared
    if(arr_buffer_size.size() == 1) {
        is_lb_shared = true;
        lb_capacity.shared = arr_buffer_size.back();
        lb_capacity.input  = 0;
        lb_capacity.weight = 0;
        lb_capacity.output = 0;
    }
    // If seperated
    else if(arr_buffer_size.size() == 3) {
        is_lb_shared = false;
        lb_capacity.shared = 0;
        lb_capacity.input  = arr_buffer_size.at((unsigned)data_t::INPUT)  / (accelerator->get_precision() / BYTE);
        lb_capacity.weight = arr_buffer_size.at((unsigned)data_t::WEIGHT) / (accelerator->get_precision() / BYTE);
        lb_capacity.output = arr_buffer_size.at((unsigned)data_t::OUTPUT) / (accelerator->get_precision() / BYTE);
    }
    else {
        // Virtual buffer
        is_lb_exist = false;
    }
    
    // Init bandwidth
    lb_bandwidth            = accelerator->get_bandwidth(buffer_t::LB) 
                            / accelerator->get_precision();
}
// Init PE array
void analyzer_t::init_pe_array() {
    pes_capacity.dim_x = accelerator->get_pe_array_size(dimension_t::DIM_X);
    pes_capacity.dim_y = accelerator->get_pe_array_size(dimension_t::DIM_Y);
}
// Init global buffer
void analyzer_t::init_global_buffer() {
    // Init unit access cost
    std::vector<float> arr_unit_energy = accelerator->get_energy(buffer_t::GB);
    std::vector<float> arr_unit_static = accelerator->get_static(buffer_t::GB);
    std::vector<float> arr_unit_cycle  = accelerator->get_cycle(buffer_t::GB);
    gb_unit_energy.input    = arr_unit_energy.at((unsigned)data_t::INPUT);
    gb_unit_energy.weight   = arr_unit_energy.at((unsigned)data_t::WEIGHT);
    gb_unit_energy.output   = arr_unit_energy.at((unsigned)data_t::OUTPUT);
    gb_unit_static.input    = arr_unit_static.at((unsigned)data_t::INPUT);
    gb_unit_static.weight   = arr_unit_static.at((unsigned)data_t::WEIGHT);
    gb_unit_static.output   = arr_unit_static.at((unsigned)data_t::OUTPUT);
    gb_unit_cycle.input     = arr_unit_cycle.at((unsigned)data_t::INPUT);
    gb_unit_cycle.weight    = arr_unit_cycle.at((unsigned)data_t::WEIGHT);
    gb_unit_cycle.output    = arr_unit_cycle.at((unsigned)data_t::OUTPUT);
    // Init buffer size
    std::vector<float> arr_buffer_size = accelerator->get_size(buffer_t::GB);
    // If shared
    if(arr_buffer_size.size() == 1) {
        is_gb_shared = true;
        gb_capacity.shared = arr_buffer_size.back();
        gb_capacity.input  = 0;
        gb_capacity.weight = 0;
        gb_capacity.output = 0;
    }
    // If seperated
    else if(arr_buffer_size.size() == 3) {
        is_gb_shared = false;
        gb_capacity.input  = arr_buffer_size.at((unsigned)data_t::INPUT)  / (accelerator->get_precision() / BYTE);
        gb_capacity.weight = arr_buffer_size.at((unsigned)data_t::WEIGHT) / (accelerator->get_precision() / BYTE);
        gb_capacity.output = arr_buffer_size.at((unsigned)data_t::OUTPUT) / (accelerator->get_precision() / BYTE);
    }
    else {
        is_gb_exist = false;
    }  
    // Init bandwidth
    gb_bandwidth            = accelerator->get_bandwidth(buffer_t::GB) 
                            / accelerator->get_precision();
}
void analyzer_t::init_multi_chips() {
    // Init Multi chip 
    chips_capacity.dim_x = accelerator->get_multi_chips_size(dimension_t::DIM_X);
    chips_capacity.dim_y = accelerator->get_multi_chips_size(dimension_t::DIM_Y);
}
void analyzer_t::init_dram() {
    // Init unit access cost
    std::vector<float> arr_unit_energy = accelerator->get_energy(buffer_t::DRAM);
    std::vector<float> arr_unit_static = accelerator->get_static(buffer_t::DRAM);
    std::vector<float> arr_unit_cycle  = accelerator->get_cycle(buffer_t::DRAM);
    dram_unit_energy.input  = arr_unit_energy.at((unsigned)data_t::INPUT);
    dram_unit_energy.weight = arr_unit_energy.at((unsigned)data_t::WEIGHT);
    dram_unit_energy.output = arr_unit_energy.at((unsigned)data_t::OUTPUT);
    dram_unit_static.input  = arr_unit_static.at((unsigned)data_t::INPUT);
    dram_unit_static.weight = arr_unit_static.at((unsigned)data_t::WEIGHT);
    dram_unit_static.output = arr_unit_static.at((unsigned)data_t::OUTPUT);
    dram_unit_cycle.input   = arr_unit_cycle.at((unsigned)data_t::INPUT);
    dram_unit_cycle.weight  = arr_unit_cycle.at((unsigned)data_t::WEIGHT);
    dram_unit_cycle.output  = arr_unit_cycle.at((unsigned)data_t::OUTPUT);
    // Init bandwidth
    dram_bandwidth          = accelerator->get_bandwidth(buffer_t::DRAM) 
                            / accelerator->get_precision();
}
