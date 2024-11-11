#include <cassert>
#include <climits>
#include <cmath>
#include <iomanip>

#include "analyzer.h"
#define BYTE 8

// Constructor for analyzer
analyzer_t::analyzer_t(const std::string& accelerator_pth_,
                       const std::string& dataflow_,
                       const std::string& network_pth_,
                       const std::string& batch_size_,
                       const std::string& scheduling_table_pth_) 
    : accelerator(new accelerator_t(accelerator_pth_)),
      network(new network_t(network_pth_)) {
        network->init_network();
        if(batch_size_ != "1") { network->update_batch_size(batch_size_); }
        scheduling_table = scheduling_table_t(accelerator, 
                                              network,
                                              scheduling_table_pth_);
        if(!dataflow_.empty() ) {
            // Convert string value to dataflow set
            std::vector<std::string> string_dataflow = split(dataflow_, '-');
            std::vector<dataflow_t>  target_dataflow;

            // Change the table's dataflow set to the specified dataflow set
            for(unsigned i = 0; i < string_dataflow.size(); ++i) {
                std::string df_string = lowercase(string_dataflow.at(i));
                dataflow_t  df = (dataflow_t)get_enum_type(dataflow_str, df_string);
                if(df > dataflow_t::SIZE) {
                    std::cerr << "Invalid dataflow setting; valid setting example (ws-os)" << std::endl;
                    exit(0);
                }
                target_dataflow.push_back(df);
            }
            scheduling_table.update_dataflow(target_dataflow);
        }
        init_mac_array();
        init_local_buffer();
        init_pe_array();
        init_global_buffer();
        init_multi_chips();
        init_dram();

        dram_idx = (unsigned)component_t::DRAM;
        gb_idx   = (unsigned)component_t::GB;
        lb_idx   = (unsigned)component_t::LB;
        
        // Print accelerator spec.
        accelerator->print_spec();
}
// Constructor for optimizer
analyzer_t::analyzer_t(const accelerator_t *accelerator_,
                       const network_t    *network_)
    :accelerator(new accelerator_t(*accelerator_)),
     network(new network_t(*network_)) {
        init_mac_array();
        init_local_buffer();
        init_pe_array();
        init_global_buffer();
        init_multi_chips();
        init_dram();
}
analyzer_t::~analyzer_t() {
    // Delete only when analyzer allocate the objects dynamically
    if (accelerator != nullptr && network != nullptr) {
        delete accelerator;
        delete network;
    }
}
// Run analyzer 
void analyzer_t::run() {
    update_tile_size();
    update_active_components();
    // Check scheduling table's validity
    if(!verify_constraints()) {
        std::cerr << "Invalid scheduling table" << std::endl;
        exit(0);
    }
    update_access_count();
    // Estimate accelerator cost
    estimate_cost();
    // Print out analysis results
    print_results();
}
// Initialize analyzer when it constructed with scheduling table
void analyzer_t::init() {
    scheduling_table.print_stats();
    dram_idx = scheduling_table.get_num_rows() - 1;
    gb_idx = (unsigned)component_t::GB;
    lb_idx = (unsigned)component_t::LB;
    rg_idx = (unsigned)component_t::REG;
    update_tile_size();
    update_active_components();
    // Check scheduling table's validity
    if(!verify_constraints()) {
        std::cerr << "Invalid scheduling table" << std::endl;
        exit(0);
    }
    update_access_count();
    return;
}
// Initialize analyzer to evaluate a given scheduling table
void analyzer_t::init(scheduling_table_t scheduling_table_) {
    // Copy scheduling table
    scheduling_table = scheduling_table_;
    dram_idx = scheduling_table.get_num_rows() - 1;
    gb_idx = (unsigned)component_t::GB;
    lb_idx = (unsigned)component_t::LB;
    rg_idx = (unsigned)component_t::REG;
    update_tile_size();
    update_access_count();
    update_active_components();
    return;
}
// Check scheduling table's validity
bool analyzer_t::verify_constraints() {
    bool rtn = true;
    // Traverse all rows and check hardware constraints. 
    rtn = rtn && verify_spatial_constraints();
    if(!rtn) { return rtn; }
    rtn = rtn && verify_temporal_constraints();
    if(!rtn) { return rtn; }
    return rtn;
}
// Check scheduling table's spatial validity
bool analyzer_t::verify_spatial_constraints() {
    // Traverse spatial rows and check hardware constraints. 
    if(scheduling_table.get_above_buffer_pos(lb_idx) != lb_idx) {
        if(scheduling_table.get_row_product(0) != 1) { return false; }
    }
    // Check MAC array validity
    if(macs_activated.dim_x > macs_capacity.dim_x 
    || macs_activated.dim_y > macs_capacity.dim_y) {
        return false;
    }
    if(!macs_constraint.empty) {
        if(!check_user_constraint(component_t::MAC_X, macs_constraint)) {
            return false;
        }
    }
    // Check PE array validity
    if(pes_activated.dim_x > pes_capacity.dim_x 
    || pes_activated.dim_y > pes_capacity.dim_y) {
        return false;
    }
    if(!pes_constraint.empty) {
        if(!check_user_constraint(component_t::PE_X, pes_constraint)) {
            return false;
        }
    }
    // Check Multi chips validity
    if(chips_activated.dim_x > chips_capacity.dim_x 
    || chips_activated.dim_y > chips_capacity.dim_y) {
        return false;
    }
    if(!chips_constraint.empty) {
        if(!check_user_constraint(component_t::CHIP_X, chips_constraint)) {
            return false;
        }
    }
    return true;
}
// Check scheduling table's temporal validity
bool analyzer_t::verify_temporal_constraints() {
    bool rtn = true;
    unsigned shared_tile_size = 0;
    // Traverse temporal rows and check hardware constraints. 
    // Check Local Buffer validity
    if(is_lb_exist) {
        if(is_lb_shared) {
            shared_tile_size  = lb_bypass.input  ?  0 : lb_tile_size_alloc.input;
            shared_tile_size += lb_bypass.weight ?  0 : lb_tile_size_alloc.weight;
            shared_tile_size += lb_bypass.output ?  0 : lb_tile_size_alloc.output;
            if(shared_tile_size > lb_capacity.shared ) {
                return false;
            }
        }
        else {
            // If the tile size is smaller than LB capacity or the data tile is set to bypass LB
            rtn = rtn && ((lb_tile_size_alloc.input  <= lb_capacity.input ) || lb_bypass.input);
            rtn = rtn && ((lb_tile_size_alloc.weight <= lb_capacity.weight) || lb_bypass.weight);
            rtn = rtn && ((lb_tile_size_alloc.output <= lb_capacity.output) || lb_bypass.output);
        }
    }
    // Check Global Buffer validity
    if(is_gb_exist) {
        if(is_gb_shared) {
            shared_tile_size  = (gb_separate.input  || gb_bypass.input)  ? 0 : gb_tile_size_alloc.input;
            shared_tile_size += (gb_separate.weight || gb_bypass.weight) ? 0 : gb_tile_size_alloc.weight;
            shared_tile_size += (gb_separate.output || gb_bypass.output) ? 0 : gb_tile_size_alloc.output;
            if(shared_tile_size > gb_capacity.shared ) {
                return false;
            }
            // Handles the partially shared buffer case
            if(gb_separate.input && !gb_bypass.input && 
               gb_tile_size_alloc.input > gb_capacity.input) {
                return false;
            }
            if(gb_separate.weight && !gb_bypass.weight && 
               gb_tile_size_alloc.weight > gb_capacity.weight) {
                return false;
            }
            if(gb_separate.output && !gb_bypass.output && 
               gb_tile_size_alloc.output > gb_capacity.output) {
                return false;
            }        
        }
        else {
            // Check if tile sizes exceed GB capacity when GB bypass is not enabled
            // Returns false if any non-bypassed tile size is larger than its corresponding GB capacity
            if(!gb_bypass.input && gb_tile_size_alloc.input > gb_capacity.input) {
                return false;
            }
            if(!gb_bypass.weight && gb_tile_size_alloc.weight > gb_capacity.weight) {
                return false;
            }
            if(!gb_bypass.output && gb_tile_size_alloc.output > gb_capacity.output) {
                return false;
            }
        }
    }
    if(!lb_constraint.empty()) {
        rtn = rtn && check_user_constraint(component_t::LB, lb_constraint); 
    }
    if(!gb_constraint.empty()) {
        rtn = rtn && check_user_constraint(component_t::GB, gb_constraint); 
    }
    if(!dram_constraint.empty()) {
        rtn = rtn && check_user_constraint(component_t::DRAM, dram_constraint); 
    }
    return rtn;
}
bool analyzer_t::check_user_constraint(component_t comp_, arr_cnst_t cnst_) {
    bool     rtn          = true;
    unsigned row_product  = 1;
    std::vector<unsigned> mapping_values;
    if(!cnst_.dim_x.empty()) {
        // Get row values from scheduling table
        mapping_values = scheduling_table.get_row_values((unsigned)comp_);
        row_product    = scheduling_table.get_row_product((unsigned)comp_);
        for(auto it = cnst_.dim_x.begin(); it != cnst_.dim_x.end(); ++it) {
            row_product /= mapping_values[(unsigned)(parameter_t)get_enum_type(parameter_str, lowercase(*it))];

        }
        // If optimizer allocate mapping values in unspecificed parameter, the product value should be larger than one.
        if(row_product > 1) { 
            return false;
        }
    }
    if(!cnst_.dim_y.empty()) {
        // Change to Y-dim component
        comp_ = (component_t)(((unsigned)comp_)+1);
        // Get row values from scheduling table
        mapping_values = scheduling_table.get_row_values((unsigned)comp_);
        row_product    = scheduling_table.get_row_product((unsigned)comp_);
        for(auto it = cnst_.dim_y.begin(); it != cnst_.dim_y.end(); ++it) {
            row_product /= mapping_values[(unsigned)(parameter_t)get_enum_type(parameter_str, lowercase(*it))];
        }
        if(row_product > 1) {
            return false;
        }
    }
    return rtn;
}
bool analyzer_t::check_user_constraint(component_t comp_, std::vector<unsigned> cnst_) {
    bool rtn = true;
    std::vector<unsigned> mapping_values = scheduling_table.get_row_values((unsigned)comp_);
    if(!cnst_.empty()) {
        for(unsigned i = 0; i < (unsigned)parameter_t::SIZE; i++) {
            if(cnst_.at(i) != 0) { 
                // Compare mapping value and constraint value
                if(cnst_.at(i) != mapping_values.at(i)) {
                    rtn = false;
                    break;
                }
            }
        }
    }
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
    table_row_t values;
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
    values = scheduling_table.get_row_wise_product(0, buffer_idx);
    // Compute input height & width
    unsigned input_height = 1;
    unsigned input_width  = 1;
    unsigned layer_idx = scheduling_table.get_layer_index();
    unsigned stride = network->get_stride(layer_idx);
    unsigned stride_h = 1;
    unsigned stride_w = 1;

    // Determine stride for height and width
    if(values.at((unsigned)parameter_t::R) != 1) { stride_h = stride; }
    if(values.at((unsigned)parameter_t::S) != 1) { stride_w = stride; }
    
    // Calculate input height and width
    input_height = (values.at((unsigned)parameter_t::P) - 1) * stride_h
                 + values.at((unsigned)parameter_t::R);
    input_width  = (values.at((unsigned)parameter_t::Q) - 1) * stride_w
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
    table_row_t values;
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
    values = scheduling_table.get_row_wise_product(0, buffer_idx);
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
    table_row_t values;
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
    values = scheduling_table.get_row_wise_product(0, buffer_idx);
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
    dataflow = scheduling_table.get_dataflow(rg_idx);
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
    // Unlike GLB bypass where the bypassed data can be stored in LB,
    // LB bypass requires special handling since register cannot store data (as the neurospector supposed that).
    if(lb_bypass.input) {
        gb_access_count.input_rd *= get_irrelevant_mapping_value((unsigned)component_t::LB, data_t::INPUT);
    }
    else if(lb_bypass.weight) {
        gb_access_count.weight_rd *= get_irrelevant_mapping_value((unsigned)component_t::LB, data_t::WEIGHT);
    }
    else if(lb_bypass.output) {
        // Get irrelevant mapping values for handling partial sum accumulation
        // C, R, S dimensions for each memory level that affect output accumulation
        unsigned irr_lb_mapping = get_irrelevant_mapping_value((unsigned)component_t::LB, data_t::OUTPUT);
        unsigned irr_glb_mapping = get_irrelevant_mapping_value((unsigned)component_t::GB, data_t::OUTPUT);
        unsigned irr_dram_mapping = get_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::OUTPUT);

        // If LB would have accumulated partial sums (irr_lb_mapping > 1)
        if(irr_lb_mapping > 1) {
            // If the local buffer holds more than 1 of C, R, S values,
            // the hardware that bypasses output should iteratively read/write the partial sums to GLB
            gb_access_count.output_wt *= irr_lb_mapping;

            // Handle read access count for partial sum accumulation
            // Case `output rd non-exist`: re-calculate the read count
            if(gb_access_count.output_rd == 0) {
                gb_access_count.output_rd = 1;
                // When no irrelevant mappings in GLB and DRAM
                // i.e., psum accumulation only exist in LB level
                if(irr_glb_mapping == 1 && irr_dram_mapping == 1) {
                    // Need (irr_lb_mapping - 1) reads for accumulation
                    gb_access_count.output_rd *= irr_lb_mapping - 1;
                    // Consider relevant mappings in GLB and DRAM for total access pattern
                    gb_access_count.output_rd *= get_relevant_mapping_value((unsigned)component_t::GB, data_t::OUTPUT);
                    gb_access_count.output_rd *= get_relevant_mapping_value((unsigned)component_t::DRAM, data_t::OUTPUT);
                }
            }
            // Case `output rd exist`: adjust existing read count considering all memory level mappings
            else {
                gb_access_count.output_rd /= (irr_glb_mapping * irr_dram_mapping - 1);
                gb_access_count.output_rd *= (irr_lb_mapping * irr_glb_mapping *irr_dram_mapping - 1);
            }
        }
    }
    // Exception case handler; a data type bypasses the global buffer but DRAM sends all the data to on-chip.
    if(gb_bypass.input && (dram_tile_size_alloc.input == dram_tile_size_send.input)
    && (gb_tile_size_alloc.input != gb_tile_size_send.input)) {
        dram_access_count.output_wt *= get_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::INPUT);
    }
    else if(gb_bypass.weight && (dram_tile_size_alloc.weight == dram_tile_size_send.weight)
    && (gb_tile_size_alloc.weight != gb_tile_size_send.weight)) {
        dram_access_count.weight_rd *= get_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::WEIGHT);
    }
    else if(gb_bypass.output && (dram_tile_size_alloc.output == dram_tile_size_send.output)
    && (gb_tile_size_alloc.output != gb_tile_size_send.output)) {
        dram_access_count.output_rd = get_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::OUTPUT) - 1;
        dram_access_count.output_wt *= get_irrelevant_mapping_value((unsigned)component_t::DRAM, data_t::OUTPUT);
    }
    return;
}
// Update tile-granular input access count
unsigned analyzer_t::update_input_access_count(unsigned idx_,
                                               unsigned value_) {
    unsigned access_count = value_;
    // Dataflow adjustment 
    if(dataflow == dataflow_t::IS) {
        access_count /= get_irrelevant_mapping_value(idx_, data_t::INPUT);
    }
    return access_count;
}
// Update tile-granular weight access count
unsigned analyzer_t::update_weight_access_count(unsigned idx_,
                                                unsigned value_) {
    unsigned access_count = value_;
    // Dataflow adjustment 
    if(dataflow == dataflow_t::WS) {
        access_count /= get_irrelevant_mapping_value(idx_, data_t::WEIGHT);
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
                access_count /= get_irrelevant_mapping_value(idx_, data_t::OUTPUT); 
            }
            break;
        default:
            break;
    }
    return access_count;
}
// Product all irrelevant mapping values with stationary data
unsigned analyzer_t::get_irrelevant_mapping_value(unsigned row_idx_,
                                                  data_t stationary_data_) {
    unsigned irrelevant_mapping_value = 1;
    
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
// Product all relevant mapping values with stationary data
unsigned analyzer_t::get_relevant_mapping_value(unsigned row_idx_,
                                                data_t stationary_data_) {
    unsigned irrelevant_mapping_value = 1;
    
    switch(stationary_data_) {
    case data_t::INPUT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::B)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::P)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::Q)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::C)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::R)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::S)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::G);
        break;
    case data_t::WEIGHT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::K)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::C)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::R)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::S)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::G);
        break;
    case data_t::OUTPUT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::K)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::B)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::P)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::Q)
            * scheduling_table.get_mapping_value(row_idx_, (unsigned)parameter_t::G);
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
        macs_activated.dim_x = 1;
        macs_activated.dim_y = 1;
    }
    else {
        macs_activated.dim_x = scheduling_table.get_row_product(lb_idx - 2);
        macs_activated.dim_y = scheduling_table.get_row_product(lb_idx - 1);
    }
    num_active_macs  = macs_activated.dim_x * macs_activated.dim_y;
    // Get num active PEs
    if(pes_capacity.dim_x * pes_capacity.dim_y == 1) {
        pes_activated.dim_x = 1;
        pes_activated.dim_y = 1;
    }
    else {
        pes_activated.dim_x = scheduling_table.get_row_product(gb_idx - 2);
        pes_activated.dim_y = scheduling_table.get_row_product(gb_idx - 1);
    }
    num_active_pes   = pes_activated.dim_x * pes_activated.dim_y;

    // Get num active Chips
    if(chips_capacity.dim_x * chips_capacity.dim_y == 1) {
        chips_activated.dim_x = 1;
        chips_activated.dim_y = 1;
    }
    else {
        chips_activated.dim_x = scheduling_table.get_row_product(dram_idx - 2);
        chips_activated.dim_y = scheduling_table.get_row_product(dram_idx - 1);
    }
    num_active_chips = chips_activated.dim_x * chips_activated.dim_y;

    // Get num clusters and there sizes
    num_clusters_mac  = scheduling_table.get_num_clusters(data_t::OUTPUT, component_t::MAC_X);
    num_clusters_pe   = scheduling_table.get_num_clusters(data_t::OUTPUT, component_t::PE_X);
    num_clusters_chip = scheduling_table.get_num_clusters(data_t::OUTPUT, component_t::CHIP_X);
    cluster_size = scheduling_table.get_cluster_size();
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
    float*   bitwidth       = accelerator->get_bitwidth(component_t::DRAM);
    float    precision      = accelerator->get_precision();
    float    datawidth      = bitwidth[0] /precision;
 
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
		reduced_cost = ceil((float)overlapped_size / datawidth)
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
// Custom ternary max operation
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
    cycle_i = std::ceil((float)lb_tile_size_send.input / lb_bitwidth.input)
            * lb_access_count.input_rd
            * lb_unit_cycle.input;
    cycle_w = std::ceil((float)lb_tile_size_send.weight / lb_bitwidth.weight)
            * lb_access_count.weight_rd
            * lb_unit_cycle.weight;
    cycle_o = std::ceil((float)lb_tile_size_send.output / lb_bitwidth.output)
            * (lb_access_count.output_rd + lb_access_count.output_wt) 
            * lb_unit_cycle.output;
    if(is_lb_shared) {
        local_buffer_cycle = cycle_i + cycle_w + cycle_o;
    }
    else {
        local_buffer_cycle = max(cycle_i, cycle_w, cycle_o);
    }
    // Get global buffer cycle
    cycle_i = std::ceil((float)gb_tile_size_send.input / gb_bitwidth.input)
            * gb_access_count.input_rd
            * gb_unit_cycle.input;
    cycle_w = std::ceil((float)gb_tile_size_send.weight / gb_bitwidth.weight)
            * gb_access_count.weight_rd
            * gb_unit_cycle.weight;
    cycle_o = std::ceil((float)gb_tile_size_send.output / gb_bitwidth.output)
            * (gb_access_count.output_rd + gb_access_count.output_wt) 
            * gb_unit_cycle.output;
    global_buffer_cycle = cycle_i + cycle_w + cycle_o;
    // Get on-chip cycle
    on_chip_cycle = mac_cycle + local_buffer_cycle + global_buffer_cycle;  
    
    // Get dram cycle
    cycle_i = std::ceil((float)dram_tile_size_send.input / dram_bitwidth)
            * dram_access_count.input_rd 
            * dram_unit_cycle.input;
    cycle_w = std::ceil((float)dram_tile_size_send.weight / dram_bitwidth)
            * dram_access_count.weight_rd 
            * dram_unit_cycle.weight;
    cycle_o = std::ceil((float)dram_tile_size_send.output / dram_bitwidth)
            * (dram_access_count.output_rd + dram_access_count.output_wt) 
            * dram_unit_cycle.output;
    dram_cycle = cycle_i + cycle_w + cycle_o;
    
    // Get total cycle
    total_cycle = on_chip_cycle + dram_cycle;
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

    lb_energy_lower  = (lb_tile_size_alloc.input  * gb_access_count.input_rd  
                     * lb_unit_energy.input)
                     + (lb_tile_size_alloc.weight * gb_access_count.weight_rd 
                     * lb_unit_energy.weight)
                     + (lb_tile_size_alloc.output * gb_access_count.output_wt
                     * lb_unit_energy.output);
    lb_energy_lower *= num_active_pes * num_active_chips;
    lb_energy_lower += lb_tile_size_alloc.output * gb_access_count.output_rd  
                     * num_clusters_pe * lb_unit_energy.output;

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
    
    gb_energy_lower  = (gb_tile_size_alloc.input  * dram_access_count.input_rd  
                     * gb_unit_energy.input)
                     + (gb_tile_size_alloc.weight * dram_access_count.weight_rd 
                     * gb_unit_energy.weight)
                     + (gb_tile_size_alloc.output * dram_access_count.output_wt
                     * gb_unit_energy.output);
    gb_energy_lower *= num_active_chips;
    gb_energy_lower += gb_tile_size_alloc.output * dram_access_count.output_rd  
                     * num_clusters_chip * gb_unit_energy.output;

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
    local_buffer_static  = lb_unit_static
                         * accelerator->get_clock_time()
                         * pes_capacity.dim_x * pes_capacity.dim_y
                         * chips_capacity.dim_x * chips_capacity.dim_y
                         * total_cycle;
    global_buffer_static = gb_unit_static
                         * accelerator->get_clock_time()
                         * chips_capacity.dim_x * chips_capacity.dim_y
                         * total_cycle;
    dram_static          = dram_unit_static
                         * accelerator->get_clock_time()
                         * total_cycle;
    total_static_energy  = mac_static + local_buffer_static + global_buffer_static + dram_static;
}
// Update accelerator's power
void analyzer_t::update_runtime_power() {
    float frequency = accelerator->get_clock_frequency();
    float mac_static = accelerator->get_mac_static_power();
    // Calculate MAC unit power
    mac_dynamic_power = (mac_energy / mac_cycle) * frequency / 1000; // uW -> mW
    mac_static_power  = (mac_static * accelerator->get_total_num_MACs());
    mac_power         = mac_dynamic_power + mac_static_power;
    // Calculate local buffer power
    local_buffer_dynamic_power = (local_buffer_energy / local_buffer_cycle) * frequency / 1000;
    local_buffer_static_power  = (lb_unit_static * accelerator->get_total_num_PEs()); 
    local_buffer_power = local_buffer_dynamic_power + local_buffer_static_power;
    // Calculate global buffer power
    global_buffer_dynamic_power = (global_buffer_energy / global_buffer_cycle) * frequency /1000;
    global_buffer_static_power  = (gb_unit_static * accelerator->get_total_num_chips()); 
    global_buffer_power = global_buffer_dynamic_power + global_buffer_static_power;
    // Calculate dram power
    dram_dynamic_power = (dram_energy / dram_cycle) * frequency / 1000;
    dram_static_power  = dram_unit_static;
    dram_power         = dram_dynamic_power + dram_static_power;
    // Calculate average power
    average_power_acc = (total_energy + total_static_energy) / total_cycle * frequency / 1000;
    average_power_on_chip = ((mac_energy + local_buffer_energy + global_buffer_energy) 
        / on_chip_cycle) * frequency / 1000;
}

// Print out analysis result
void analyzer_t::print_results() {
    float shared_tile_size = 0;
    update_bypassed_data();
    // Calculate Power value
    update_runtime_power();
    // std::cout << "[Accelerator Spec.]"<< std::endl;
    // accelerator->print_spec();
    std::cout << "[Scheduling Table]"<< std::endl;
    scheduling_table.print_stats();
    network->print_stats(scheduling_table.get_layer_index());
    std::cout << "[Analyze Results]"<< std::endl;
    // TRANSFERED TILE SIZE 
    std::cout << "**************************" << std::endl;
    std::cout << "  LB I  transfer tile size : " << lb_tile_size_send.input    << "\n"
              << "  LB W  transfer tile size : " << lb_tile_size_send.weight   << "\n"
              << "  LB O  transfer tile size : " << lb_tile_size_send.output   << "\n";
    if(is_gb_exist) {
        std::cout << "  GB I  transfer tile size : " << gb_tile_size_send.input    << "\n"
                  << "  GB W  transfer tile size : " << gb_tile_size_send.weight   << "\n"
                  << "  GB O  transfer tile size : " << gb_tile_size_send.output   << "\n";
    }
    std::cout << "DRAM I  transfer tile size : " << dram_tile_size_send.input  << "\n"
              << "DRAM W  transfer tile size : " << dram_tile_size_send.weight << "\n"
              << "DRAM O  transfer tile size : " << dram_tile_size_send.output << std::endl;
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
        std::cout << "  LB I BUFFER UTILIZATION : ";
        if(!lb_bypass.input) {
            std::cout << (float)lb_tile_size_alloc.input / lb_capacity.input  * 100 
                      << std::endl;
        }
        else { std::cout << "bypass" << std::endl; }
      
        std::cout << "  LB W BUFFER UTILIZATION : ";
        if(!lb_bypass.weight) {
            std::cout << (float)lb_tile_size_alloc.weight/ lb_capacity.weight * 100 
                      << std::endl;
        }
        else { std::cout << "bypass" << std::endl; }
    
        std::cout << "  LB O BUFFER UTILIZATION : ";
        if(!lb_bypass.output) {
            std::cout << (float)lb_tile_size_alloc.output/ lb_capacity.output * 100 
                      << std::endl;
        }
        else { std::cout << "bypass" << std::endl; }
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
            std::cout << "  GB I BUFFER UTILIZATION : ";
            if(!gb_bypass.input) {
                std::cout << (float)gb_tile_size_alloc.input / gb_capacity.input * 100 
                          << std::endl;
            }
            else { std::cout << "bypass" << std::endl; }
            
            std::cout << "  GB W BUFFER UTILIZATION : ";
            if(!gb_bypass.weight) {
                std::cout << (float)gb_tile_size_alloc.weight/ gb_capacity.weight * 100 
                          << std::endl;
            }
            else { std::cout << "bypass" << std::endl; }
            
            std::cout << "  GB O BUFFER UTILIZATION : ";
            if(!gb_bypass.output) {
                std::cout << (float)gb_tile_size_alloc.output/ gb_capacity.output * 100 
                          << std::endl;
            }
            else { std::cout << "bypass" << std::endl; }
        }
    }
    std::cout << "**************************" << std::endl;
    std::cout << "    MAC ARRAY UTILIZATION : " << (float)num_active_macs / (macs_capacity.dim_x * macs_capacity.dim_y) * 100 << std::endl;
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
    std::cout << "                MAC POWER : " << mac_power << std::endl;
    std::cout << "                 LB POWER : " << local_buffer_power  << std::endl;
    if(is_gb_exist) {
        std::cout << "                 GB POWER : " << global_buffer_power << std::endl;
    }
    std::cout << "               DRAM POWER : " << dram_power << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "     TOTAL DYNAMIC ENERGY : " << total_energy << " pJ" << std::endl;
    std::cout << "      TOTAL STATIC ENERGY : " << total_static_energy << " pJ" << std::endl;
    std::cout << "             TOTAL ENERGY : " << total_energy + total_static_energy << " pJ" << std::endl;
    std::cout << "              TOTAL CYCLE : " << total_cycle << std::endl;
    std::cout << "  AVERAGE POWER (ON-CHIP) : " << average_power_on_chip << " mW" << std::endl;
    std::cout << "   AVERAGE POWER (SYSTEM) : " << average_power_acc << " mW" << std::endl;
    std::cout << "**************************" << std::endl;
    std::cout << "Version: " << FULL_VERSION << std::endl;
    return;
}
// Print analyze result to output file
void analyzer_t::print_results(std::ofstream &output_file_) {
    float shared_tile_size = 0;
    update_bypassed_data();
    // Calculate Power value
    update_runtime_power();

    output_file_ << "[Accelerator Spec.]"<< std::endl;
    accelerator->print_spec(output_file_);
    output_file_ << "[Scheduling Table]"<< std::endl;
    scheduling_table.print_stats(output_file_);
    network->print_stats(scheduling_table.get_layer_index(), output_file_);
    output_file_ << "[Analyze Results]"<< std::endl;
    // TRANSFERED TILE SIZE 
    output_file_ << "**************************" << std::endl;
    output_file_ << "  LB I  transfer tile size : " << lb_tile_size_send.input    << "\n"
                 << "  LB W  transfer tile size : " << lb_tile_size_send.weight   << "\n"
                 << "  LB O  transfer tile size : " << lb_tile_size_send.output   << "\n";
    if(is_gb_exist) {
        output_file_ << "  GB I  transfer tile size : " << gb_tile_size_send.input    << "\n"
                     << "  GB W  transfer tile size : " << gb_tile_size_send.weight   << "\n"
                     << "  GB O  transfer tile size : " << gb_tile_size_send.output   << "\n";
    }
    output_file_ << "DRAM I  transfer tile size : " << dram_tile_size_send.input  << "\n"
                 << "DRAM W  transfer tile size : " << dram_tile_size_send.weight << "\n"
                 << "DRAM O  transfer tile size : " << dram_tile_size_send.output << std::endl;
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
        output_file_ << "  LB I BUFFER UTILIZATION : ";
        if(!lb_bypass.input) {
            output_file_ << (float)lb_tile_size_alloc.input / lb_capacity.input  * 100 
                         << std::endl;
        }
        else { output_file_ << "bypass" << std::endl; }

        output_file_ << "  LB W BUFFER UTILIZATION : ";
        if(!lb_bypass.weight) {
            output_file_ << (float)lb_tile_size_alloc.weight/ lb_capacity.weight * 100 
                         << std::endl;
        }
        else { output_file_ << "bypass" << std::endl; }

        output_file_ << "  LB O BUFFER UTILIZATION : ";
        if(!lb_bypass.output) {
            output_file_ << (float)lb_tile_size_alloc.output/ lb_capacity.output * 100 
                         << std::endl;
        }
        else { output_file_ << "bypass" << std::endl; }
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
            output_file_ << "  GB I BUFFER UTILIZATION : ";
            if(!gb_bypass.input) {
                output_file_ << (float)gb_tile_size_alloc.input / gb_capacity.input * 100 
                             << std::endl;
            }
            else { output_file_ << "bypass" << std::endl; }
           
            output_file_ << "  GB W BUFFER UTILIZATION : ";
            if(!gb_bypass.weight) {
                output_file_ << (float)gb_tile_size_alloc.weight/ gb_capacity.weight * 100 
                             << std::endl;
            }
            else { output_file_ << "bypass" << std::endl; }
            
            output_file_ << "  GB O BUFFER UTILIZATION : ";
            if(!gb_bypass.output) {
                output_file_ << (float)gb_tile_size_alloc.output/ gb_capacity.output * 100 
                             << std::endl;
            }
            else { output_file_ << "bypass" << std::endl; }
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
    output_file_ << "                MAC POWER : " << mac_power << std::endl;
    output_file_ << "                 LB POWER : " << local_buffer_power << std::endl;
    if(is_gb_exist) {
        output_file_ << "                 GB POWER : " << global_buffer_power << std::endl;
    }
    output_file_ << "               DRAM POWER : " << dram_power << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "     TOTAL DYNAMIC ENERGY : " << total_energy << " pJ" << std::endl;
    output_file_ << "      TOTAL STATIC ENERGY : " << total_static_energy << " pJ" << std::endl;
    output_file_ << "             TOTAL ENERGY : " << total_energy + total_static_energy << " pJ" << std::endl;
    output_file_ << "              TOTAL CYCLE : " << total_cycle            << std::endl;
    output_file_ << "  AVERAGE POWER (ON-CHIP) : " << average_power_on_chip << " mW" << std::endl;
    output_file_ << "   AVERAGE POWER (SYSTEM) : " << average_power_acc << " mW" << std::endl;
    output_file_ << "**************************" << std::endl;
    output_file_ << "Version: " << FULL_VERSION << std::endl;
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
            if(!is_gb_exist) {
                rtn = dram_energy + lb_energy_lower
                    + dram_static + mac_static;
            }
            else {
                rtn = dram_energy + gb_energy_lower
                    + dram_static + mac_static;
            }
        }
        else if(lower_idx == gb_idx) {
            if(macs_capacity.dim_x * macs_capacity.dim_y != 1) {
                rtn = gb_energy_upper + lb_energy_lower
                    + global_buffer_static + mac_static;
            }
            else {
                rtn = gb_energy_upper + lb_energy_lower + lb_energy_upper
                    + global_buffer_static + local_buffer_static + mac_static;
            }
        }
        else if(lower_idx == lb_idx) {
            rtn = lb_energy_upper 
                + local_buffer_static + mac_static;
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
            rtn = global_buffer_cycle + mac_cycle;
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
    rtn *= chips_activated.dim_x * chips_activated.dim_y;
    return rtn;
}
unsigned analyzer_t::get_tile_size(component_t comp_, data_t data_type_) {
    unsigned rtn = 1;
    if(comp_ == component_t::DRAM) {
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
    }
    else if(comp_ == component_t::GB) {
        switch (data_type_) {
        case data_t::INPUT:
            rtn = gb_tile_size_send.input; 
            break;
        case data_t::WEIGHT:
            rtn = gb_tile_size_send.weight; 
            break;
        case data_t::OUTPUT:
            rtn = gb_tile_size_send.output; 
            break;
        default:
            break;
        }
    }
    else if(comp_ == component_t::LB) {
        switch (data_type_) {
        case data_t::INPUT:
            rtn = lb_tile_size_send.input; 
            break;
        case data_t::WEIGHT:
            rtn = lb_tile_size_send.weight; 
            break;
        case data_t::OUTPUT:
            rtn = lb_tile_size_send.output; 
            break;
        default:
            break;
        }
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
            rtn = dram_access_count.output_rd + dram_access_count.output_wt; 
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
            rtn = gb_access_count.output_rd + gb_access_count.output_wt; 
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
            rtn = lb_access_count.output_rd + lb_access_count.output_wt; 
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
    if(get_irrelevant_mapping_value(idx_, (data_t)((unsigned)df_-1)) == 1) {
        switch(dataflow) {
            case dataflow_t::IS:
                if(bypass.weight) { dataflow = dataflow_t::OS; break; }
                if(bypass.output) { dataflow = dataflow_t::WS; break; }
                if(get_irrelevant_mapping_value(idx_, data_t::WEIGHT) 
                 > get_irrelevant_mapping_value(idx_, data_t::OUTPUT)) {
                    dataflow = dataflow_t::WS;
                }
                else {
                    dataflow = dataflow_t::OS;
                }
                break;
            case dataflow_t::WS:
                if(bypass.input)  { dataflow = dataflow_t::OS; break; }
                if(bypass.output) { dataflow = dataflow_t::IS; break; }
                if(get_irrelevant_mapping_value(idx_, data_t::INPUT) 
                 > get_irrelevant_mapping_value(idx_, data_t::OUTPUT)) {
                    dataflow = dataflow_t::IS;
                }
                else {
                    dataflow = dataflow_t::OS;
                }
                break;
            case dataflow_t::OS:
                if(bypass.input)  { dataflow = dataflow_t::WS; break; }
                if(bypass.weight) { dataflow = dataflow_t::IS; break; }
                if(get_irrelevant_mapping_value(idx_, data_t::INPUT) 
                 > get_irrelevant_mapping_value(idx_, data_t::WEIGHT)) {
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
    mac_unit_energy = accelerator->get_mac_energy();
    mac_unit_static = accelerator->get_mac_static_power();
    mac_unit_cycle  = 1;
    
    // Init MAC array
    macs_capacity.dim_x = accelerator->get_array_size(component_t::MAC_X, dimension_t::DIM_X); 
    macs_capacity.dim_y = accelerator->get_array_size(component_t::MAC_X, dimension_t::DIM_Y);

    // Init MAC radix
    macs_radix.input  = accelerator->get_radix_degree(component_t::MAC_X, data_t::INPUT);
    macs_radix.weight = accelerator->get_radix_degree(component_t::MAC_X, data_t::WEIGHT);
    macs_radix.output = accelerator->get_radix_degree(component_t::MAC_X, data_t::OUTPUT);
}
void analyzer_t::init_local_buffer() {
    // Init unit access cost
    component_t comp = component_t::LB;
    
    lb_unit_energy.input    = accelerator->get_data_access_energy(comp, data_t::INPUT);
    lb_unit_energy.weight   = accelerator->get_data_access_energy(comp, data_t::WEIGHT);
    lb_unit_energy.output   = accelerator->get_data_access_energy(comp, data_t::OUTPUT);
    lb_unit_cycle.input     = accelerator->get_data_access_cycle(comp, data_t::INPUT);
    lb_unit_cycle.weight    = accelerator->get_data_access_cycle(comp, data_t::WEIGHT);
    lb_unit_cycle.output    = accelerator->get_data_access_cycle(comp, data_t::OUTPUT);
    lb_unit_static          = accelerator->get_comp_level_static(component_t::LB);

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
    // Setup separated type configuration
    bool* arr_lb_separate = accelerator->get_separated_data(component_t::LB);
    if (arr_lb_separate != nullptr) {
        lb_separate.input  = arr_lb_separate[(unsigned)data_t::INPUT];
        lb_separate.weight = arr_lb_separate[(unsigned)data_t::WEIGHT];
        lb_separate.output = arr_lb_separate[(unsigned)data_t::OUTPUT];
    }
    bool*  arr_lb_bypass    = accelerator->get_bypass(component_t::LB);
    if(arr_lb_bypass != nullptr) {
        if(arr_lb_bypass[(unsigned)data_t::INPUT]) { lb_bypass.input = true; }
        if(arr_lb_bypass[(unsigned)data_t::WEIGHT]) { lb_bypass.weight = true; }  
        if(arr_lb_bypass[(unsigned)data_t::OUTPUT]) { lb_bypass.output = true; }  
    }
    
    // Init bitwidth
    lb_bitwidth.input  = accelerator->get_bitwidth(component_t::LB, data_t::INPUT) 
                        / accelerator->get_precision();
    lb_bitwidth.weight = accelerator->get_bitwidth(component_t::LB, data_t::WEIGHT) 
                        / accelerator->get_precision();
    lb_bitwidth.output = accelerator->get_bitwidth(component_t::LB, data_t::OUTPUT) 
                        / accelerator->get_precision();

    lb_constraint = accelerator->get_memory_constraint(component_t::LB);
}
// Init PE array
void analyzer_t::init_pe_array() {
    // Init PE array's dimension
    pes_capacity.dim_x = accelerator->get_array_size(component_t::PE_X, dimension_t::DIM_X);
    pes_capacity.dim_y = accelerator->get_array_size(component_t::PE_X, dimension_t::DIM_Y);

    // Init mapping constraints
    pes_constraint.dim_x = split(accelerator->get_array_constraint(component_t::PE_X, dimension_t::DIM_X),',');
    pes_constraint.dim_y = split(accelerator->get_array_constraint(component_t::PE_X, dimension_t::DIM_Y),',');
    // If either const dim_x or dim_y exist
    if(!(pes_constraint.dim_x.empty() && pes_constraint.dim_y.empty())) {
        pes_constraint.empty = false;
    }
    
    // Init PE array's radix
    pes_radix.input  = accelerator->get_radix_degree(component_t::PE_X, data_t::INPUT);
    pes_radix.weight = accelerator->get_radix_degree(component_t::PE_X, data_t::WEIGHT);
    pes_radix.output = accelerator->get_radix_degree(component_t::PE_X, data_t::OUTPUT);
}
// Init global buffer
void analyzer_t::init_global_buffer() {
    component_t comp = component_t::GB;
    
    // Check if global buffer exists
    is_gb_exist = accelerator->check_component_exist(comp);

    // Initialize unit access costs
    gb_unit_energy.input  = accelerator->get_data_access_energy(comp, data_t::INPUT);
    gb_unit_energy.weight = accelerator->get_data_access_energy(comp, data_t::WEIGHT);
    gb_unit_energy.output = accelerator->get_data_access_energy(comp, data_t::OUTPUT);
    
    gb_unit_cycle.input  = accelerator->get_data_access_cycle(comp, data_t::INPUT);
    gb_unit_cycle.weight = accelerator->get_data_access_cycle(comp, data_t::WEIGHT);
    gb_unit_cycle.output = accelerator->get_data_access_cycle(comp, data_t::OUTPUT);
    
    gb_unit_static = accelerator->get_comp_level_static(comp);
    // Setup separated type configuration
    bool* arr_gb_separate = accelerator->get_separated_data(comp);
    if (arr_gb_separate!= nullptr) {
        gb_separate.input  = arr_gb_separate[(unsigned)data_t::INPUT];
        gb_separate.weight = arr_gb_separate[(unsigned)data_t::WEIGHT];
        gb_separate.output = arr_gb_separate[(unsigned)data_t::OUTPUT];
    }

    // Setup bypass configuration
    bool* arr_gb_bypass = accelerator->get_bypass(comp);
    if (arr_gb_bypass != nullptr) {
        gb_bypass.input  = arr_gb_bypass[(unsigned)data_t::INPUT];
        gb_bypass.weight = arr_gb_bypass[(unsigned)data_t::WEIGHT];
        gb_bypass.output = arr_gb_bypass[(unsigned)data_t::OUTPUT];
    }

    // Initialize buffer sizes
    std::vector<float> arr_buffer_size = accelerator->get_size(comp);
    float precision_ratio = accelerator->get_precision() / BYTE;

    // Handle different buffer configurations
    if (arr_buffer_size.size() == 1) {  // Shared buffer
        is_gb_shared = true;
        gb_capacity.shared = arr_buffer_size.back() / precision_ratio;
        gb_capacity.input = gb_capacity.weight = gb_capacity.output = 0;
    }
    else if (arr_buffer_size.size() == 2) {  // Partially separated buffer
        is_gb_shared = true;
        gb_capacity.shared = arr_buffer_size.front() / precision_ratio;
        float separate_capacity = arr_buffer_size.back() / precision_ratio;

        if (gb_separate.input) {
            gb_capacity.input = separate_capacity;
        }
        else if (gb_separate.weight) {
            gb_capacity.weight = separate_capacity;
        }
        else if (gb_separate.output) {
            gb_capacity.output = separate_capacity;
        }
        else {
            std::cerr << "Error: please specify which data type has its own buffer through typing `separate_data` in accelerator config" << std::endl;
            exit(0);
        }
    }
    else if (arr_buffer_size.size() == 3) {  // Fully separated buffer
        is_gb_shared = false;
        gb_capacity.input  = arr_buffer_size.at((unsigned)data_t::INPUT)  / precision_ratio;
        gb_capacity.weight = arr_buffer_size.at((unsigned)data_t::WEIGHT) / precision_ratio;
        gb_capacity.output = arr_buffer_size.at((unsigned)data_t::OUTPUT) / precision_ratio;
    }
    else {
        is_gb_exist = false;
    }

    // Initialize bitwidth configuration
    float precision = accelerator->get_precision();
    gb_bitwidth.input  = accelerator->get_bitwidth(comp, data_t::INPUT)  / precision;
    gb_bitwidth.weight = accelerator->get_bitwidth(comp, data_t::WEIGHT) / precision;
    gb_bitwidth.output = accelerator->get_bitwidth(comp, data_t::OUTPUT) / precision;

    // Set memory constraints
    gb_constraint = accelerator->get_memory_constraint(comp);
}
void analyzer_t::init_multi_chips() {
    // Init Multi chip 
    chips_capacity.dim_x = accelerator->get_array_size(component_t::CHIP_X, dimension_t::DIM_X);
    chips_capacity.dim_y = accelerator->get_array_size(component_t::CHIP_X, dimension_t::DIM_Y);
    
    // Init multi-chips' radix
    chips_radix.input  = accelerator->get_radix_degree(component_t::CHIP_X, data_t::INPUT);
    chips_radix.weight = accelerator->get_radix_degree(component_t::CHIP_X, data_t::WEIGHT);
    chips_radix.output = accelerator->get_radix_degree(component_t::CHIP_X, data_t::OUTPUT);
}
void analyzer_t::init_dram() {
    // Init unit access cost
    component_t comp = component_t::DRAM;
    dram_unit_energy.input  = accelerator->get_data_access_energy(comp, data_t::INPUT);
    dram_unit_energy.weight = accelerator->get_data_access_energy(comp, data_t::WEIGHT);
    dram_unit_energy.output = accelerator->get_data_access_energy(comp, data_t::OUTPUT);
    dram_unit_cycle.input   = accelerator->get_data_access_cycle(comp, data_t::INPUT);
    dram_unit_cycle.weight  = accelerator->get_data_access_cycle(comp, data_t::WEIGHT);
    dram_unit_cycle.output  = accelerator->get_data_access_cycle(comp, data_t::OUTPUT);
    dram_unit_static = accelerator->get_comp_level_static(component_t::DRAM);
    // Init bitwidth
    dram_bitwidth = accelerator->get_bitwidth(component_t::DRAM)[0] 
                  / accelerator->get_precision();
    dram_constraint = accelerator->get_memory_constraint(component_t::DRAM);
}
void analyzer_t::update_bypassed_data() {
    // Neat access counts and tile size for bypassed data.
    // This code is just for preventing misunderstood of the result file by users when 
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