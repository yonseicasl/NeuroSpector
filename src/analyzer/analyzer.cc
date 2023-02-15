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

        dram_idx = (unsigned)component_t::DRAM;
        gb_idx   = (unsigned)component_t::GB;
        lb_idx   = (unsigned)component_t::LB;
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
analyzer_t::~analyzer_t() {
}
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

    // Delete accelerator and network
    delete accelerator;
    delete network;
}
// Initialize analyzer to evaluate a given scheduling table
void analyzer_t::init(scheduling_table_t scheduling_table_) {
    // Copy scheduling table
    scheduling_table = scheduling_table_;
    dram_idx = scheduling_table.get_num_rows() - 1;
    gb_idx = (unsigned)component_t::GB;
    lb_idx = (unsigned)component_t::LB;
    update_tile_size();
    update_access_count();
    update_active_components();
    return;
}
// Check scheduling table's validity
bool analyzer_t::check_validity() {
    bool rtn = true;
    // Traverse all rows and check hardware constraints. 
    rtn = rtn && check_hardware_constraints();
    // Traverse all columns and check network constraints. 
    rtn = rtn && check_network_constraints();
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
        rtn = false;
    }

    // Check Local Buffer validity
    if(is_lb_exist) {
        if(is_lb_shared) {
            shared_tile_size  = lb_bypass.input  ?  0 : lb_tile_size_alloc.input;
            shared_tile_size += lb_bypass.weight ?  0 : lb_tile_size_alloc.weight;
            shared_tile_size += lb_bypass.output ?  0 : lb_tile_size_alloc.output;
            if(shared_tile_size > lb_capacity.shared ) {
                rtn = false;
            }
        }
        else {
            rtn = rtn && (lb_tile_size_alloc.input  < lb_capacity.input);
            rtn = rtn && (lb_tile_size_alloc.weight < lb_capacity.weight);
            rtn = rtn && (lb_tile_size_alloc.output < lb_capacity.output);
        }
    }
    // Check PE array validity
    if(pes_actived.dim_x > pes_capacity.dim_x 
    || pes_actived.dim_y > pes_capacity.dim_y) {
        rtn = false;
    }
    // Check Global Buffer validity
    if(is_gb_exist) {
        if(is_gb_shared) {
            shared_tile_size  = gb_bypass.input  ? 0 : gb_tile_size_alloc.input;
            shared_tile_size += gb_bypass.weight ? 0 : gb_tile_size_alloc.weight;
            shared_tile_size += gb_bypass.output ? 0 : gb_tile_size_alloc.output;
            if(shared_tile_size > gb_capacity.shared ) {
                rtn = false;
            }
        }
        else {
            rtn = rtn && (gb_tile_size_alloc.input  < gb_capacity.input);
            rtn = rtn && (gb_tile_size_alloc.weight < gb_capacity.weight);
            rtn = rtn && (gb_tile_size_alloc.output < gb_capacity.output);
        }
    }
    // Check Multi chips validity
    if(chips_actived.dim_x > chips_capacity.dim_x 
    || chips_actived.dim_y > chips_capacity.dim_y) {
        rtn = false;
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
    lb_tile_size_send.input     =  update_input_tile_size(component_t::LB, direction_t::UPPER);
    lb_tile_size_send.weight    = update_weight_tile_size(component_t::LB, direction_t::UPPER);
    lb_tile_size_send.output    = update_output_tile_size(component_t::LB, direction_t::UPPER);
    lb_tile_size_alloc.input    =  update_input_tile_size(component_t::LB, direction_t::LOWER);
    lb_tile_size_alloc.weight   = update_weight_tile_size(component_t::LB, direction_t::LOWER);
    lb_tile_size_alloc.output   = update_output_tile_size(component_t::LB, direction_t::LOWER);
    
    // Update   GB tile size
    gb_tile_size_send.input     =  update_input_tile_size(component_t::GB, direction_t::UPPER);
    gb_tile_size_send.weight    = update_weight_tile_size(component_t::GB, direction_t::UPPER);
    gb_tile_size_send.output    = update_output_tile_size(component_t::GB, direction_t::UPPER);
    gb_tile_size_alloc.input    =  update_input_tile_size(component_t::GB, direction_t::LOWER);
    gb_tile_size_alloc.weight   = update_weight_tile_size(component_t::GB, direction_t::LOWER);
    gb_tile_size_alloc.output   = update_output_tile_size(component_t::GB, direction_t::LOWER);

    // Update DRAM tile size
    dram_tile_size_send.input   =  update_input_tile_size(component_t::DRAM, direction_t::UPPER);
    dram_tile_size_send.weight  = update_weight_tile_size(component_t::DRAM, direction_t::UPPER);
    dram_tile_size_send.output  = update_output_tile_size(component_t::DRAM, direction_t::UPPER);
    dram_tile_size_alloc.input  =  update_input_tile_size(component_t::DRAM, direction_t::LOWER);
    dram_tile_size_alloc.weight = update_weight_tile_size(component_t::DRAM, direction_t::LOWER);
    dram_tile_size_alloc.output = update_output_tile_size(component_t::DRAM, direction_t::LOWER);

    return;
}
// Update input tile size to allocate (or send)
unsigned analyzer_t::update_input_tile_size(component_t buffer_,
                                            direction_t direction_) {
    unsigned tile_size = 1;
    unsigned buffer_idx = 0;
    switch (buffer_) {
    case component_t::LB:
        buffer_idx = lb_idx;
        break;
    case component_t::GB:
        buffer_idx = gb_idx;
        break;
    case component_t::DRAM:
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
unsigned analyzer_t::update_weight_tile_size(component_t buffer_,
                                             direction_t direction_) {
    unsigned tile_size = 1;
    unsigned buffer_idx = 0;
    switch (buffer_) {
    case component_t::LB:
        buffer_idx = lb_idx;
        break;
    case component_t::GB:
        buffer_idx = gb_idx;
        break;
    case component_t::DRAM:
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
unsigned analyzer_t::update_output_tile_size(component_t buffer_,
                                             direction_t direction_) {
    unsigned tile_size = 1;
    unsigned buffer_idx = 0;
    switch (buffer_) {
    case component_t::LB:
        buffer_idx = lb_idx;
        break;
    case component_t::GB:
        buffer_idx = gb_idx;
        break;
    case component_t::DRAM:
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
    
    dataflow = scheduling_table.get_dataflow(lb_idx);
    // dataflow = handle_dataflow_exception_case(gb_idx, dataflow, lb_bypass);
    value = scheduling_table.get_temporal_row_wise_product(gb_idx, dram_idx);
    gb_access_count.input_rd  = update_input_access_count(gb_idx, value);
    gb_access_count.weight_rd = update_weight_access_count(gb_idx, value);
    gb_access_count.output_rd = update_output_access_count(gb_idx, value, operation_t::READ);
    gb_access_count.output_wt = update_output_access_count(gb_idx, value, operation_t::WRITE);

    // Update dram access count
    dataflow = scheduling_table.get_dataflow(gb_idx);
    // dataflow = handle_dataflow_exception_case(dram_idx, dataflow, gb_bypass);
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
    // Bypass adjustment
    if(gb_bypass.input && (dram_tile_size_alloc.input == dram_tile_size_send.input)
    && (gb_tile_size_alloc.input != gb_tile_size_send.input)) {
        dram_access_count.output_wt *= update_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::INPUT);
    }
    if(gb_bypass.weight && (dram_tile_size_alloc.weight == dram_tile_size_send.weight)
    && (gb_tile_size_alloc.weight != gb_tile_size_send.weight)) {
        dram_access_count.weight_rd *= update_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::WEIGHT);
    }
    if(gb_bypass.output && (dram_tile_size_alloc.output == dram_tile_size_send.output)
    && (gb_tile_size_alloc.output != gb_tile_size_send.output)) {
        dram_access_count.output_rd = update_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::OUTPUT) - 1;
        dram_access_count.output_wt *= update_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::OUTPUT);
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
                access_count /= update_irrelevant_mapping_value(idx_, data_t::OUTPUT); 
            }
            break;
        default:
            break;
    }
    return access_count;
}
// Product all irrelevant mapping values with stationary data
unsigned analyzer_t::update_irrelevant_mapping_value(unsigned row_idx_,
                                                      data_t stationary_data_) {
    unsigned irrelevant_mapping_value = 1;
    unsigned alternative_one = 1;
    unsigned alternative_two = 1;
    
    switch(stationary_data_) {
    case data_t::INPUT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::K);

        // If nothing repeats in the inner loops.
        if(irrelevant_mapping_value == 1) {
            // evaluates both other dataflows on behalf of the ineffective current dataflow.
            // Weight stationary
            alternative_one = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::B)
                            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::P)
                            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::Q);
            // Output stationary 
            alternative_two = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::C)
                            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::R)
                            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::S);
            if(alternative_one >  alternative_two) {
                irrelevant_mapping_value = alternative_one;
            }
            else {
                irrelevant_mapping_value = alternative_two;
            }
        }
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
	unsigned overlapped_size = 0;
	float    reduced_cost    = 0.0f;
    float    bitwidth       = accelerator->get_bitwidth(component_t::DRAM)
                             / accelerator->get_precision();
 
    float  input_access_energy = dram_unit_energy.input;
    float output_access_energy = dram_unit_energy.output;
    float   input_access_cycle = dram_unit_cycle.input;
    float  output_access_cycle = dram_unit_cycle.output;
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
		reduced_cost = ceil(overlapped_size / bitwidth)
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
            / lb_bitwidth)
            * lb_unit_cycle.input;
    cycle_w = std::ceil(lb_tile_size_send.weight * lb_access_count.weight_rd
            / lb_bitwidth)
            * lb_unit_cycle.weight;
    cycle_o = std::ceil(lb_tile_size_send.output 
            * (lb_access_count.output_rd + lb_access_count.output_wt) / lb_bitwidth)
            * lb_unit_cycle.output;
    if(is_lb_shared) {
        local_buffer_cycle = cycle_i + cycle_w + cycle_o;
    }
    else {
        local_buffer_cycle = max(cycle_i, cycle_w, cycle_o);
    }
    // Get global buffer cycle
    cycle_i = std::ceil(gb_tile_size_send.input * gb_access_count.input_rd
            / gb_bitwidth)
            * gb_unit_cycle.input;
    cycle_w = std::ceil(gb_tile_size_send.weight * gb_access_count.weight_rd
            / gb_bitwidth)
            * gb_unit_cycle.weight;
    cycle_o = std::ceil(gb_tile_size_send.output 
            * (gb_access_count.output_rd + gb_access_count.output_wt) / gb_bitwidth)
            * gb_unit_cycle.output;
    if(is_gb_shared) {
        global_buffer_cycle = cycle_i + cycle_w + cycle_o;
    }
    else {
        global_buffer_cycle = max(cycle_i, cycle_w, cycle_o);
    }
    // Get dram cycle
    cycle_i = std::ceil(dram_tile_size_send.input * dram_access_count.input_rd 
            / dram_bitwidth)
            * dram_unit_cycle.input;
    cycle_w = std::ceil(dram_tile_size_send.weight * dram_access_count.weight_rd 
            / dram_bitwidth)
            * dram_unit_cycle.weight;
    cycle_o = std::ceil(dram_tile_size_send.output 
            * (dram_access_count.output_rd + dram_access_count.output_wt) / dram_bitwidth)
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
    
    /**
     * @brief Total static energy (pJ) 
     * = Unit static power (mW) x clock time (ns) x total cycle count
     */
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
    float shared_tile_size = 0;
    update_bypassed_data();
    std::cout << "[Scheduling Table]"<< std::endl;
    scheduling_table.print_stats();
    std::cout << "[Analyze Results]"<< std::endl;
    // TRANSFERED TILE SIZE 
    //std::cout << "**************************" << std::endl;
    //std::cout << "  LB I  transfer tile size : " << lb_tile_size_send.input    << "\n"
              //<< "  LB W  transfer tile size : " << lb_tile_size_send.weight   << "\n"
              //<< "  LB O  transfer tile size : " << lb_tile_size_send.output   << "\n";
    //if(is_gb_exist) {
        //std::cout << "  GB I  transfer tile size : " << gb_tile_size_send.input    << "\n"
                  //<< "  GB W  transfer tile size : " << gb_tile_size_send.weight   << "\n"
                  //<< "  GB O  transfer tile size : " << gb_tile_size_send.output   << "\n";
    //}
    //std::cout << "DRAM I  transfer tile size : " << dram_tile_size_send.input  << "\n"
              //<< "DRAM W  transfer tile size : " << dram_tile_size_send.weight << "\n"
              //<< "DRAM O  transfer tile size : " << dram_tile_size_send.output << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "           LB I TILE SIZE : " << lb_tile_size_alloc.input    << "\n"
              << "           LB W TILE SIZE : " << lb_tile_size_alloc.weight   << "\n"
              << "           LB O TILE SIZE : " << lb_tile_size_alloc.output   << "\n";
    if(is_gb_exist) {
        std::cout << "           GB I TILE SIZE : " << gb_tile_size_alloc.input    << "\n"
                  << "           GB W TILE SIZE : " << gb_tile_size_alloc.weight   << "\n"
                  << "           GB O TILE SIZE : " << gb_tile_size_alloc.output   << "\n";
    }
    std::cout << "         DRAM I TILE SIZE : " << dram_tile_size_alloc.input  << "\n"
              << "         DRAM W TILE SIZE : " << dram_tile_size_alloc.weight << "\n"
              << "         DRAM O TILE SIZE : " << dram_tile_size_alloc.output << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "  LB I  READ ACCESS COUNT : " << lb_access_count.input_rd     << "\n"
              << "  LB W  READ ACCESS COUNT : " << lb_access_count.weight_rd    << "\n"
              << "  LB O  READ ACCESS COUNT : " << lb_access_count.output_rd << "\n"
              << "  LB O WRITE ACCESS COUNT : " << lb_access_count.output_wt << "\n";
    if(is_gb_exist) {
        std::cout << "  GB I  READ ACCESS COUNT : " << gb_access_count.input_rd     << "\n"
                << "  GB W  READ ACCESS COUNT : " << gb_access_count.weight_rd    << "\n"
                << "  GB O  READ ACCESS COUNT : " << gb_access_count.output_rd << "\n"
                << "  GB O WRITE ACCESS COUNT : " << gb_access_count.output_wt << "\n";
    }
    std::cout << "DRAM I  READ ACCESS COUNT : " << dram_access_count.input_rd     << "\n"
              << "DRAM W  READ ACCESS COUNT : " << dram_access_count.weight_rd    << "\n"
              << "DRAM O  READ ACCESS COUNT : " << dram_access_count.output_rd << "\n"
              << "DRAM O WRITE ACCESS COUNT : " << dram_access_count.output_wt << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "     NUM ACTIVE MAC ARRAY : " << num_active_macs << std::endl;
    std::cout << "      NUM ACTIVE PE ARRAY : " << num_active_pes << std::endl;
    std::cout << "   NUM ACTIVE MULTI CHIPS : " << num_active_chips << std::endl;
    std::cout << "**************************" << std::endl;
    if(is_lb_shared) {
        // Print utilization as shared format
        shared_tile_size = 0;
        shared_tile_size += lb_bypass.input  ? 0 : lb_tile_size_alloc.input;
        shared_tile_size += lb_bypass.weight ? 0 : lb_tile_size_alloc.weight;
        shared_tile_size += lb_bypass.output ? 0 : lb_tile_size_alloc.output;
        std::cout << " LOCAL BUFFER UTILIZATION : " 
                << shared_tile_size / (lb_capacity.shared) * 100
                << std::endl;
    }
    else {
        if(!lb_bypass.input) {
            std::cout << "  LB I BUFFER UTILIZATION : "
                      << (float)lb_tile_size_alloc.input / lb_capacity.input  * 100 
                      << std::endl;
        }
        if(!lb_bypass.weight) {
            std::cout << "  LB W BUFFER UTILIZATION : " 
                      << (float)lb_tile_size_alloc.weight/ lb_capacity.weight * 100 
                      << std::endl;
        }
        if(!lb_bypass.output) {
            std::cout << "  LB O BUFFER UTILIZATION : " 
                      << (float)lb_tile_size_alloc.output/ lb_capacity.output * 100 
                      << std::endl;
        }
    }
    if(is_gb_exist) {
        if(is_gb_shared) {
            // Print utilization as shared format
            shared_tile_size = 0;
            shared_tile_size += gb_bypass.input  ? 0 : gb_tile_size_alloc.input;
            shared_tile_size += gb_bypass.weight ? 0 : gb_tile_size_alloc.weight;
            shared_tile_size += gb_bypass.output ? 0 : gb_tile_size_alloc.output;
            std::cout << "GLOBAL BUFFER UTILIZATION : " 
                    << shared_tile_size / (gb_capacity.shared) * 100
                    << std::endl;
        }
        else {
            if(!gb_bypass.input) {
                std::cout << "  GB I BUFFER UTILIZATION : " 
                          << (float)gb_tile_size_alloc.input / gb_capacity.input * 100 
                          << std::endl;
            }
            if(!gb_bypass.weight) {
                std::cout << "  GB W BUFFER UTILIZATION : " 
                          << (float)gb_tile_size_alloc.weight/ gb_capacity.weight * 100 
                          << std::endl;
            }
            if(!gb_bypass.output) {
                std::cout << "  GB O BUFFER UTILIZATION : " 
                          << (float)gb_tile_size_alloc.output/ gb_capacity.output * 100 
                          << std::endl;
            }
        }
    }
    std::cout << "**************************" << std::endl;
    std::cout << "    MAC ARRAY UTILIZAITON : " << (float)num_active_macs / (macs_capacity.dim_x * macs_capacity.dim_y) * 100 << std::endl;
    std::cout << "     PE ARRAY UTILIZATION : " << (float)num_active_pes / (pes_capacity.dim_x * pes_capacity.dim_y) * 100 << std::endl;
    std::cout << "  MULTI CHIPS UTILIZATION : " << (float)num_active_chips / (chips_capacity.dim_x * chips_capacity.dim_y) * 100 << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "       MAC DYNAMIC ENERGY : " << mac_energy           << "\n"
              << "        LB DYNAMIC ENERGY : " << local_buffer_energy  << "\n";
    if(is_gb_exist) {
        std::cout << "        GB DYNAMIC ENERGY : " << global_buffer_energy << "\n";
    }
    std::cout << "      DRAM DYNAMIC ENERGY : " << dram_energy          << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "        MAC STATIC ENERGY : " << mac_static           << "\n"
              << "         LB STATIC ENERGY : " << local_buffer_static  << "\n";
    if(is_gb_exist) {
        std::cout << "         GB STATIC ENERGY : " << global_buffer_static << "\n";
    }
    std::cout << "       DRAM STATIC ENERGY : " << dram_static          << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "               MAC ENERGY : " << mac_energy + mac_static << "\n"
              << "                LB ENERGY : " << local_buffer_energy + local_buffer_static << "\n";
    if(is_gb_exist) {
        std::cout << "                GB ENERGY : " << global_buffer_energy + global_buffer_static << "\n";
    }
    std::cout << "              DRAM ENERGY : " << dram_energy  + dram_static << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "                MAC CYCLE : " << mac_cycle              << "\n"
              << "                 LB CYCLE : " << local_buffer_cycle     << "\n";
    if(is_gb_exist) {
        std::cout << "                 GB CYCLE : " << global_buffer_cycle    << "\n";
    }
    std::cout << "               DRAM CYCLE : " << dram_cycle             << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "     TOTAL DYNAMIC ENERGY : " << total_energy << std::endl;
    std::cout << "      TOTAL STATIC ENERGY : " << total_static_energy << std::endl;
    std::cout << "             TOTAL ENERGY : " << total_energy + total_static_energy << std::endl;
    std::cout << "              TOTAL CYCLE : " << total_cycle            << std::endl;
    std::cout << "**************************" << std::endl;
    return;
}
// Print analyze result to output file
void analyzer_t::print_results(std::ofstream &output_file_) {
    float shared_tile_size = 0;
    update_bypassed_data();
    output_file_ << "[Scheduling Table]"<< std::endl;
    scheduling_table.print_stats(output_file_);
    output_file_ << "[Analyze Results]"<< std::endl;
    // TRANSFERED TILE SIZE 
    //output_file_ << "**************************" << std::endl;
    //output_file_ << "  LB I  transfer tile size : " << lb_tile_size_send.input    << "\n"
                 //<< "  LB W  transfer tile size : " << lb_tile_size_send.weight   << "\n"
                 //<< "  LB O  transfer tile size : " << lb_tile_size_send.output   << "\n";
    //if(is_gb_exist) {
        //output_file_ << "  GB I  transfer tile size : " << gb_tile_size_send.input    << "\n"
                     //<< "  GB W  transfer tile size : " << gb_tile_size_send.weight   << "\n"
                     //<< "  GB O  transfer tile size : " << gb_tile_size_send.output   << "\n";
    //}
    //output_file_ << "DRAM I  transfer tile size : " << dram_tile_size_send.input  << "\n"
                 //<< "DRAM W  transfer tile size : " << dram_tile_size_send.weight << "\n"
                 //<< "DRAM O  transfer tile size : " << dram_tile_size_send.output << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "           LB I tile size : " << lb_tile_size_alloc.input    << "\n"
                 << "           LB W tile size : " << lb_tile_size_alloc.weight   << "\n"
                 << "           LB O tile size : " << lb_tile_size_alloc.output   << "\n";
    if(is_gb_exist) {
        output_file_ << "           GB I tile size : " << gb_tile_size_alloc.input    << "\n"
                     << "           GB W tile size : " << gb_tile_size_alloc.weight   << "\n"
                     << "           GB O tile size : " << gb_tile_size_alloc.output   << "\n";
    }
    output_file_ << "         DRAM I tile size : " << dram_tile_size_alloc.input  << "\n"
                 << "         DRAM W tile size : " << dram_tile_size_alloc.weight << "\n"
                 << "         DRAM O tile size : " << dram_tile_size_alloc.output << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "  LB I  read access count : " << lb_access_count.input_rd     << "\n"
                 << "  LB W  read access count : " << lb_access_count.weight_rd    << "\n"
                 << "  LB O  read access count : " << lb_access_count.output_rd << "\n"
                 << "  LB O write access count : " << lb_access_count.output_wt << "\n";
    if(is_gb_exist) {
        output_file_ << "  GB I  read access count : " << gb_access_count.input_rd     << "\n"
                     << "  GB W  read access count : " << gb_access_count.weight_rd    << "\n"
                     << "  GB O  read access count : " << gb_access_count.output_rd << "\n"
                     << "  GB O write access count : " << gb_access_count.output_wt << "\n";
    }
    output_file_ << "DRAM I  read access count : " << dram_access_count.input_rd     << "\n"
                 << "DRAM W  read access count : " << dram_access_count.weight_rd    << "\n"
                 << "DRAM O  read access count : " << dram_access_count.output_rd << "\n"
                 << "DRAM O write access count : " << dram_access_count.output_wt << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "     NUM ACTIVE MAC ARRAY : " << num_active_macs << std::endl;
    output_file_ << "      NUM ACTIVE PE ARRAY : " << num_active_pes << std::endl;
    output_file_ << "   NUM ACTIVE MULTI CHIPS : " << num_active_chips << std::endl;
    output_file_ << "**************************" << std::endl;
    if(is_lb_shared) {
        // Print utilization as shared format
        shared_tile_size = 0;
        shared_tile_size += lb_bypass.input  ? 0 : lb_tile_size_alloc.input;
        shared_tile_size += lb_bypass.weight ? 0 : lb_tile_size_alloc.weight;
        shared_tile_size += lb_bypass.output ? 0 : lb_tile_size_alloc.output;
        output_file_ << " LOCAL BUFFER UTILIZATION : " 
                     << shared_tile_size / (lb_capacity.shared) * 100
                     << std::endl;
    }
    else {
        if(!lb_bypass.input) {
            output_file_ << "  LB I BUFFER UTILIZATION : "
                         << (float)lb_tile_size_alloc.input / lb_capacity.input  * 100 
                         << std::endl;
        }
        if(!lb_bypass.weight) {
            output_file_ << "  LB W BUFFER UTILIZATION : " 
                         << (float)lb_tile_size_alloc.weight/ lb_capacity.weight * 100 
                         << std::endl;
        }
        if(!lb_bypass.output) {
            output_file_ << "  LB O BUFFER UTILIZATION : " 
                         << (float)lb_tile_size_alloc.output/ lb_capacity.output * 100 
                         << std::endl;
        }
    }
    if(is_gb_exist) {
        if(is_gb_shared) {
            // Print utilization as shared format
            shared_tile_size = 0;
            shared_tile_size += gb_bypass.input  ? 0 : gb_tile_size_alloc.input;
            shared_tile_size += gb_bypass.weight ? 0 : gb_tile_size_alloc.weight;
            shared_tile_size += gb_bypass.output ? 0 : gb_tile_size_alloc.output;
            output_file_ << "GLOBAL BUFFER UTILIZATION : " 
                         << shared_tile_size / (gb_capacity.shared) * 100
                         << std::endl;
        }
        else {
            if(!gb_bypass.input) {
                output_file_ << "  GB I BUFFER UTILIZATION : " 
                             << (float)gb_tile_size_alloc.input / gb_capacity.input * 100 
                             << std::endl;
            }
            if(!gb_bypass.weight) {
                output_file_ << "  GB W BUFFER UTILIZATION : " 
                             << (float)gb_tile_size_alloc.weight/ gb_capacity.weight * 100 
                             << std::endl;
            }
            if(!gb_bypass.output) {
                output_file_ << "  GB O BUFFER UTILIZATION : " 
                             << (float)gb_tile_size_alloc.output/ gb_capacity.output * 100 
                             << std::endl;
            }
        }
    }
    output_file_ << "    MAC ARRAY UTILIZAITON : " << (float)num_active_macs / (macs_capacity.dim_x * macs_capacity.dim_y) * 100 << std::endl;
    output_file_ << "     PE ARRAY UTILIZATION : " << (float)num_active_pes / (pes_capacity.dim_x * pes_capacity.dim_y) * 100 << std::endl;
    output_file_ << "  MULTI CHIPS UTILIZATION : " << (float)num_active_chips / (chips_capacity.dim_x * chips_capacity.dim_y) * 100 << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "       MAC DYNAMIC ENERGY : " << mac_energy           << "\n"
                 << "        LB DYNAMIC ENERGY : " << local_buffer_energy  << "\n";
    if(is_gb_exist) {
        output_file_ << "        GB DYNAMIC ENERGY : " << global_buffer_energy << "\n";
    }
    output_file_ << "      DRAM DYNAMIC ENERGY : " << dram_energy          << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "        MAC STATIC ENERGY : " << mac_static           << "\n"
                 << "         LB STATIC ENERGY : " << local_buffer_static  << "\n";
    if(is_gb_exist) {
        output_file_ << "         GB STATIC ENERGY : " << global_buffer_static << "\n";
    }
    output_file_ << "       DRAM STATIC ENERGY : " << dram_static          << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "               MAC ENERGY : " << mac_energy + mac_static << "\n"
                 << "                LB ENERGY : " << local_buffer_energy + local_buffer_static << "\n";
    if(is_gb_exist) {
        output_file_ << "                GB ENERGY : " << global_buffer_energy + global_buffer_static << "\n";
    }
    output_file_ << "              DRAM ENERGY : " << dram_energy  + dram_static << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "                MAC CYCLE : " << mac_cycle              << "\n"
                 << "                 LB CYCLE : " << local_buffer_cycle     << "\n";
    if(is_gb_exist) {
        output_file_ << "                 GB CYCLE : " << global_buffer_cycle    << "\n";
    }
    output_file_ << "               DRAM CYCLE : " << dram_cycle             << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "     TOTAL DYNAMIC ENERGY : " << total_energy << std::endl;
    output_file_ << "      TOTAL STATIC ENERGY : " << total_static_energy << std::endl;
    output_file_ << "             TOTAL ENERGY : " << total_energy + total_static_energy << std::endl;
    output_file_ << "              TOTAL CYCLE : " << total_cycle            << std::endl;
    output_file_ << "**************************" << std::endl;
    return;
}

float analyzer_t::get_total_cost(metric_t  metric_) {
    float rtn = 0;
    switch(metric_) {
        case metric_t::ENERGY:
            rtn = total_energy + total_static_energy;
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
    // Find spatial component level that placed above DRAM
    rtn *= chips_actived.dim_x * chips_actived.dim_y;
    return rtn;
}
unsigned analyzer_t::get_tile_size(component_t comp_, data_t data_type_) {
    unsigned rtn = 1;
    switch (data_type_) {
    case data_t::INPUT:
        rtn = dram_tile_size_send.input; 
        break;
    case data_t::WEIGHT:
        rtn = dram_tile_size_send.weight; 
        break;
    case data_t::OUTPUT:
        rtn = dram_tile_size_send.output; 
        break;
    default:
        break;
    }
    return rtn;
}
unsigned analyzer_t::get_access_count(component_t comp_, data_t data_type_) {
    unsigned rtn = 1;
    std::vector<unsigned> access_count;
    if(comp_ == component_t::DRAM) {
        switch (data_type_) {
        case data_t::INPUT:
            rtn = dram_access_count.input_rd; 
            break;
        case data_t::WEIGHT:
            rtn = dram_access_count.weight_rd; 
            break;
        case data_t::OUTPUT:
            rtn = dram_access_count.output_rd; 
            break;
        default:
            break;
        }
    }
    else if(comp_ == component_t::GB) {
        switch (data_type_) {
        case data_t::INPUT:
            rtn = gb_access_count.input_rd; 
            break;
        case data_t::WEIGHT:
            rtn = gb_access_count.weight_rd; 
            break;
        case data_t::OUTPUT:
            rtn = gb_access_count.output_rd; 
            break;
        default:
            break;
        }
    }
    else if(comp_ == component_t::LB) {
        switch (data_type_) {
        case data_t::INPUT:
            rtn = lb_access_count.input_rd; 
            break;
        case data_t::WEIGHT:
            rtn = lb_access_count.weight_rd; 
            break;
        case data_t::OUTPUT:
            rtn = lb_access_count.output_rd; 
            break;
        default:
            break;
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

// Handle exception case where given dataflow is ineffective 
dataflow_t analyzer_t::handle_dataflow_exception_case(unsigned idx_, dataflow_t df_, bypass_t bp_) {
    dataflow_t dataflow = df_;
    bypass_t bypass = bp_;
    // if target component does not exist, return NONE
    if(dataflow == dataflow_t::NONE) { return df_;}
    if(update_irrelevant_mapping_value(idx_, (data_t)((unsigned)df_-1)) == 1) {
        switch(dataflow) {
            case dataflow_t::IS:
                if(bypass.weight) { dataflow = dataflow_t::OS; break; }
                if(bypass.output) { dataflow = dataflow_t::WS; break; }
                if(update_irrelevant_mapping_value(idx_, data_t::WEIGHT) 
                 > update_irrelevant_mapping_value(idx_, data_t::OUTPUT)) {
                    dataflow = dataflow_t::WS;
                }
                else {
                    dataflow = dataflow_t::OS;
                }
                break;
            case dataflow_t::WS:
                if(bypass.input)  { dataflow = dataflow_t::OS; break; }
                if(bypass.output) { dataflow = dataflow_t::IS; break; }
                if(update_irrelevant_mapping_value(idx_, data_t::INPUT) 
                 > update_irrelevant_mapping_value(idx_, data_t::OUTPUT)) {
                    dataflow = dataflow_t::IS;
                }
                else {
                    dataflow = dataflow_t::OS;
                }
                break;
            case dataflow_t::OS:
                if(bypass.input)  { dataflow = dataflow_t::WS; break; }
                if(bypass.weight) { dataflow = dataflow_t::IS; break; }
                if(update_irrelevant_mapping_value(idx_, data_t::INPUT) 
                 > update_irrelevant_mapping_value(idx_, data_t::WEIGHT)) {
                    dataflow = dataflow_t::IS;
                }
                else {
                    dataflow = dataflow_t::WS;
                }
                break;
            default:
                break;
        }
    }
    return dataflow;
}

void analyzer_t::init_mac_array() {
    // Init MAC unit
    mac_unit_energy     = accelerator->get_mac_energy();
    mac_unit_static     = accelerator->get_mac_static_power();
    mac_unit_cycle      = 1;
    
    // Init MAC array
    macs_capacity.dim_x = accelerator->get_mac_array_size(dimension_t::DIM_X); 
    macs_capacity.dim_y = accelerator->get_mac_array_size(dimension_t::DIM_Y);
}
void analyzer_t::init_local_buffer() {
    // Init unit access cost
    bool*  arr_lb_bypass   = accelerator->get_bypass(component_t::LB);
    float* arr_unit_energy  = accelerator->get_energy(component_t::LB);
    float* arr_unit_static  = accelerator->get_static(component_t::LB);
    float* arr_unit_cycle   = accelerator->get_cycle(component_t::LB);
    lb_unit_energy.input    = arr_unit_energy[(unsigned)data_t::INPUT];
    lb_unit_energy.weight   = arr_unit_energy[(unsigned)data_t::WEIGHT];
    lb_unit_energy.output   = arr_unit_energy[(unsigned)data_t::OUTPUT];
    lb_unit_static.input    = arr_unit_static[(unsigned)data_t::INPUT];
    lb_unit_static.weight   = arr_unit_static[(unsigned)data_t::WEIGHT];
    lb_unit_static.output   = arr_unit_static[(unsigned)data_t::OUTPUT];
    lb_unit_cycle.input     =  arr_unit_cycle[(unsigned)data_t::INPUT];
    lb_unit_cycle.weight    =  arr_unit_cycle[(unsigned)data_t::WEIGHT];
    lb_unit_cycle.output    =  arr_unit_cycle[(unsigned)data_t::OUTPUT];

    // Init buffer size
    std::vector<float> arr_buffer_size = accelerator->get_size(component_t::LB);
    // If shared
    if(arr_buffer_size.size() == 1) {
        is_lb_shared = true;
        lb_capacity.shared = arr_buffer_size.back() / (accelerator->get_precision() / BYTE);
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
    if(arr_lb_bypass != nullptr) {
        if(arr_lb_bypass[(unsigned)data_t::INPUT]) { lb_bypass.input = true; }
        if(arr_lb_bypass[(unsigned)data_t::WEIGHT]) { lb_bypass.weight = true; }  
        if(arr_lb_bypass[(unsigned)data_t::OUTPUT]) { lb_bypass.output = true; }  
    }
    
    // Init bitwidth
    lb_bitwidth = accelerator->get_bitwidth(component_t::LB) 
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
    bool*  arr_gb_bypass   = accelerator->get_bypass(component_t::GB);
    float* arr_unit_energy = accelerator->get_energy(component_t::GB);
    float* arr_unit_static = accelerator->get_static(component_t::GB);
    float* arr_unit_cycle  = accelerator->get_cycle(component_t::GB);
    if(arr_unit_energy != nullptr) {
        gb_unit_energy.input    = arr_unit_energy[(unsigned)data_t::INPUT];
        gb_unit_energy.weight   = arr_unit_energy[(unsigned)data_t::WEIGHT];
        gb_unit_energy.output   = arr_unit_energy[(unsigned)data_t::OUTPUT];
    }
    else {
        is_gb_exist = false;
    }
    if(arr_unit_static != nullptr) {
        gb_unit_static.input    = arr_unit_static[(unsigned)data_t::INPUT];
        gb_unit_static.weight   = arr_unit_static[(unsigned)data_t::WEIGHT];
        gb_unit_static.output   = arr_unit_static[(unsigned)data_t::OUTPUT];
    }
    if(arr_unit_cycle != nullptr) {
        gb_unit_cycle.input     =  arr_unit_cycle[(unsigned)data_t::INPUT];
        gb_unit_cycle.weight    =  arr_unit_cycle[(unsigned)data_t::WEIGHT];
        gb_unit_cycle.output    =  arr_unit_cycle[(unsigned)data_t::OUTPUT];
    }
    // Init buffer size
    std::vector<float> arr_buffer_size = accelerator->get_size(component_t::GB);
    // If shared
    if(arr_buffer_size.size() == 1) {
        is_gb_shared = true;
        gb_capacity.shared = arr_buffer_size.back() / (accelerator->get_precision() / BYTE);
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
    if(arr_gb_bypass != nullptr) {
        if(arr_gb_bypass[(unsigned)data_t::INPUT]) { gb_bypass.input = true; }
        if(arr_gb_bypass[(unsigned)data_t::WEIGHT]) { gb_bypass.weight = true; }  
        if(arr_gb_bypass[(unsigned)data_t::OUTPUT]) { gb_bypass.output = true; }  
    }
    // Init bitwidth
    if(is_gb_exist) {
        gb_bitwidth = accelerator->get_bitwidth(component_t::GB) 
                / accelerator->get_precision();
    }
    else { gb_bitwidth = 1; }
}
void analyzer_t::init_multi_chips() {
    // Init Multi chip 
    chips_capacity.dim_x = accelerator->get_multi_chips_size(dimension_t::DIM_X);
    chips_capacity.dim_y = accelerator->get_multi_chips_size(dimension_t::DIM_Y);
}
void analyzer_t::init_dram() {
    // Init unit access cost
    float* arr_unit_energy = accelerator->get_energy(component_t::DRAM);
    float* arr_unit_static = accelerator->get_static(component_t::DRAM);
    float* arr_unit_cycle  = accelerator->get_cycle(component_t::DRAM);
    dram_unit_energy.input  = arr_unit_energy[(unsigned)data_t::INPUT];
    dram_unit_energy.weight = arr_unit_energy[(unsigned)data_t::WEIGHT];
    dram_unit_energy.output = arr_unit_energy[(unsigned)data_t::OUTPUT];
    dram_unit_static.input  = arr_unit_static[(unsigned)data_t::INPUT];
    dram_unit_static.weight = arr_unit_static[(unsigned)data_t::WEIGHT];
    dram_unit_static.output = arr_unit_static[(unsigned)data_t::OUTPUT];
    dram_unit_cycle.input   = arr_unit_cycle[(unsigned)data_t::INPUT];
    dram_unit_cycle.weight  = arr_unit_cycle[(unsigned)data_t::WEIGHT];
    dram_unit_cycle.output  = arr_unit_cycle[(unsigned)data_t::OUTPUT];
    // Init bitwidth
    dram_bitwidth = accelerator->get_bitwidth(component_t::DRAM) 
                   / accelerator->get_precision();
}
void analyzer_t::update_bypassed_data() {
    // Neat access counts and tile size for bypassed data.
    // It is just to prevent misunderstanding of the result file by users when 
    // result reports that bypassed data types are allocated in a specific buffer level.
    // Since unit access costs are set to zero, the access count and allocated 
    // tile of the bypassed data type do not affect the accelerator's cost.
    if(lb_bypass.input && lb_unit_energy.input == 0) {
        lb_tile_size_alloc.input  = 0;
        lb_tile_size_send.input   = 0;
        lb_access_count.input_rd  = 0;
    }
    if(lb_bypass.weight && lb_unit_energy.weight == 0) {
        lb_tile_size_alloc.input  = 0;
        lb_tile_size_alloc.weight = 0;
        lb_tile_size_send.weight  = 0;
        lb_access_count.weight_rd = 0;
    }

    if(lb_bypass.output && lb_unit_energy.output == 0) {
        lb_tile_size_alloc.output = 0;
        lb_tile_size_send.output  = 0;
        lb_access_count.output_rd = 0;
        lb_access_count.output_wt = 0;
    }

    if(gb_bypass.input && gb_unit_energy.input == 0) {
        gb_tile_size_alloc.input  = 0;
        gb_tile_size_send.input   = 0;
        gb_access_count.input_rd  = 0;
    }

    if(gb_bypass.weight && gb_unit_energy.weight == 0) {
        gb_tile_size_alloc.weight = 0;
        gb_tile_size_send.weight  = 0;
        gb_access_count.weight_rd = 0;
    }

    if(gb_bypass.output && gb_unit_energy.output == 0) {
        gb_tile_size_alloc.output = 0;
        gb_tile_size_send.output  = 0;
        gb_access_count.output_rd = 0;
        gb_access_count.output_wt = 0;
    }
}
