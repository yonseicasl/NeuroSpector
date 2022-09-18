#include <climits>
#include <iomanip>

#include "utils.h"
#include "scheduling_table.h"

scheduling_table_t::scheduling_table_t() {
}
scheduling_table_t::scheduling_table_t(accelerator_t *accelerator_,
                                       network_t *network_)
    :accelerator(accelerator_),
     network(network_),
     layer_name(""),
     layer_index(0),
     layer_parameters((unsigned)parameter_t::SIZE, 1),
     num_mac_operations(1),
     num_table_rows(0),
     num_table_cols((unsigned)parameter_t::SIZE) {
}
scheduling_table_t::scheduling_table_t(accelerator_t *accelerator_,
                                       network_t *network_,
                                       const std::string& scheduling_table_pth_)
    :accelerator(accelerator_),
     network(network_),
     layer_name(""),
     layer_index(0),
     layer_parameters((unsigned)parameter_t::SIZE, 1),
     num_mac_operations(1),
     num_table_rows(0),
     num_table_cols((unsigned)parameter_t::SIZE) {
         parser_t parser;
         parser.cfgparse(scheduling_table_pth_);

         init_table_rows();
         init_mapping_values();
         fill_out_mapping_values(parser);
}
scheduling_table_t::~scheduling_table_t() {
}
scheduling_table_t::scheduling_table_t(const scheduling_table_t &scheduling_table_) 
    :accelerator(scheduling_table_.accelerator),
     network(scheduling_table_.network),
     layer_name(scheduling_table_.layer_name),
     layer_index(scheduling_table_.layer_index),
     layer_parameters(scheduling_table_.layer_parameters),
     num_mac_operations(scheduling_table_.num_mac_operations),
     num_table_rows(scheduling_table_.num_table_rows), 
     num_table_cols(scheduling_table_.num_table_cols), 
     mapping_values(scheduling_table_.mapping_values), 
     row_names(scheduling_table_.row_names), 
     row_types(scheduling_table_.row_types),
     row_dataflows(scheduling_table_.row_dataflows) {
}
// Initialize scheduling table
void scheduling_table_t::init() {
    std::clog << "[message] initialize scheduling table" << std::endl;
    // Initialize scheduling table rows
    init_table_rows();
    // Initialize mapping values in scheduling table
    init_mapping_values();
}
// Initialize scheduling table rows
void scheduling_table_t::init_table_rows() {
    std::clog << "[message] initialize table rows" << std::endl;

    for(unsigned i = 0; i < (unsigned)component_t::SIZE; i++) {
        // If the component is undefined in accelerator then insert virtual row
        if(accelerator->get_name((component_t)i) == "virtual") {
            add_virtual_component(accelerator->get_type((component_t)i));
        }
        else {
            // Skip the component's size is 1, it is considered as virtual row
            if(accelerator->is_unit_component((component_t)i)) {
                add_virtual_component(accelerator->get_type((component_t)i));
            }
            else {
                if(i == (unsigned)component_t::MAC_X 
                || i == (unsigned)component_t::PE_X 
                || i == (unsigned)component_t::CHIP_X) {
                    row_names.emplace_back(accelerator->get_name((component_t)i) + "(X)");
                }
                else if(i == (unsigned)component_t::MAC_Y 
                    || i == (unsigned)component_t::PE_Y 
                    || i == (unsigned)component_t::CHIP_Y) {
                    row_names.emplace_back(accelerator->get_name((component_t)i) + "(Y)");
                }
                else {
                    row_names.emplace_back(accelerator->get_name((component_t)i));
                }

                row_types.emplace_back(accelerator->get_type((component_t)i));
                row_dataflows.emplace_back(accelerator->get_dataflow((component_t)i));
                num_table_rows++;
            }
        }
    }
}
// Initialize mapping values in scheduling table
void scheduling_table_t::init_mapping_values() {
    num_table_cols = unsigned(parameter_t::SIZE); 
    std::cerr << "[message] initialize mapping_values" << std::endl;
    // Based on the value which are generated in init_table function, 
    // initialize all mapping values to 1.
    mapping_values.resize(num_table_rows * num_table_cols);
    fill(mapping_values.begin(), mapping_values.end(), 1);
}
// Get buffer index one level above
unsigned scheduling_table_t::get_above_buffer_pos(unsigned pos_) const {
    unsigned rtn = pos_;
    for(int i = (int)rtn-1; i >= 0; i--) {
        rtn--;
        if(row_types.at(i) == reuse_t::TEMPORAL) { break; }
    }
    return rtn;
}
// Get buffer index one level below
unsigned scheduling_table_t::get_below_buffer_pos(unsigned pos_) const {
    unsigned rtn = pos_;
    for(unsigned i = pos_+1; i < get_num_rows(); i++) {
        rtn++;
        if(get_component_type(i) == reuse_t::TEMPORAL) { break; }
    }
    return rtn;
}
// Get spatial-level (dim-X) index one level above
unsigned scheduling_table_t::get_above_spatial_level_pos(unsigned pos_) const {
    unsigned rtn = pos_;
    for(int i = (int)rtn-1; i >= 0; i--) {
        rtn--;
        if(row_types.at(i) == reuse_t::SPATIAL
        && row_names.at(i).find("(X)") != std::string::npos) { break; }
    }
    return rtn;
}
// Get total number of rows of scheduling table
unsigned scheduling_table_t::get_num_rows() const { 
   return num_table_rows; 
}
// Get product of corrleation parameters
unsigned scheduling_table_t::get_correlation_product(int idx_, 
                                                      correlation_t correlation_) {
    unsigned rtn = 1;
    std::vector<parameter_t> param_list;
    switch(correlation_) {
        case correlation_t::WO:
            param_list.emplace_back(parameter_t::K);
            for(auto it = param_list.begin(); it != param_list.end(); ++it) {
                // i from one level below a buffer downwards to DRAM
                for(int row_idx = idx_; row_idx < (int)get_num_rows(); row_idx++) {
                    if(get_component_type(row_idx) == reuse_t::SPATIAL) { continue; }
                    rtn *= get_mapping_value(row_idx, (unsigned)*it);
                }
            }
            break;
        case correlation_t::OI:
            param_list.emplace_back(parameter_t::B);
            param_list.emplace_back(parameter_t::P);
            param_list.emplace_back(parameter_t::Q);
            for(auto it = param_list.begin(); it != param_list.end(); ++it) {
                // i from one level below a buffer downwards to DRAM
                for(int row_idx = idx_; row_idx < (int)get_num_rows(); row_idx++) {
                    if(get_component_type(row_idx) == reuse_t::SPATIAL) { continue; }
                    rtn *= get_mapping_value(row_idx, (unsigned)*it);
                }
            }
            break;
        case correlation_t::IW:
            param_list.emplace_back(parameter_t::C);
            param_list.emplace_back(parameter_t::R);
            param_list.emplace_back(parameter_t::S);
            for(auto it = param_list.begin(); it != param_list.end(); ++it) {
                // i from one level below a buffer downwards to DRAM
                for(int row_idx = idx_; row_idx < (int)get_num_rows(); row_idx++) {
                    if(get_component_type(row_idx) == reuse_t::SPATIAL) { continue; }
                    rtn *= get_mapping_value(row_idx, (unsigned)*it);
                }
            }
            break;
        case correlation_t::IWO:
            param_list.emplace_back(parameter_t::G);
            for(auto it = param_list.begin(); it != param_list.end(); ++it) {
                // i from one level below a buffer downwards to DRAM
                for(int row_idx = idx_; row_idx < (int)get_num_rows(); row_idx++) {
                    if(get_component_type(row_idx) == reuse_t::SPATIAL) { continue; }
                    rtn *= get_mapping_value(row_idx, (unsigned)*it);
                }
            }
            break;
        default:
            exit(0);
    }
    return rtn;
}
// Get product of dataflow irrelevant parameters 
unsigned scheduling_table_t::get_dataflow_irrelevant_params_product(int idx_) {
    unsigned rtn = 1;
    unsigned upper_level_idx = get_above_buffer_pos(idx_);
    dataflow_t dataflow = get_dataflow(upper_level_idx);
    switch(dataflow) {
        case dataflow_t::IS:
            rtn *= get_mapping_value(idx_, (unsigned)parameter_t::K);
            break;
        case dataflow_t::WS:
            rtn *= get_mapping_value(idx_, (unsigned)parameter_t::B)
                 * get_mapping_value(idx_, (unsigned)parameter_t::P)
                 * get_mapping_value(idx_, (unsigned)parameter_t::Q);
            break;
        case dataflow_t::OS:
            rtn *= get_mapping_value(idx_, (unsigned)parameter_t::C)
                 * get_mapping_value(idx_, (unsigned)parameter_t::R)
                 * get_mapping_value(idx_, (unsigned)parameter_t::S);
            break;
        default:
            break;
    }
    return rtn;
}
bool* scheduling_table_t::get_bypass(unsigned idx_) const {
    bool* rtn;
    rtn = accelerator->get_bypass((component_t)idx_);
    return rtn;
}
dataflow_t scheduling_table_t::get_dataflow(unsigned idx_) const {
    return row_dataflows.at(idx_);
}
// Get target component name
std::string scheduling_table_t::get_component_name(unsigned idx_) const {
    return row_names.at(idx_);
}
// Get component type of target row 
reuse_t scheduling_table_t::get_component_type(unsigned idx_) const {
    return row_types.at(idx_);
}
// Get mapping values of a component level
std::vector<unsigned> scheduling_table_t::get_row_values(unsigned idx_) const {
    std::vector<unsigned> rtn;
    unsigned row = idx_;
    for(unsigned col = 0; col < num_table_cols; col++) {
        rtn.emplace_back(get_mapping_value(row, col));
    }
    assert(rtn.size() == num_table_cols);
    return rtn;
}
// Get mapping value at (row, col) 
unsigned scheduling_table_t::get_mapping_value(unsigned row_, unsigned col_) const {
    unsigned rtn;
    unsigned idx = row_ * num_table_cols + col_;
    assert(idx < mapping_values.size());
    rtn = mapping_values.at(idx);
    return rtn;
}
// Get current layer's index
unsigned scheduling_table_t::get_layer_index() {
    return layer_index;
}
std::vector<unsigned> scheduling_table_t::get_layer_parameters() {
    return layer_parameters;
}
// Get total number of operations in a layer 
float scheduling_table_t::get_num_mac_operations() {
    return num_mac_operations;
}
// Add virtual component
void scheduling_table_t::add_virtual_component(reuse_t component_type_) {
    row_names.emplace_back("virtual");
    row_types.emplace_back(component_type_);
    row_dataflows.emplace_back(dataflow_t::NONE);
    num_table_rows++;
}
// Check the component is virtual
bool scheduling_table_t::is_virtual(unsigned idx_) {
    return row_names.at(idx_) == "virtual";
}
// Update DRAM mapping values to layer parameters
void scheduling_table_t::load_dnn_layer(unsigned idx_) {
    unsigned dram_pos = num_table_rows - 1; // DRAM should be placed in lowest level
    layer_name       = network->get_layer_name(idx_);
    layer_parameters = network->get_layer_parameters(idx_);
    layer_index      = idx_;
    update_row(dram_pos, layer_parameters);
    
    num_mac_operations = 1; 
    for(unsigned i = 0; i <layer_parameters.size(); i++) {
        num_mac_operations *= layer_parameters.at(i);
    }
    return;
}
// 
void scheduling_table_t::clear_set_of_rows(unsigned begin_, unsigned end_) {
    std::vector<unsigned> mapping_values_set = {1, 1, 1, 1, 1, 1, 1, 1};
    for(unsigned row_idx = begin_; row_idx < end_ + 1; row_idx++) {
        if(is_virtual(row_idx)) continue;
        update_row(row_idx, mapping_values_set); 
    }
    return;
}
// Update mapping values of a row-set in scheduling table
void scheduling_table_t::update_set_of_rows(unsigned begin_, unsigned end_, 
                        std::vector<std::vector<unsigned>> mapping_values_set_) {
    unsigned mapping_set_idx = 0;
    for(unsigned row_idx = begin_; row_idx < end_ + 1; row_idx++) {
        if(is_virtual(row_idx)) continue;
        update_row(row_idx, mapping_values_set_.at(mapping_set_idx)); 
        mapping_set_idx++;
    }
    return;
}
// Update mapping values of row in scheduling table
void scheduling_table_t::update_row(unsigned component_pos_,
                                    std::vector<unsigned> mapping_values_) {
    unsigned row_idx = component_pos_ * num_table_cols;
    for(unsigned i = 0; i < mapping_values_.size(); i++) {
        update_mapping_value(row_idx + i, mapping_values_.at(i));
    }
    return;
}
// Update mapping value in scheduling table
void scheduling_table_t::update_mapping_value(unsigned dst_, unsigned val_) {
    mapping_values.at(dst_) = val_;
    return;
}
// Update each temporal components dataflow
void scheduling_table_t::update_dataflow(std::vector<dataflow_t> dataflow_) {
    auto iter = dataflow_.begin();
    for(unsigned i = 0; i < get_num_rows() - 1; i++) {
        if(get_component_type(i) == reuse_t::TEMPORAL 
        && get_component_name(i) != "virtual") {
            row_dataflows.at(i) = *iter;
            iter++;
        }
    }
    return;
}
// Print out scheduling table
void scheduling_table_t::print_stats() {
    std::string dataflow_type[4] = {"-", "IS", "WS", "OS"};
    std::cout << "|------------------------------------------------------------------------------|------|" << std::endl;
    std::cout << "|Correlations  |  W/O  |          O/I          |          I/W          | I/W/O | DATA |" << std::endl;
    std::cout << "|Parameters    |   K   |   B   |   P   |   Q   |   C   |   R   |   S   |   G   | FLOW |" << std::endl;
    std::cout << "|------------------------------------------------------------------------------|------|" << std::endl;
    for(unsigned row = 0; row < num_table_rows; row++) {
        if(!is_virtual(row)) {
            std::cout << "|";
            std::cout.width(14);
            std::cout << std::left << row_names.at(row); 
            std::cout << "|";
            for(unsigned col = 0; col < num_table_cols; col++) {
                std::cout.width(6);
                std::cout << std::right 
                          << mapping_values.at((row * num_table_cols) + col) 
                          << " |";
            }
            // Print dataflow
            if(get_component_type(row) == reuse_t::TEMPORAL) {
                std::cout << std::setw(5) 
                          << dataflow_type[(unsigned)get_dataflow(row)]
                          << " |";
            }
            else { std::cout << std::setw(5) << "-" << " |"; }
            std::cout << std::endl;
        }
    }
    std::cout << "|------------------------------------------------------------------------------|------|" << std::endl;
}

void scheduling_table_t::print_stats(std::ofstream &output_file_) {
    std::string dataflow_type[4] = {"-", "IS", "WS", "OS"};
    output_file_ << "|------------------------------------------------------------------------------|------|" << std::endl;
    output_file_ << "|Correlations  |  W/O  |          O/I          |          I/W          | I/W/O | DATA |" << std::endl;
    output_file_ << "|Parameters    |   K   |   B   |   P   |   Q   |   C   |   R   |   S   |   G   | FLOW |" << std::endl;
    output_file_ << "|------------------------------------------------------------------------------|------|" << std::endl;
    for(unsigned row = 0; row < num_table_rows; row++) {
        if(!is_virtual(row)) {
            output_file_ << "|";
            output_file_.width(14);
            output_file_ << std::left << row_names.at(row); 
            output_file_ << "|";
            for(unsigned col = 0; col < num_table_cols; col++) {
                output_file_.width(6);
                output_file_ << std::right 
                          << mapping_values.at((row * num_table_cols) + col) 
                          << " |";
            }
            // Print dataflow
            if(get_component_type(row) == reuse_t::TEMPORAL) {
                output_file_ << std::setw(5) 
                          << dataflow_type[(unsigned)get_dataflow(row)]
                          << " |";
            }
            else { output_file_ << std::setw(5) << "-" << " |"; }
            output_file_ << std::endl;
        }
    }
    output_file_ << "|------------------------------------------------------------------------------|------|" << std::endl;
}

// Overload != operation to compare to scheduling table
bool scheduling_table_t::operator!=(const scheduling_table_t& scheduling_table_) {
    bool rtn = false;
    std::vector<unsigned> mapping_values;
    // Compare mapping values for all rows and cols
    for(unsigned row = 0; row < scheduling_table_.get_num_rows(); row++) {
        mapping_values = scheduling_table_.get_row_values(row);
        for(unsigned col = 0; col < mapping_values.size(); col++) {
            // If mapping value are different, break the loop
            if(mapping_values.at(col) != get_mapping_value(row, col)) { 
                rtn = true; 
                break; 
            }
        }
        if(rtn == true) { break; }
    }
    return rtn;
}
// Fill out scheduling table based on scheduling table file
void scheduling_table_t::fill_out_mapping_values(const parser_t parser_) {
    std::string target_level_name;
    std::string target_level_mapping_str;
    std::vector<unsigned> target_level_mapping;
    for(unsigned i = 0; i < parser_.sections.size(); i++) {
        section_config_t section_config = parser_.sections[i];
        if(section_config.name == "SCHEDULING_TABLE") {
            for(unsigned i = 0; i < get_num_rows(); i++) {
                target_level_name = get_component_name(i);
                if(target_level_name == "virtual") {
                    target_level_mapping_str.assign((unsigned)parameter_t::SIZE, 1);
                }
                else {
                    section_config.get_setting(target_level_name, 
                                            &target_level_mapping_str);
                    if(target_level_mapping_str.empty()) {
                        std::cerr << "Please enter the mapping value of "
                                << target_level_name << std::endl;
                        exit(0);
                    }
                    target_level_mapping = comma_to_vector(target_level_mapping_str);
                }
                update_row(i, target_level_mapping);
                target_level_mapping.clear();
            }
        }
        else {
            section_config.get_setting("layer_name", &layer_name);
            layer_index = network->get_layer_index(layer_name);
        }
    }
    // Update layer_parameters
    std::string parameter[(unsigned)parameter_t::SIZE] = { "K", "B", "P", "Q", "C", "R", "S", "G" };
    for(unsigned i = 0; i < (unsigned)parameter_t::SIZE; i++) {
        layer_parameters.at(i) = get_column_wise_product((parameter_t)i, 0, get_num_rows() - 1);
        if(layer_parameters.at(i) != network->get_layer_parameters(layer_index).at(i)) {
            std::cerr << "Error: Violate parameter constraint; column "
                      << parameter[i] 
                      << std::endl;
            exit(0);
        }
        num_mac_operations *= layer_parameters.at(i);
    }
    return;
}
unsigned scheduling_table_t::get_column_wise_product(parameter_t param_, 
                                          unsigned begin_, unsigned end_) {
    unsigned rtn = 1;
    for(unsigned row_idx = begin_; row_idx < end_+1; row_idx++) {
        rtn *= get_mapping_value(row_idx, (unsigned)param_);
    }
    return rtn;
}
std::vector<unsigned> scheduling_table_t::get_row_wise_product(int begin_, 
                                                               int end_) {
    std::vector<unsigned> rtn;
    unsigned tmp = 1;
    for(unsigned col_idx = 0; col_idx < (unsigned)parameter_t::SIZE; col_idx++) {
        tmp = get_column_wise_product((parameter_t)col_idx, begin_, end_);
        rtn.emplace_back(tmp);
        tmp = 1;
    }

    return rtn;
}
// Get a result that multiplying all mapping values existed in temporal rows
unsigned scheduling_table_t::get_temporal_row_wise_product(int begin_, 
                                                           int end_) {
    unsigned rtn = 1;
    for(int row_idx = begin_; row_idx <= end_; row_idx++) {
        if(row_types.at(row_idx) == reuse_t::SPATIAL) continue;
        rtn *= get_row_product(row_idx);
    }
    return rtn;
}
// Get a result that multiplying all mapping values existed in a row
unsigned scheduling_table_t::get_row_product(int pos_) {
    unsigned rtn = 1;
    for(unsigned col_idx = 0; col_idx < (unsigned)parameter_t::SIZE; col_idx++) {
            rtn *= get_mapping_value(pos_, col_idx);
    }
    return rtn;
}
