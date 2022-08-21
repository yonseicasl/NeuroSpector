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

}
analyzer_t::analyzer_t(accelerator_t *accelerator_,
                       network_t    *network_)
    :accelerator(accelerator_),
     network(network_) {
}
analyzer_t::~analyzer_t() { }

void analyzer_t::run() {
    update_active_components();
    update_tile_size_to_receive();
    update_tile_size_to_send();
    update_access_count();

    check_validity();
    estimate_cost();
    print_stats();
}
void analyzer_t::init(scheduling_table_t scheduling_table_) {
    // Copy scheduling table
    scheduling_table = scheduling_table_;
    update_active_components();
    update_tile_size_to_receive();
    update_tile_size_to_send();
    update_access_count();
    return;
}
/* Section 1. Check validity of components */
bool analyzer_t::check_validity() const {
    bool rtn = true;
    // Traverse all rows and check hardware constraints. 
    rtn *= check_hardware_constraints();
    // Traverse all columns and check network constraints. 
    rtn *= check_network_constraints();
    return rtn;
}
bool analyzer_t::check_hardware_constraints() const {
    bool rtn = true;
    // Traverse accelerator components and check the size validity
    for(unsigned i = 0; i < accelerator->get_num_components(); i++) {
        if(accelerator->get_name(i) == "virtual") continue;
        if(accelerator->get_type(i) == component_type_t::SPATIAL) {
            rtn *= check_spatial_validity(i);
        }
        else if(accelerator->get_type(i) == component_type_t::TEMPORAL) {
            rtn *= check_temporal_validity(i);
        }
    }
    return rtn;
}
bool analyzer_t::check_spatial_validity(unsigned idx_) const {
    bool rtn = true;
    std::vector<float>    capacity = accelerator->get_size(idx_);
    std::vector<unsigned> allocated_size = accelerator->get_active_components(idx_);
    assert(capacity.size() == allocated_size.size());
    for(unsigned i = 0; i < capacity.size(); i++) {
        rtn *= (float)allocated_size.at(i) <= capacity.at(i);
    }
    return rtn;
}
bool analyzer_t::check_temporal_validity(unsigned idx_) const {
    bool rtn = true;
    unsigned size = 0;
    std::vector<float>    capacity = accelerator->get_size(idx_);
    std::vector<unsigned> allocated_size = accelerator->get_allocated_size(idx_, direction_type_t::LOWER);
    std::vector<data_type_t> bypass   = accelerator->get_bypass(idx_);
    // Shared buffer
    if(capacity.size() == 1) {
        for(unsigned i = 0; i < allocated_size.size(); i++) {
            // Bypass check
            if(find(bypass.begin(), bypass.end(), (data_type_t)i) 
                                                    != bypass.end()) { 
                continue; 
            }
            size += allocated_size.at(i);
        }
        // Check validity
        rtn = (float)size <= (capacity.at(0) * BYTE) / accelerator->get_precision();
    }
    // Separated buffer
    else if(capacity.size() == 3) {
        for(unsigned i = 0; i < allocated_size.size(); i++) {
            // Bypass check
            if(find(bypass.begin(), bypass.end(), (data_type_t)i) 
                                                    != bypass.end()) { 
                continue; 
            }
            rtn *= (float)allocated_size.at(i) <= (capacity.at(i) * BYTE) / accelerator->get_precision();
        }
    }
    return rtn;
}
bool analyzer_t::check_network_constraints() const {
    bool rtn = true;
    return rtn;
}

/* Section 2. Compute tile sizes */
void analyzer_t::update_tile_size_to_receive() {
    for(unsigned i = 0; i < scheduling_table.get_num_rows(); i++) {
        if(scheduling_table.get_component_type(i) == component_type_t::TEMPORAL
        && scheduling_table.get_component_name(i) != "virtual") {
            std::vector<unsigned> values((unsigned)parameter_t::SIZE, 1);
            // Update tile size 
            for(unsigned param_idx = 0; param_idx < (unsigned)parameter_t::SIZE; param_idx++) {
                for(unsigned row_idx = 0; row_idx < i+1; row_idx++) {
                    values.at(param_idx) 
                        *= scheduling_table.get_mapping_value(row_idx, param_idx);
                }
            }
            update_component_input_tile_size(i, values, direction_type_t::LOWER);
            update_component_weight_tile_size(i, values, direction_type_t::LOWER);
            update_component_output_tile_size(i, values, direction_type_t::LOWER);
            std::fill(values.begin(), values.end(), 1); // Reset vector
        }
    }
    return;
}
void analyzer_t::update_tile_size_to_send() {
    for(unsigned i = 0; i < scheduling_table.get_num_rows(); i++) {
        if(scheduling_table.get_component_type(i) == component_type_t::TEMPORAL
        && scheduling_table.get_component_name(i) != "virtual") {
            std::vector<unsigned> values((unsigned)parameter_t::SIZE, 1);
            // Update tile size
            for(unsigned param_idx = 0; param_idx < (unsigned)parameter_t::SIZE; param_idx++) {
                for(int row_idx = i - 1; row_idx >= 0; row_idx--) {
                    values.at(param_idx) 
                        *= scheduling_table.get_mapping_value(row_idx, param_idx);
                }
            }
            update_component_input_tile_size(i, values, direction_type_t::UPPER);
            update_component_weight_tile_size(i, values, direction_type_t::UPPER);
            update_component_output_tile_size(i, values, direction_type_t::UPPER);
            std::fill(values.begin(), values.end(), 1); // Reset vector
        }
    }
    return;
}
void analyzer_t::update_component_input_tile_size(unsigned idx_, 
                                                  std::vector<unsigned> values_,
                                                  direction_type_t direction_) {
    unsigned tile_size = 1;
    unsigned component_idx = 1;
    unsigned above_component_idx = 1;
    std::vector<data_type_t> bypass;

    // Compute input height & width
    unsigned input_height = 1;
    unsigned input_width  = 1;
    unsigned layer_idx = scheduling_table.get_layer_index();
    unsigned stride = network->get_stride(layer_idx);
    input_height = (values_.at((unsigned)parameter_t::P) - 1) * stride 
                 + values_.at((unsigned)parameter_t::R);
    input_width  = (values_.at((unsigned)parameter_t::Q) - 1) * stride 
                 + values_.at((unsigned)parameter_t::S);
    // Compute input tile size
    tile_size *= values_.at((unsigned)parameter_t::B) 
               * values_.at((unsigned)parameter_t::C)
               * values_.at((unsigned)parameter_t::G)
               * input_height * input_width;
    component_idx = scheduling_table.get_component_index(idx_);
    // Bypass adjustment
    above_component_idx = scheduling_table.get_component_index(scheduling_table.get_above_buffer_pos(idx_));
    // if the upper level component bypasses the weight data
    if(above_component_idx != UINT_MAX) {
        bypass = accelerator->get_bypass(above_component_idx);
    }
    if(find(bypass.begin(), bypass.end(), data_type_t::INPUT) != bypass.end()) {
        std::vector<unsigned> upper_level_tile_size 
                        = accelerator->get_allocated_size(above_component_idx, 
                                                          direction_type_t::UPPER);
        // then change tile size to be sended to that of upper level components. 
        tile_size = upper_level_tile_size.at((unsigned)data_type_t::INPUT);
        // And then change upper level tile size to be sended and received to zero.
        accelerator->update_allocated_tile_size(above_component_idx, 0, 
                                                data_type_t::INPUT,
                                                direction_type_t::UPPER);
        accelerator->update_allocated_tile_size(above_component_idx, 0, 
                                                data_type_t::INPUT,
                                                direction_type_t::LOWER);
    }
    accelerator->update_allocated_tile_size(component_idx, tile_size, 
                                            data_type_t::INPUT,
                                            direction_);
    return;
}
void analyzer_t::update_component_weight_tile_size(unsigned idx_,
                                                   std::vector<unsigned> values_,
                                                   direction_type_t direction_) {
    unsigned tile_size = 1;
    unsigned component_idx = 1;
    unsigned above_component_idx = 1;
    std::vector<data_type_t> bypass;
    // Compute weight tile size
    tile_size *= values_.at((unsigned)parameter_t::K) 
               * values_.at((unsigned)parameter_t::C)
               * values_.at((unsigned)parameter_t::R) 
               * values_.at((unsigned)parameter_t::S)
               * values_.at((unsigned)parameter_t::G);
    component_idx = scheduling_table.get_component_index(idx_);
    // Bypass adjustment
    above_component_idx = scheduling_table.get_component_index(scheduling_table.get_above_buffer_pos(idx_));
    // if the upper level component bypasses the weight data
    if(direction_ == direction_type_t::UPPER) {
        if(above_component_idx != UINT_MAX) {
            bypass = accelerator->get_bypass(above_component_idx);
        }
        if (find(bypass.begin(), bypass.end(), data_type_t::WEIGHT) != bypass.end()) {
            std::vector<unsigned> upper_level_tile_size 
                        = accelerator->get_allocated_size(above_component_idx, 
                                                          direction_type_t::UPPER);
            // then change tile size to be sended to that of upper level components.
            tile_size = upper_level_tile_size.at((unsigned)data_type_t::WEIGHT);
            // And then change upper level tile size to be sended and received to zero.
            accelerator->update_allocated_tile_size(above_component_idx, 0,
                                                    data_type_t::WEIGHT,
                                                    direction_type_t::UPPER);
            accelerator->update_allocated_tile_size(above_component_idx, 0,
                                                    data_type_t::WEIGHT,
                                                    direction_type_t::LOWER);
        }
    }
    accelerator->update_allocated_tile_size(component_idx, tile_size, 
                                            data_type_t::WEIGHT,
                                            direction_);
    return;
}
void analyzer_t::update_component_output_tile_size(unsigned idx_,
                                                   std::vector<unsigned> values_,
                                                   direction_type_t direction_) {
    unsigned     tile_size = 1;
    unsigned component_idx = 1;
    unsigned above_component_idx = 1;
    std::vector<data_type_t> bypass;
    // Compute output tile size/ab
    tile_size *= values_.at((unsigned)parameter_t::K) 
               * values_.at((unsigned)parameter_t::B)
               * values_.at((unsigned)parameter_t::P) 
               * values_.at((unsigned)parameter_t::Q)
               * values_.at((unsigned)parameter_t::G);
    component_idx = scheduling_table.get_component_index(idx_);
    // Bypass adjustment
    above_component_idx = scheduling_table.get_component_index(scheduling_table.get_above_buffer_pos(idx_));
    // if the upper level component bypasses the weight data
    if(above_component_idx != UINT_MAX) {
        bypass = accelerator->get_bypass(above_component_idx);
    }
    if(find(bypass.begin(), bypass.end(), data_type_t::OUTPUT) != bypass.end()) {
        std::vector<unsigned> upper_level_tile_size 
                        = accelerator->get_allocated_size(above_component_idx, 
                                                          direction_type_t::UPPER);
        // then change tile size to be sended to that of upper level components. 
        tile_size = upper_level_tile_size.at((unsigned)data_type_t::OUTPUT);
        // And then change upper level tile size to be sended and received to zero.
        accelerator->update_allocated_tile_size(above_component_idx, 0, 
                                                data_type_t::OUTPUT,
                                                direction_type_t::UPPER);
        accelerator->update_allocated_tile_size(above_component_idx, 0, 
                                                data_type_t::OUTPUT,
                                                direction_type_t::LOWER);
    }
    accelerator->update_allocated_tile_size(component_idx, tile_size, 
                                            data_type_t::OUTPUT,
                                            direction_);
    return;
}
/* Section 3. Compute access counts */
void analyzer_t::update_access_count() {
    for(unsigned i = 0; i < scheduling_table.get_num_rows()-1; i++) {
        if(scheduling_table.get_component_type(i) == component_type_t::TEMPORAL
        && scheduling_table.get_component_name(i) != "virtual" ) {
            unsigned tile_access_count = 1;
            // Get baseline tile access count
            for(unsigned param_idx = 0; param_idx < (unsigned)parameter_t::SIZE; param_idx++) {
                // i from one level below a buffer downwards to DRAM
                for(int row_idx = (i+1); row_idx < (int)scheduling_table.get_num_rows(); row_idx++) {
                    if(scheduling_table.get_component_type(row_idx) == component_type_t::SPATIAL) { continue; }
                    tile_access_count *= scheduling_table.get_mapping_value(row_idx, param_idx);
                }
            }
            // Get tile access count for each data type
            update_component_input_access_count(i, tile_access_count);
            update_component_weight_access_count(i, tile_access_count);
            update_component_output_access_count(i, tile_access_count);
        }
    }
    return;
}
void analyzer_t::update_component_input_access_count(unsigned idx_,
                                                     unsigned value_) {
    access_count = value_;
    unsigned lower_level_idx = scheduling_table.get_below_buffer_pos(idx_);
    unsigned upper_level_idx = scheduling_table.get_above_buffer_pos(idx_);
    dataflow_t dataflow = dataflow_t::NONE;
    component_idx_curr = scheduling_table.get_component_index(idx_);
    component_idx_lower = scheduling_table.get_component_index(lower_level_idx);
    component_idx_upper = scheduling_table.get_component_index(upper_level_idx);
    std::vector<data_type_t> bypass;

    if(component_idx_curr == 0) {
        unsigned tile_access_count = 1;
        for(unsigned param_idx = 0; param_idx < (unsigned)parameter_t::SIZE; param_idx++) {
            for(int row_idx = 0; row_idx < (int)scheduling_table.get_num_rows(); row_idx++) {
                if(scheduling_table.get_component_type(row_idx) == component_type_t::TEMPORAL) { 
                    tile_access_count *= scheduling_table.get_mapping_value(row_idx, param_idx);
                }
            }
        }
        accelerator->update_tile_access_count(component_idx_curr, tile_access_count, 
                                              data_type_t::INPUT, operation_type_t::READ,
                                              direction_type_t::UPPER);
    }
    // Get dataflow
    dataflow = accelerator->get_dataflow(component_idx_curr);
    if(dataflow == dataflow_t::IS) { 
        access_count /= update_irrelevant_mapping_value(idx_, data_type_t::INPUT); 
    }
    // Bypass adjustment
    // if the upper level component bypasses the weight data
    if(component_idx_curr != UINT_MAX) {
        bypass = accelerator->get_bypass(component_idx_curr);
    }
    if(find(bypass.begin(), bypass.end(), data_type_t::INPUT) != bypass.end()) { 
        access_count = 0; 
    }
    accelerator->update_tile_access_count(component_idx_curr, access_count, 
                                          data_type_t::INPUT, operation_type_t::WRITE,
                                          direction_type_t::LOWER);

    std::vector<unsigned> send_tile_size 
            = accelerator->get_allocated_size(component_idx_curr, direction_type_t::UPPER);
    std::vector<unsigned> received_tile_size 
            = accelerator->get_allocated_size(component_idx_curr, direction_type_t::LOWER);
    if(send_tile_size.at((unsigned)data_type_t::INPUT) == received_tile_size.at((unsigned)data_type_t::INPUT) &&
       send_tile_size.at((unsigned)data_type_t::INPUT) != 0) {
        if(component_idx_curr != 0) {
        // Change access count to lower level access count
        accelerator->update_tile_access_count(component_idx_curr, access_count, 
                                              data_type_t::INPUT, operation_type_t::READ,
                                              direction_type_t::UPPER);
        }
    }

    if(find(bypass.begin(), bypass.end(), data_type_t::INPUT) != bypass.end()) {
        std::vector<unsigned> upper_level_tile_access_count
                        = accelerator->get_tile_access_count(component_idx_curr, operation_type_t::READ,
                                                             direction_type_t::UPPER);
        access_count = upper_level_tile_access_count.at((unsigned)data_type_t::INPUT);
        accelerator->update_tile_access_count(component_idx_curr, 0, data_type_t::INPUT, 
                                              operation_type_t::READ, direction_type_t::UPPER);
    }
    accelerator->update_tile_access_count(component_idx_lower, access_count, 
                                            data_type_t::INPUT, operation_type_t::READ,
                                            direction_type_t::UPPER);
    return;
}
void analyzer_t::update_component_weight_access_count(unsigned idx_,
                                                      unsigned value_) {
    access_count = value_;
    unsigned lower_level_idx = scheduling_table.get_below_buffer_pos(idx_);
    unsigned upper_level_idx = scheduling_table.get_above_buffer_pos(idx_);
    dataflow_t dataflow = dataflow_t::NONE;
    component_idx_curr = scheduling_table.get_component_index(idx_);
    component_idx_lower = scheduling_table.get_component_index(lower_level_idx);
    component_idx_upper = scheduling_table.get_component_index(upper_level_idx);
    std::vector<data_type_t> bypass;

    // If the component connected to MAC
    if(component_idx_curr == 0) {
        unsigned tile_access_count = 1;
        for(unsigned param_idx = 0; param_idx < (unsigned)parameter_t::SIZE; param_idx++) {
            for(int row_idx = 0; row_idx < (int)scheduling_table.get_num_rows(); row_idx++) {
                if(scheduling_table.get_component_type(row_idx) == component_type_t::TEMPORAL) { 
                    tile_access_count *= scheduling_table.get_mapping_value(row_idx, param_idx);
                }
            }
        }
        accelerator->update_tile_access_count(component_idx_curr, tile_access_count, 
                                              data_type_t::WEIGHT, operation_type_t::READ,
                                              direction_type_t::UPPER);
    }
    // Dataflow adjustment
    dataflow = accelerator->get_dataflow(component_idx_curr);
    if(dataflow == dataflow_t::WS) { 
        access_count /= update_irrelevant_mapping_value(idx_, data_type_t::WEIGHT); 
    }
    // Bypass adjustment
    // if the current level component bypasses the weight data
    if(component_idx_curr != UINT_MAX) {
        bypass = accelerator->get_bypass(component_idx_curr);
    }
    if(find(bypass.begin(), bypass.end(), data_type_t::WEIGHT) != bypass.end()) { access_count = 0; }
    // Update component tile access count
    accelerator->update_tile_access_count(component_idx_curr, access_count, 
                                          data_type_t::WEIGHT, operation_type_t::WRITE,
                                          direction_type_t::LOWER);
    // Read once adjustment
    if(dataflow != dataflow_t::NONE) {
        std::vector<unsigned> send_tile_size 
                = accelerator->get_allocated_size(component_idx_curr, direction_type_t::UPPER);
        std::vector<unsigned> received_tile_size 
                = accelerator->get_allocated_size(component_idx_curr, direction_type_t::LOWER);
        if(send_tile_size.at((unsigned)data_type_t::WEIGHT) == received_tile_size.at((unsigned)data_type_t::WEIGHT) &&
        send_tile_size.at((unsigned)data_type_t::WEIGHT) != 0) {
            if(component_idx_curr != 0) {
            // Change access count to lower level access count
            // UPPER READ = LOWER WRITE
            accelerator->update_tile_access_count(component_idx_curr, access_count, 
                                                    data_type_t::WEIGHT, operation_type_t::READ,
                                                    direction_type_t::UPPER);
            }
        }
    }
    if(find(bypass.begin(), bypass.end(), data_type_t::WEIGHT) != bypass.end()) {
        std::vector<unsigned> upper_level_tile_access_count
                        = accelerator->get_tile_access_count(component_idx_curr, operation_type_t::READ,
                                                             direction_type_t::UPPER);
        access_count = upper_level_tile_access_count.at((unsigned)data_type_t::WEIGHT);
        // Set current level's access count to zero
        accelerator->update_tile_access_count(component_idx_curr, 0, data_type_t::WEIGHT, 
                                              operation_type_t::READ, direction_type_t::UPPER);
        // Weight data fully loaded it's upper level at once
        if(access_count == scheduling_table.get_correlation_product(idx_,
                                                                    correlation_type_t::OI)) {
            access_count = 1;
            // Change it's upper level receive count
            accelerator->update_tile_access_count(component_idx_upper, access_count,
                                                  data_type_t::WEIGHT, operation_type_t::WRITE,
                                                  direction_type_t::LOWER);
        }
    }
    accelerator->update_tile_access_count(component_idx_lower, access_count, 
                                            data_type_t::WEIGHT, operation_type_t::READ,
                                            direction_type_t::UPPER);
    return;
}
void analyzer_t::update_component_output_access_count(unsigned idx_,
                                                      unsigned value_) {
    unsigned lower_level_idx    = scheduling_table.get_below_buffer_pos(idx_);
    unsigned upper_level_idx = scheduling_table.get_above_buffer_pos(idx_);
    dataflow_t dataflow = dataflow_t::NONE;
    component_idx_curr = scheduling_table.get_component_index(idx_);
    component_idx_lower = scheduling_table.get_component_index(lower_level_idx);
    component_idx_upper = scheduling_table.get_component_index(upper_level_idx);
    std::vector<data_type_t> bypass;
    std::vector<unsigned> upper_level_tile_access_count;

    // If the component connected to MAC
    if(component_idx_curr == 0) {
        write_access_count = scheduling_table.get_correlation_product(-1, correlation_type_t::IWO)
                           * scheduling_table.get_correlation_product(-1, correlation_type_t::WO)
                           * scheduling_table.get_correlation_product(-1, correlation_type_t::OI)
                           * (scheduling_table.get_correlation_product(-1, correlation_type_t::IW) - 1);
        read_access_count = scheduling_table.get_correlation_product(-1, correlation_type_t::IWO)
                           * scheduling_table.get_correlation_product(-1, correlation_type_t::WO)
                           * scheduling_table.get_correlation_product(-1, correlation_type_t::OI)
                           * scheduling_table.get_correlation_product(-1, correlation_type_t::IW);
        accelerator->update_tile_access_count(component_idx_curr, write_access_count, 
                                              data_type_t::OUTPUT, operation_type_t::READ,
                                              direction_type_t::UPPER);
        accelerator->update_tile_access_count(component_idx_curr, read_access_count, 
                                              data_type_t::OUTPUT, operation_type_t::WRITE,
                                              direction_type_t::UPPER);
    }
    // Update read and write access count 
    write_access_count = scheduling_table.get_correlation_product(idx_, correlation_type_t::IWO)
                      * scheduling_table.get_correlation_product(idx_, correlation_type_t::WO)
                      * scheduling_table.get_correlation_product(idx_, correlation_type_t::OI)
                      * (scheduling_table.get_correlation_product(idx_, correlation_type_t::IW) - 1);
    read_access_count = value_;
    // Get dataflow
    dataflow = accelerator->get_dataflow(component_idx_curr);
    if(dataflow == dataflow_t::OS) { 
        write_access_count = scheduling_table.get_correlation_product(idx_, correlation_type_t::IWO)
                          * scheduling_table.get_correlation_product(idx_, correlation_type_t::WO)
                          * scheduling_table.get_correlation_product(idx_, correlation_type_t::OI)
                          * (scheduling_table.get_correlation_product(lower_level_idx, correlation_type_t::IW) - 1);
        read_access_count /= update_irrelevant_mapping_value(idx_, data_type_t::OUTPUT); 
    }
    // Bypass adjustment
    // if the upper level component bypasses the weight data
    if(component_idx_curr != UINT_MAX) {
        bypass = accelerator->get_bypass(component_idx_curr);
    }
    if(find(bypass.begin(), bypass.end(), data_type_t::OUTPUT) != bypass.end()) { 
        write_access_count = 0; read_access_count = 0; 
    }
    // Update component tile access count
    accelerator->update_tile_access_count(component_idx_curr, write_access_count, 
                                          data_type_t::OUTPUT, operation_type_t::WRITE,
                                          direction_type_t::LOWER);
    accelerator->update_tile_access_count(component_idx_curr, read_access_count, 
                                          data_type_t::OUTPUT, operation_type_t::READ,
                                          direction_type_t::LOWER);
    // 2. Read once adjustment
    if(dataflow != dataflow_t::NONE) {
    std::vector<unsigned> send_tile_size 
            = accelerator->get_allocated_size(component_idx_curr, direction_type_t::UPPER);
    std::vector<unsigned> received_tile_size 
            = accelerator->get_allocated_size(component_idx_curr, direction_type_t::LOWER);
    if(send_tile_size.at((unsigned)data_type_t::OUTPUT) == received_tile_size.at((unsigned)data_type_t::OUTPUT) &&
       send_tile_size.at((unsigned)data_type_t::OUTPUT) != 0) {
        if(component_idx_curr != 0 && 
           component_idx_curr != accelerator->get_num_components()) {
        // Change access count to upper level access count
        accelerator->update_tile_access_count(component_idx_upper, read_access_count, 
                                                data_type_t::OUTPUT, operation_type_t::READ,
                                                direction_type_t::LOWER);
        accelerator->update_tile_access_count(component_idx_upper, write_access_count, 
                                                data_type_t::OUTPUT, operation_type_t::WRITE,
                                                direction_type_t::LOWER);
        // Change access count to lower level access count
        accelerator->update_tile_access_count(component_idx_curr, write_access_count, 
                                                data_type_t::OUTPUT, operation_type_t::READ,
                                                direction_type_t::UPPER);
        accelerator->update_tile_access_count(component_idx_curr, read_access_count, 
                                                data_type_t::OUTPUT, operation_type_t::WRITE,
                                                direction_type_t::UPPER);
        }
    }
    }
    if(find(bypass.begin(), bypass.end(), data_type_t::OUTPUT) != bypass.end()) {
        upper_level_tile_access_count = accelerator->get_tile_access_count(component_idx_curr, operation_type_t::READ,
                                                                           direction_type_t::UPPER);
        write_access_count = upper_level_tile_access_count.at((unsigned)data_type_t::OUTPUT);
        
        upper_level_tile_access_count = accelerator->get_tile_access_count(component_idx_curr, operation_type_t::WRITE,
                                                                           direction_type_t::UPPER);
        read_access_count  = upper_level_tile_access_count.at((unsigned)data_type_t::OUTPUT);
        accelerator->update_tile_access_count(component_idx_curr, 0, data_type_t::OUTPUT, 
                                              operation_type_t::READ, direction_type_t::UPPER);
        accelerator->update_tile_access_count(component_idx_curr, 0, data_type_t::OUTPUT, 
                                              operation_type_t::WRITE, direction_type_t::UPPER);
    }
    accelerator->update_tile_access_count(component_idx_lower, write_access_count, 
                                        data_type_t::OUTPUT, operation_type_t::READ,
                                        direction_type_t::UPPER);
    accelerator->update_tile_access_count(component_idx_lower, read_access_count, 
                                            data_type_t::OUTPUT, operation_type_t::WRITE,
                                            direction_type_t::UPPER);
    return;
}
unsigned analyzer_t::update_irrelevant_mapping_value(unsigned row_idx_,
                                                      data_type_t stationary_data_) {
    unsigned irrelevant_mapping_value = 1;
    unsigned one_level_below = 0; 
    // Find temporal component which exists at a level below the buffer
    for(unsigned i = row_idx_ + 1; i < scheduling_table.get_num_rows(); i++) {
        if(scheduling_table.get_component_type(i) == component_type_t::TEMPORAL) {
            one_level_below = i;
            break;
        }
    }
    assert(one_level_below != 0);
    // Product all irrelevant mapping values with stationary data
    switch(stationary_data_) {
    case data_type_t::INPUT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(one_level_below, (unsigned)parameter_t::K);
        break;
    case data_type_t::WEIGHT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(one_level_below, (unsigned)parameter_t::B)
            * scheduling_table.get_mapping_value(one_level_below, (unsigned)parameter_t::P)
            * scheduling_table.get_mapping_value(one_level_below, (unsigned)parameter_t::Q);
        break;
    case data_type_t::OUTPUT:
        irrelevant_mapping_value 
            = scheduling_table.get_mapping_value(one_level_below, (unsigned)parameter_t::C)
            * scheduling_table.get_mapping_value(one_level_below, (unsigned)parameter_t::R)
            * scheduling_table.get_mapping_value(one_level_below, (unsigned)parameter_t::S);
        break;
    default:
        std::cerr << "Error in irrelevant loop calculation" << std::endl;
        exit(0);
    }
    return irrelevant_mapping_value;
}
/* Section 4. Compute the number of active components */
void analyzer_t::update_active_components() {
    std::vector<unsigned> num_active_components;
    unsigned         value = 1;
    unsigned component_idx = 1;

    for(unsigned row_idx = 0; row_idx < scheduling_table.get_num_rows(); row_idx++) {
        if(scheduling_table.get_component_type(row_idx) == component_type_t::SPATIAL) {
            for(unsigned param_idx = 0; param_idx < (unsigned)parameter_t::SIZE; param_idx++) {
                value *= scheduling_table.get_mapping_value(row_idx, param_idx);
            }
            num_active_components.push_back(value);
            value = 1;
        }
        // Collects # active_components info. for both X,Y dimension
        if(num_active_components.size() == 2) {
            // Determine accelerator component index.
            component_idx = scheduling_table.get_component_index(row_idx);
            // If component idx is valid
            if(component_idx != UINT_MAX) {
                accelerator->update_active_components(component_idx, num_active_components);
            }
            num_active_components.clear();
        }
    }
    return;
}
/* End Section 4 */

/* Section 5. Compute energy cost of accelerator */
void analyzer_t::estimate_cost() {
    // Estimate scheduling table's energy, cycle
    update_cycle(); 
    update_energy(); 
    update_static_energy(); 
    return;
}
// Estimate saved cost by reusing output data of prev. layer as input data
void analyzer_t::estimate_cross_layer_reuse(scheduling_table_t prev_table_,
                                            metric_type_t      metric_) {

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
    float  input_access_energy = unit_energy.at((unsigned)data_type_t::INPUT);
    float output_access_energy = unit_energy.at((unsigned)data_type_t::INPUT);
    float   input_access_cycle = unit_cycle.at((unsigned)data_type_t::INPUT);
    float  output_access_cycle = unit_cycle.at((unsigned)data_type_t::INPUT);
	// Compute overlapped tile size
	overlapped_size = compute_overlapped_size(prev_table_);
	// Compute the amount of reduced cost
	if(metric_ == metric_type_t::ENERGY) {
        // Energy
        reduced_cost += overlapped_size
                     * (input_access_energy + output_access_energy);
        // Change the cost of target scheduling_option
	    total_energy -= reduced_cost;
	}
	else if(metric_ == metric_type_t::CYCLE) {
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

void analyzer_t::update_cycle() {
    unsigned component_idx = 0;
    float mac_cycle = 0.0;
    float buffer_cycle = 0.0;
    std::vector<unsigned> tile_size(3,0);
    std::vector<unsigned> access_count(3,0);
    std::vector<float> unit_cycle(3,0);
    unsigned bandwidth = 1;
    unsigned precision = accelerator->get_precision();
    // Accumulate MAC unit cost
    mac_cycle = scheduling_table.get_num_mac_operations()
              / accelerator->get_active_MACs();
    components_cycle.push_back(mac_cycle);
    // Accumulate each buffer (or memory) cost
    for(unsigned row_idx = 0; row_idx < scheduling_table.get_num_rows(); row_idx++) {
        if(scheduling_table.get_component_type(row_idx) == component_type_t::TEMPORAL) {
            component_idx = scheduling_table.get_component_index(row_idx);
            // Compute energy consumption for each component 
            for (unsigned op_type = 0; op_type < (unsigned)operation_type_t::SIZE; op_type++) {
                // Get tile size and access count and unit access energy 
                tile_size = accelerator->get_allocated_size(component_idx, direction_type_t::UPPER);
                access_count = accelerator->get_tile_access_count(component_idx, 
                                                                    (operation_type_t)op_type, 
                                                                    direction_type_t::UPPER);
                unit_cycle = accelerator->get_component_cycle(component_idx);
                bandwidth  = accelerator->get_bandwidth(component_idx);
                // If bandwidth is undefined, default bandwidth is equal to precision
                if(bandwidth == 1) { bandwidth = precision; }
                // Calculate cycle 
                for (unsigned i = 0; i < (unsigned)data_type_t::SIZE; i++) {
                    assert(tile_size.size() == (unsigned)data_type_t::SIZE
                        && access_count.size() == (unsigned)data_type_t::SIZE
                        && unit_cycle.size() == (unsigned)data_type_t::SIZE);
                    // Buffer type is unified
                    if(accelerator->get_size(component_idx).size()==1) {
                        buffer_cycle += std::ceil((float)tile_size.at(i) / (float)bandwidth) 
                                      * (float)access_count.at(i) * unit_cycle.at(i);
                    }
                    // Buffer type is seperated
                    else {
                        unsigned data_transfer_cycle = std::ceil((float)tile_size.at(i) 
                                                     / ((float)bandwidth / (float)precision))
                                                     * (float)access_count.at(i) * unit_cycle.at(i);
                        if(data_transfer_cycle > buffer_cycle) {
                            buffer_cycle = data_transfer_cycle;
                        }
                    }
                }
            }
            accelerator->update_accelerator_energy(buffer_cycle);
            components_cycle.push_back(buffer_cycle);
            buffer_cycle = 0.0;
        }
    }
    // Update total cycle 
    for(unsigned i = 0; i < components_cycle.size(); i++) {
        total_cycle += components_cycle.at(i);
    }
}
void analyzer_t::update_static_energy() {
    // Total static energy (pJ) 
    // = Unit static power (mW) x clock time (ns) x total cycle count
    ///
    float mac_static_energy = 0.0;
    float buffer_static_energy = 0.0;
    unsigned component_idx = 0;
    mac_static_energy = accelerator->get_mac_static_power()
                      * accelerator->get_clock_time()
                      * accelerator->get_total_num_MACs()
                      * total_cycle;
    components_static_energy.push_back(mac_static_energy);
    total_static_energy += mac_static_energy;
    for(unsigned row_idx = 0; row_idx < scheduling_table.get_num_rows(); row_idx++) {
        buffer_static_energy = 0.0;
        component_idx = scheduling_table.get_component_index(row_idx);
        if(scheduling_table.get_component_type(row_idx) == component_type_t::TEMPORAL) {
            buffer_static_energy = accelerator->get_component_static_power(component_idx)
                                 * accelerator->get_clock_time()
                                 * accelerator->get_total_num_components(component_idx)
                                 * total_cycle;
            components_static_energy.push_back(buffer_static_energy);
            total_static_energy += buffer_static_energy;
        }
    }
    return;
}
void analyzer_t::update_energy() {
    unsigned component_idx = 0;
    float mac_energy = 0.0;
    float buffer_energy = 0.0;
    std::vector<unsigned> tile_size(3,0);
    std::vector<unsigned> access_count(3,0);
    std::vector<float> unit_energy(3,0);

    const unsigned dim_x = (unsigned)dimension_t::DIM_X;
    const unsigned dim_y = (unsigned)dimension_t::DIM_Y;
    // Accumulate MAC unit cost
    mac_energy = scheduling_table.get_num_mac_operations()
               * accelerator->get_mac_energy();
    accelerator->update_accelerator_energy(mac_energy);
    components_energy.push_back(mac_energy);
    total_energy += mac_energy;
    // Accumulate each buffer (or memory) cost
    for(unsigned row_idx = 0; row_idx < scheduling_table.get_num_rows(); row_idx++) {
        if(scheduling_table.get_component_type(row_idx) == component_type_t::TEMPORAL) {
            buffer_energy = 0.0;
            component_idx = scheduling_table.get_component_index(row_idx);
            // Compute energy consumption for each component 
            for (unsigned dir_type = 0; dir_type < (unsigned)direction_type_t::SIZE; dir_type++) {
                for (unsigned op_type = 0; op_type < (unsigned)operation_type_t::SIZE; op_type++) {
                    // Get tile size and access count and unit access energy 
                    tile_size = accelerator->get_allocated_size(component_idx, (direction_type_t)dir_type);
                    access_count = accelerator->get_tile_access_count(component_idx, 
                                                                      (operation_type_t)op_type, 
                                                                      (direction_type_t)dir_type);
                    unit_energy = accelerator->get_component_energy(component_idx);
                    // Calculate energy consumption
                    for (unsigned i = 0; i < (unsigned)data_type_t::SIZE; i++) {
                        assert(tile_size.size() == (unsigned)data_type_t::SIZE
                           && access_count.size() == (unsigned)data_type_t::SIZE
                           && unit_energy.size() == (unsigned)data_type_t::SIZE);
                        buffer_energy += (float)tile_size.at(i) * (float)access_count.at(i) * unit_energy.at(i);
                    }
                }
            }
            // Consider energy consumption of all active components
            for(unsigned i = row_idx + 1; i < scheduling_table.get_num_rows(); i++) {
                if(scheduling_table.get_component_type(i) == component_type_t::SPATIAL) {
                    component_idx = scheduling_table.get_component_index(i);
                    // If component is virtual component
                    if(component_idx == UINT_MAX) { buffer_energy *= 1; }
                    else {
                        buffer_energy *= accelerator->get_active_components(component_idx).at(dim_x)
                                       * accelerator->get_active_components(component_idx).at(dim_y);
                    }
                    i++; // Skip Y
                }
            }
            accelerator->update_accelerator_energy(buffer_energy);
            components_energy.push_back(buffer_energy);
            total_energy += buffer_energy;
        }
    }
}
/* End Section 5 */
void analyzer_t::print_stats() {
    std::cout << "====  Analyze Results ====" << std::endl;
    scheduling_table.print_stats();
    accelerator->print_stats();
    std::cout << "====    Energy/Cycle  ====" << std::endl;
    // Print component's name
    std::cout << " Components,";
    std::cout << std::setw(13) << "MAC" << ",";
    for(unsigned i = 0; i < scheduling_table.get_num_rows(); i++) {
        if(scheduling_table.get_component_type(i) == component_type_t::TEMPORAL) {
            unsigned component_idx = scheduling_table.get_component_index(i);
            std::cout << std::setw(13) << accelerator->get_name(component_idx) << ",";
        }
    }
    std::cout << std::setw(13) << "Total" << std::endl;
    // Print components' energy
    std::cout << "Energy (pJ),";
    for(unsigned i = 0; i < components_energy.size(); i++) {
        std::cout << std::setw(13) 
                  << std::fixed << std::setprecision(1)
                  << components_energy.at(i) << ",";
    }
    std::cout << std::setw(13) 
              << std::fixed << std::setprecision(1)
              << total_energy << std::endl;
    std::cout << "Static (pJ),";
    for(unsigned i = 0; i < components_energy.size(); i++) {
        std::cout << std::setw(13) 
                  << std::fixed << std::setprecision(1)
                  << components_static_energy.at(i) << ",";
    }
    std::cout << std::setw(13) 
              << std::fixed << std::setprecision(1)
              << total_static_energy << std::endl;
    // Print components' cycle
    std::cout << "      Cycle,";
    for(unsigned i = 0; i < components_cycle.size(); i++) {
        std::cout << std::setw(13) << components_cycle.at(i) << ",";
    }
    std::cout << std::setw(13) << total_cycle << std::endl;
    std::cout << std::endl;
    return;
}

void analyzer_t::reset() {
    components_energy.clear();
    components_static_energy.clear();
    components_cycle.clear();
    total_energy = 0;
    total_static_energy = 0;
    total_cycle  = 0;
    accelerator->reset(); // Reset accelerator's dataflow
    return;
}

float analyzer_t::get_total_cost(metric_type_t  metric_) {
    float rtn = 0;
    switch(metric_) {
        case metric_type_t::ENERGY:
            rtn = total_energy;
            break;
        case metric_type_t::CYCLE:
            rtn = total_cycle;
            break;
        default:
            std::cerr << "Error invalid metric" << std::endl;
            exit(0);
    }
    return rtn;
}
float analyzer_t::get_target_level_cost(unsigned idx_, metric_type_t metric_) {
    float rtn = 0;
    // Set Scheduling table row index
    unsigned lower_idx = idx_;
    unsigned upper_idx = scheduling_table.get_above_buffer_pos(idx_);
    if(metric_ == metric_type_t::ENERGY) {
        // Compute energy consumption when lower level buffer transfers
        // data tile to upper level buffer
        rtn += get_energy_consumption(lower_idx, 
                                      direction_type_t::UPPER);
        // Compute energy consumption when upper level buffer transfers
        // data tile to lower level buffer
        rtn += get_energy_consumption(upper_idx, 
                                      direction_type_t::LOWER);
        // If upper idx is the top temporal level component
        if(upper_idx == 0) {
            rtn += get_energy_consumption(upper_idx, 
                                          direction_type_t::UPPER);
        }
    }
    else if(metric_ == metric_type_t::CYCLE) {
        rtn += get_cycle_consumption(lower_idx, 
                                     direction_type_t::UPPER);
        // If upper idx is the top temporal level component
        if(upper_idx == 0) {
            rtn += get_cycle_consumption(upper_idx, 
                                         direction_type_t::UPPER);
        }
        // MAC cycle
        rtn += scheduling_table.get_num_mac_operations()
             / accelerator->get_active_MACs();
    }
    else { std::cerr << "Error invalid metric type" << std::endl; exit(0); }
    return rtn;
}
unsigned analyzer_t::get_target_level_factorization(unsigned idx_) {
    return get_factorization_degrees(idx_);
}
unsigned analyzer_t::get_active_chips() {
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
unsigned analyzer_t::get_tile_size(unsigned idx_, data_type_t data_type_) {
    unsigned rtn;
    std::vector<unsigned> tile_size;
    unsigned component_idx = scheduling_table.get_component_index(idx_);
    // Get tile size 
    tile_size = accelerator->get_allocated_size(component_idx, 
                                                direction_type_t::UPPER);
    assert(tile_size.size() == (unsigned)data_type_t::SIZE);
    rtn = tile_size.at((unsigned)data_type_);
    return rtn;
}
unsigned analyzer_t::get_access_count(unsigned idx_, data_type_t data_type_) {
    unsigned rtn;
    std::vector<unsigned> access_count;
    unsigned component_idx = scheduling_table.get_component_index(idx_);
    // Get access count and unit access energy 
    access_count = accelerator->get_tile_access_count(component_idx,
                                                      operation_type_t::READ,
                                                      direction_type_t::UPPER);
    assert(access_count.size() == (unsigned)data_type_t::SIZE);
    rtn = access_count.at((unsigned)data_type_);
    return rtn;
}
float analyzer_t::get_energy_consumption(unsigned idx_,
                                         direction_type_t direction_) {
    float rtn = 0.0;
    
    std::vector<unsigned> tile_size(3,0);
    std::vector<unsigned> access_count(3,0);
    std::vector<float> unit_energy(3,0);
    const unsigned dim_x = (unsigned)dimension_t::DIM_X;
    const unsigned dim_y = (unsigned)dimension_t::DIM_Y;

    unsigned component_idx = scheduling_table.get_component_index(idx_);
    for (unsigned op_type = 0; op_type < (unsigned)operation_type_t::SIZE; op_type++) {
        // Get tile size and access count and unit access energy 
        tile_size = accelerator->get_allocated_size(component_idx, direction_);
        access_count = accelerator->get_tile_access_count(component_idx, 
                                                         (operation_type_t)op_type, 
                                                          direction_);
        unit_energy = accelerator->get_component_energy(component_idx);
        // Calculate energy consumption
        for (unsigned i = 0; i < (unsigned)data_type_t::SIZE; i++) {
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
                                        direction_type_t direction_) {
    float rtn = 0.0;

    std::vector<unsigned> tile_size(3,0);
    std::vector<unsigned> access_count(3,0);
    std::vector<float> unit_cycle(3,0);
    unsigned bandwidth = 1;

    unsigned component_idx = scheduling_table.get_component_index(idx_);
    for (unsigned op_type = 0; op_type < (unsigned)operation_type_t::SIZE; op_type++) {
        // Get tile size and access count and unit access energy 
        tile_size = accelerator->get_allocated_size(component_idx, direction_);
        access_count = accelerator->get_tile_access_count(component_idx, 
                                                         (operation_type_t)op_type, 
                                                          direction_);
        unit_cycle = accelerator->get_component_cycle(component_idx);
        bandwidth  = accelerator->get_bandwidth(component_idx);
        // Calculate cycle count
        for (unsigned i = 0; i < (unsigned)data_type_t::SIZE; i++) {
            assert(tile_size.size() == (unsigned)data_type_t::SIZE
                && access_count.size() == (unsigned)data_type_t::SIZE
                && unit_cycle.size() == (unsigned)data_type_t::SIZE);
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
                    num_factors.push_back(i);
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
