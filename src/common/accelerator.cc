#include <cassert>
#include <numeric>

#include "accelerator.h"

#define BYTE 8

accelerator_t::accelerator_t(const std::string &cfg_path_) 
    : acc_cfg_path(cfg_path_),
      precision(8),
      mac_operation_energy(1.0) {
        // Initialize component list
        for(unsigned i = 0; i < (unsigned)component_t::SIZE; i++) {
            component_list[i] = nullptr;
        }
        // Parse the configuration file
        parser_t parser;
        parser.cfgparse(acc_cfg_path);

        // Generate components
        generate_components(parser);
        calc_accelerator_max_power();
        calc_dram_power();
}
// Copy constructor 
accelerator_t::accelerator_t(const accelerator_t &accelerator_) 
    : acc_cfg_path(accelerator_.acc_cfg_path),
      precision(accelerator_.precision),
      mac_operation_energy(accelerator_.mac_operation_energy) {
        // Parse the configuration file
        parser_t parser;
        parser.cfgparse(acc_cfg_path);

        // Generate components
        generate_components(parser);
        calc_accelerator_max_power();
        calc_dram_power();
}
accelerator_t::~accelerator_t() {
    delete register_file;
    delete mac_array;
    delete local_buffer;
    delete pe_array;
    delete global_buffer;
    delete multi_chips;
    delete dram;
};

// Generate components that make up accelerator
void accelerator_t::generate_components(const parser_t parser) {
    register_file = new temporal_component_t;
    mac_array     = new spatial_component_t;
    local_buffer  = new temporal_component_t;
    pe_array      = new spatial_component_t;
    global_buffer = new temporal_component_t;
    multi_chips   = new spatial_component_t;
    dram          = new temporal_component_t;
    for(unsigned i = 0; i < parser.sections.size(); i++) {
        section_config_t section_config = parser.sections[i];
        if(section_config.name == "ACCELERATOR") {
            std::string precision_str;
            section_config.get_setting("name", &name);
            section_config.get_setting("precision", &precision_str);
            // Convert string value to int
            if(precision_str.length() != 0) precision = 0;
            for(unsigned i = 0; i < unsigned(precision_str.length()); i++) {
                if(precision_str[i] > 47 && precision_str[i] < 58)
                    precision = precision * 10 + precision_str[i] - 48;
            }
            section_config.get_setting("frequency", &clock_frequency);
            section_config.get_setting("unit_mac_energy", &mac_operation_energy);
            section_config.get_setting("unit_mul_energy", &mul_operation_energy);
            section_config.get_setting("unit_add_energy", &add_operation_energy);
            section_config.get_setting("unit_mac_static", &mac_static_power);
        }
        else {
            if(section_config.name == "REGISTER") {
                init_temporal_component(register_file, section_config);
                component_list[(unsigned)component_t::REG] = register_file;
            }
            else if(section_config.name == "MAC_ARRAY") {
                init_spatial_component(mac_array, section_config);
                component_list[(unsigned)component_t::MAC_X] = mac_array;
                component_list[(unsigned)component_t::MAC_Y] = mac_array;
            }
            else if(section_config.name == "LOCAL_BUFFER") {
                init_temporal_component(local_buffer, section_config);
                component_list[(unsigned)component_t::LB] = local_buffer;
            }
            else if(section_config.name == "PE_ARRAY") {
                init_spatial_component(pe_array, section_config);
                component_list[(unsigned)component_t::PE_X] = pe_array;
                component_list[(unsigned)component_t::PE_Y] = pe_array;
            }
            else if(section_config.name == "GLOBAL_BUFFER") {
                init_temporal_component(global_buffer, section_config);
                component_list[(unsigned)component_t::GB] = global_buffer;
            }
            else if(section_config.name == "MULTI_CHIPS") {
                init_spatial_component(multi_chips, section_config);
                component_list[(unsigned)component_t::CHIP_X] = multi_chips;
                component_list[(unsigned)component_t::CHIP_Y] = multi_chips;
            }
            else if(section_config.name == "DRAM") {
                init_temporal_component(dram, section_config);
                component_list[(unsigned)component_t::DRAM] = dram;
            }
        }
    }
}
bool accelerator_t::check_component_exist(component_t comp_) {
    return (component_list[(unsigned)comp_] != nullptr);
}
std::string accelerator_t::get_acc_name() {
    return name;
}
std::string accelerator_t::get_name(component_t comp_) {
    std::string rtn;
    if(component_list[(unsigned)comp_] != nullptr) {
        rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->name;
    }
    else {
        rtn = "virtual";
    }
    return rtn;
}
reuse_t accelerator_t::get_type(component_t comp_) const {
    reuse_t rtn;
    if(comp_ == component_t::REG || comp_ == component_t::LB
    || comp_ == component_t::GB  || comp_ == component_t::DRAM) {
        rtn = reuse_t::TEMPORAL;
    }
    else {
        rtn = reuse_t::SPATIAL;
    }
    return rtn;
}
// Get component size
std::vector<float> accelerator_t::get_size(component_t comp_) {
    std::vector<float> rtn;
    if (get_type(comp_) == reuse_t::TEMPORAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->size;
        }
        else {
            rtn.emplace_back(precision/BYTE);  // Input
            rtn.emplace_back(precision/BYTE);  // Weight
            rtn.emplace_back(precision/BYTE);  // Output
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component dataflow
dataflow_t accelerator_t::get_dataflow(component_t comp_) {
    dataflow_t rtn = dataflow_t::NONE;
    if (get_type(comp_) == reuse_t::TEMPORAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->dataflow;
        }
        else {

        }
    }
    return rtn;
}
// Get component bitwidth
float* accelerator_t::get_bitwidth(component_t comp_) {
    float* rtn = nullptr;
    if (get_type(comp_) == reuse_t::TEMPORAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->bitwidth.data();
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
float accelerator_t::get_bitwidth(component_t comp_, data_t dtype_) {
    float rtn = precision;
    if (get_type(comp_) == reuse_t::TEMPORAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            if(dtype_ == data_t::INPUT) {
                rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->bitwidth_i;
            }
            else if(dtype_ == data_t::WEIGHT) {
                rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->bitwidth_w;
            }
            else if(dtype_ == data_t::OUTPUT) {
                rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->bitwidth_o;
            }
            else {
                std::cerr << "Invalid data type in get_bitwidth()" << std::endl;
                exit(0);
            }
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get spatial component's radix_degree
float accelerator_t::get_radix_degree(component_t comp_, data_t dtype_) {
    float rtn = 0;
    if (get_type(comp_) == reuse_t::SPATIAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            if(dtype_ == data_t::INPUT) {
                rtn = ((spatial_component_t*)component_list[(unsigned)comp_])->radix_degree_i;
            }
            else if(dtype_ == data_t::WEIGHT) {
                rtn = ((spatial_component_t*)component_list[(unsigned)comp_])->radix_degree_w;
            }
            else if(dtype_ == data_t::OUTPUT) {
                rtn = ((spatial_component_t*)component_list[(unsigned)comp_])->radix_degree_o;
            }
            else {
                std::cerr << "Invalid data type in get_radix_degree()" << std::endl;
                exit(0);
            }
        }
        else {
            // If the spatial component is undefined
            rtn = 1;
        }
    }
    else {
        std::cerr << "Invalid function access; target component is temporal" 
                  << std::endl;
        exit(0);
    }
    if(rtn == 0) { std::cerr << "Error in get_radix_degree" 
                             << ". return value is zero" << std::endl; }
    return rtn;
}
// Get component energy
float* accelerator_t::get_energy(component_t comp_) const {
    float* rtn = nullptr;
    if (get_type(comp_) == reuse_t::TEMPORAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            auto comp = (temporal_component_t*)component_list[(unsigned)comp_];
            rtn = comp->unit_energy.data();
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component static power
float* accelerator_t::get_static(component_t comp_) const {
    float* rtn = nullptr;
    if (get_type(comp_) == reuse_t::TEMPORAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            auto comp = (temporal_component_t*)component_list[(unsigned)comp_];
            rtn = comp->unit_static.data();
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get component cycle
float* accelerator_t::get_cycle(component_t comp_) const {
    float* rtn = nullptr;
    if (get_type(comp_) != reuse_t::TEMPORAL) {
        if(component_list[(unsigned)comp_] != nullptr) {
            auto comp = (temporal_component_t*)component_list[(unsigned)comp_];
            rtn = comp->unit_cycle.data();
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
float accelerator_t::get_data_access_energy(component_t comp_, data_t dtype_) const {
    if(component_list[(unsigned)comp_] != nullptr) {
        auto comp = (temporal_component_t*)component_list[(unsigned)comp_];
        return comp->unit_energy_per_dtype[(unsigned)dtype_];
    }
    return 0.0f;
}
// Static doesn't need unit cost for each data access
float accelerator_t::get_comp_level_static(component_t comp_) const {
    float static_per_comp_level = 0.0f;
    if(component_list[(unsigned)comp_] != nullptr) {
        auto comp = (temporal_component_t*)component_list[(unsigned)comp_];
        for(unsigned i = 0; i < comp->unit_static.size(); i++) {
            static_per_comp_level += comp->unit_static.at(i);
        }
        return static_per_comp_level;
    }
    return static_per_comp_level;
}
float accelerator_t::get_data_access_cycle(component_t comp_, data_t dtype_) const {
    if(component_list[(unsigned)comp_] != nullptr) {
        auto comp = (temporal_component_t*)component_list[(unsigned)comp_];
        return comp->unit_cycle_per_dtype[(unsigned)dtype_];
    }
    return 0.0f;
}


// Get component bypass
bool* accelerator_t::get_bypass(component_t comp_) {
    bool* rtn = nullptr;
    if(comp_ == component_t::REG || comp_ == component_t::LB
    || comp_ == component_t::GB  || comp_ == component_t::DRAM) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->bypass;
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}
// Get separated data
bool* accelerator_t::get_separated_data(component_t comp_) {
    bool* rtn = nullptr;
    if(comp_ == component_t::REG || comp_ == component_t::LB
    || comp_ == component_t::GB  || comp_ == component_t::DRAM) {
        if(component_list[(unsigned)comp_] != nullptr) {
            rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->separated_data;
        }
    }
    else {
        std::cerr << "Invalid function access; target component is spatial" 
                  << std::endl;
        exit(0);
    }
    return rtn;
}

// Get Total number of MAC components in accelerator
unsigned accelerator_t::get_total_num_MACs() {
    unsigned rtn = 1;
    if(component_list[(unsigned)component_t::MAC_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)component_t::MAC_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)component_t::MAC_Y])->dim_y;
    }
    if(component_list[(unsigned)component_t::PE_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)component_t::PE_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)component_t::PE_Y])->dim_y;

    }
    if(component_list[(unsigned)component_t::CHIP_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)component_t::CHIP_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)component_t::CHIP_Y])->dim_y;
    }
    return rtn;
}
// Get Total number of PE components in accelerator
unsigned accelerator_t::get_total_num_PEs() {
    unsigned rtn = 1;
    if(component_list[(unsigned)component_t::PE_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)component_t::PE_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)component_t::PE_Y])->dim_y;

    }
    if(component_list[(unsigned)component_t::CHIP_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)component_t::CHIP_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)component_t::CHIP_Y])->dim_y;
    }
    return rtn;
}
// Get Total number of CHIP components in accelerator
unsigned accelerator_t::get_total_num_chips() {
    unsigned rtn = 1;
    if(component_list[(unsigned)component_t::CHIP_X] != nullptr) {
        rtn *= ((spatial_component_t*)component_list[(unsigned)component_t::CHIP_X])->dim_x
             * ((spatial_component_t*)component_list[(unsigned)component_t::CHIP_Y])->dim_y;
    }
    return rtn;
}
// Get spatially arranged component size
unsigned accelerator_t::get_array_size(component_t comp_, dimension_t dim_) {
    unsigned rtn = 1;
    if(component_list[(unsigned)comp_] == nullptr) {
        return 1;
    }
    if(dim_ == dimension_t::DIM_X) {
        rtn = ((spatial_component_t*)component_list[(unsigned)comp_])->dim_x;
    }
    else {
        rtn = ((spatial_component_t*)component_list[(unsigned)comp_])->dim_y;
    }
    return rtn;
}
// Get spatially arranged component constraints
std::string accelerator_t::get_array_constraint(component_t comp_, dimension_t dim_) {
    std::string rtn = "";
    if(component_list[(unsigned)comp_] == nullptr) {
        return "";
    }
    if(dim_ == dimension_t::DIM_X) {
        rtn = ((spatial_component_t*)component_list[(unsigned)comp_])->constraint_x;
    }
    else {
        rtn = ((spatial_component_t*)component_list[(unsigned)comp_])->constraint_y;
    }
    return rtn;
}
std::vector<unsigned> accelerator_t::get_memory_constraint(component_t comp_) {
    // Early return if component doesn't exist
    if (!component_list[static_cast<unsigned>(comp_)]) {
        return {};  // Return empty vector using uniform initialization
    }
    std::vector<unsigned> rtn;
    std::vector<std::string> rtn_str;
    rtn_str = split(((temporal_component_t*)component_list[(unsigned)comp_])->constraint, ',');
    // Convert all mapping values to unsigned data
    for(unsigned i = 0; i < rtn_str.size(); i++) {
        rtn.push_back((unsigned)std::stoi(rtn_str.at(i)));
    }
    return rtn;
}
void accelerator_t::update_array_constraint(component_t comp_,
                                            std::string cnst_x_,
                                            std::string cnst_y_) {
    std::string& cnst_dim_x 
        = ((spatial_component_t*)component_list[(unsigned)comp_])->constraint_x;
    std::string& cnst_dim_y 
        = ((spatial_component_t*)component_list[(unsigned)comp_])->constraint_y;
    cnst_dim_x = cnst_x_;
    cnst_dim_y = cnst_y_;
    std::clog << "[message] update PE array constraint for systolic array" << std::endl;
    return;
}
bool accelerator_t::is_unit_component(component_t comp_) {
    bool rtn = false;
    std::vector<float> buff_size;
    switch (comp_) {
        case component_t::REG:
        case component_t::LB:
        case component_t::GB:
            buff_size = get_size(comp_);
            if(buff_size.size() == 1) {
                rtn = (buff_size.back() == (precision * (unsigned)data_t::SIZE) / BYTE);
            }
            else {
                // The temporal component is treated as a unit component when the component can store only one data point or nothing
                rtn = (buff_size[(unsigned)data_t::INPUT] == (precision / BYTE)
                   || buff_size[(unsigned)data_t::INPUT] == 0);
                rtn = rtn && (buff_size[(unsigned)data_t::WEIGHT] == (precision / BYTE)
                   || buff_size[(unsigned)data_t::WEIGHT] == 0);
                rtn = rtn && (buff_size[(unsigned)data_t::OUTPUT] == (precision / BYTE)
                   || buff_size[(unsigned)data_t::OUTPUT] == 0);
            }
            break;
        case component_t::MAC_X:
            rtn = (get_array_size(component_t::MAC_X, dimension_t::DIM_X) == 1); 
            break;
        case component_t::MAC_Y:
            rtn = (get_array_size(component_t::MAC_Y, dimension_t::DIM_Y) == 1); 
            break;
        case component_t::PE_X:
            rtn = (get_array_size(component_t::PE_X, dimension_t::DIM_X) == 1); 
            break;
        case component_t::PE_Y:
            rtn = (get_array_size(component_t::PE_Y, dimension_t::DIM_Y) == 1);
            break;
        case component_t::CHIP_X:
            rtn = (get_array_size(component_t::CHIP_X, dimension_t::DIM_X)== 1);
            break;
        case component_t::CHIP_Y:
            rtn = (get_array_size(component_t::CHIP_Y, dimension_t::DIM_Y) == 1);
            break;
        default:
            break;
    }
    return rtn;
};

// Get required time per 1 cycle
float accelerator_t::get_clock_time() {
    /** Convert frequency to time (ns)
     *  ! Suppose Unit of Clock frequency (MHz) 
     *  Return value; Latency (ms) is calculated by 
     *  'Latency(ns) = (clock_frequency)^(-1) (us) * 10^3(kilo)'
     */
    float rtn = 0.0;
    rtn = (1.0 / (float)clock_frequency) * 1000;
    return rtn;
}
float accelerator_t::get_clock_frequency() {
    return (float)clock_frequency;
}
// Get theoretical maximum power consumption
float accelerator_t::get_theoretical_peak_power() {
    return theoretical_peak_power;
}
float accelerator_t::get_dram_power()  {
    return dram_power;
}
// Get MAC operation unit energy
float accelerator_t::get_mac_energy() {
    return mac_operation_energy;
}
float accelerator_t::get_mul_energy() {
    return mul_operation_energy;
}
float accelerator_t::get_add_energy() {
    return add_operation_energy;
}
// Get MAC operation unit static power
float accelerator_t::get_mac_static_power() {
    return mac_static_power;
}
// Get data precision of MAC
unsigned accelerator_t::get_precision() {
    return precision;
}
// Print specifiations of accelerator
void accelerator_t::print_spec() {
    std::cout << "==== Accelerator Spec ====" << std::endl;
    std::cout << "[  acc. name] " << name << std::endl;
    std::cout << "[  precision] " << get_precision() << std::endl;
    std::cout << "[  frequency] " << clock_frequency << std::endl;
    std::cout << "[ clock time] " << get_clock_time() << std::endl;
    std::cout << "[ MAC energy] " << get_mac_energy() << std::endl;
    std::cout << "[ MAC static] " << get_mac_static_power() << std::endl;
    for(unsigned i = 0; i < (unsigned)component_t::SIZE; i++) {
        if(component_list[i] == nullptr) { continue; }
        std::cout << std::endl;
        if(((temporal_component_t*)component_list[i])->type == reuse_t::TEMPORAL) {
            std::cout << "[ comp. name] " << ((temporal_component_t*)component_list[i])->name << std::endl;
            print_temporal_component((temporal_component_t*)component_list[i]);
        }
        else {
            std::cout << "[ comp. name] " << ((spatial_component_t*)component_list[i])->name << std::endl;
            print_spatial_component((spatial_component_t*)component_list[i]);
            i++;
        }
    }
    std::cout << "==========================" << std::endl;
    std::cout << "[      on-chip max power] " << get_theoretical_peak_power() << " mW" << std::endl;
    std::cout << "[off-chip mem. max power] " << get_dram_power() << " mW" << std::endl;
    std::cout << "[        total max power] " << get_theoretical_peak_power() + get_dram_power() << " mW" << std::endl;
    std::cout << "==========================" << std::endl;
}
void accelerator_t::print_spec(std::ofstream &output_file_) {
    output_file_ << "==== Accelerator Spec ====" << std::endl;
    output_file_ << "[  acc. name] " << name << std::endl;
    output_file_ << "[  precision] " << get_precision() << std::endl;
    output_file_ << "[  frequency] " << clock_frequency << std::endl;
    output_file_ << "[ clock time] " << get_clock_time() << std::endl;
    output_file_ << "[ MAC energy] " << get_mac_energy() << std::endl;
    output_file_ << "[ MAC static] " << get_mac_static_power() << std::endl;
    for(unsigned i = 0; i < (unsigned)component_t::SIZE; i++) {
        if(component_list[i] == nullptr) { continue; }
        output_file_ << std::endl;
        if(((temporal_component_t*)component_list[i])->type == reuse_t::TEMPORAL) {
            output_file_ << "[ comp. name] " << ((temporal_component_t*)component_list[i])->name << std::endl;
            print_temporal_component(output_file_, (temporal_component_t*)component_list[i]);
        }
        else {
            output_file_ << "[ comp. name] " << ((spatial_component_t*)component_list[i])->name << std::endl;
            print_spatial_component(output_file_, (spatial_component_t*)component_list[i]);
            i++;
        }
    }
    output_file_ << "==========================" << std::endl;
    output_file_ << "[      on-chip max power] "<< get_theoretical_peak_power() << " mW"<< std::endl;
    output_file_ << "[off-chip mem. max power] "<< get_dram_power() << " mW"<< std::endl;
    output_file_ << "[        total max power] " << get_theoretical_peak_power() + get_dram_power() << " mW" << std::endl;
    output_file_ << "==========================" << std::endl;
}
// Print out temporal component
void accelerator_t::print_temporal_component(temporal_component_t* component_) {
    std::string type_str[2]  = {"TEMPORAL", "SPATIAL"};
    std::string data_type[4] = {"INPUT", "WEIGHT", "OUTPUT", "NONE"};
    std::string dataflow_type[4] = {"none", "Input stationary", "Weight stationary", "Output stationary"};
    std::cout << "[ comp. type] " << type_str[(unsigned)component_->type] << "\n"
              << "[ comp. size] "; 
    for(unsigned i = 0; i < component_->size.size(); i++) {
        if(component_->size.at(i) / 1024 > 1) {
            if(component_->size.at(i)/ (1024*1024) > 1) {
                std::cout << component_->size.at(i) / 1024 / 1024 << " MB  ";
            }
            else {
                std::cout << component_->size.at(i) / 1024 << " KB  ";
            }
        }
        else {
            std::cout << component_->size.at(i) << " B  ";
        }
    }
    std::cout << std::endl;
    if(component_->num_components == 2) {
        std::cout << "[  sep. type] ";
        for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
            if(component_->separated_data[i]) {
                std::cout << data_type[i] << " ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "[     bypass] ";
    for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
        if(component_->bypass[i]) {
            std::cout << data_type[i] << " ";
        }
    }
    std::cout << std::endl;
    std::cout << "[   bitwidth] ";
    for(unsigned i = 0; i < component_->num_components; i++) {
        std::cout << component_->bitwidth[i] << "  ";
    }
    std::cout << std::endl;
    std::cout << "[   dataflow] " << dataflow_type[(unsigned)component_->dataflow] << std::endl;
    std::cout << "[unit energy] "; 
    for(unsigned i = 0; i < component_->num_components; i++) {
        std::cout << component_->unit_energy[i] << "  ";
    }
    std::cout << std::endl;
    std::cout << "[unit static] "; 
    for(unsigned i = 0; i < component_->num_components; i++) {
        std::cout << component_->unit_static[i] << "  ";
    }
    std::cout << std::endl;
    std::cout << "[ unit cycle] "; 
    for(unsigned i = 0; i < component_->num_components; i++) {
        std::cout << component_->unit_cycle[i] << "  ";
    }
    std::cout << std::endl;
    if(component_->constraint.size() != 0) {
        std::cout << "[  unit cnst] " << component_->constraint << std::endl;
    }
    return;
}
void accelerator_t::print_temporal_component(std::ofstream &output_file_, 
                                             temporal_component_t* component_) {
    std::string type_str[2]  = {"TEMPORAL", "SPATIAL"};
    std::string data_type[4] = {"INPUT", "WEIGHT", "OUTPUT", "NONE"};
    std::string dataflow_type[4] = {"none", "Input stationary", "Weight stationary", "Output stationary"};
    output_file_ << "[ comp. type] " << type_str[(unsigned)component_->type] << "\n"
              << "[ comp. size] "; 
    for(unsigned i = 0; i < component_->size.size(); i++) {
        // Convert unit size to KB
        if(component_->size.at(i) / 1024 > 1) {
            if(component_->size.at(i)/ (1024*1024) > 1) {
                output_file_ << component_->size.at(i) / 1024 / 1024 << " MB  ";
            }
            else {
                output_file_ << component_->size.at(i) / 1024 << " KB  ";
            }
        }
        else {
            output_file_ << component_->size.at(i) << " B  ";
        }
    }
    output_file_ << std::endl;
    if(component_->num_components == 2) {
        output_file_ << "[  sep. type] ";
        for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
            if(component_->separated_data[i]) {
                output_file_ << data_type[i] << " ";
            }
        }
        output_file_ << std::endl;
    }
    output_file_ << "[     bypass] ";
    for(unsigned i = 0; i < (unsigned)data_t::SIZE; i++) {
        if(component_->bypass[i]) {
            output_file_ << data_type[i] << " ";
        }
    }
    output_file_ << std::endl;
    output_file_ << "[   bitwidth] ";
    for(unsigned i = 0; i < component_->num_components; i++) {
        output_file_ << component_->bitwidth[i] << "  ";
    }
    output_file_ << "[   dataflow] " << dataflow_type[(unsigned)component_->dataflow] << std::endl;
    output_file_ << "[unit energy] "; 
    for(unsigned i = 0; i < component_->num_components; i++) {
        output_file_ << component_->unit_energy[i] << "  ";
    }
    output_file_ << std::endl;
    output_file_ << "[unit static] "; 
    for(unsigned i = 0; i < component_->num_components; i++) {
        output_file_ << component_->unit_static[i] << "  ";
    }
    output_file_ << std::endl;
    output_file_ << "[ unit cycle] "; 
    for(unsigned i = 0; i < component_->num_components; i++) {
        output_file_ << component_->unit_cycle[i] << "  ";
    }
    output_file_ << std::endl;
    if(component_->constraint.size() != 0) {
        output_file_ << "[  unit cnst] " << component_->constraint << std::endl;
    }
    return;
}
// Print out spatial component
void accelerator_t::print_spatial_component(spatial_component_t* component_) {
    std::string type_str[2] = {"TEMPORAL", "SPATIAL"};

    std::cout << "[ comp. type] " << type_str[(unsigned)component_->type] << "\n"
              << "[ comp. dimX] " << component_->dim_x << "\n"
              << "[ comp. dimY] " << component_->dim_y << std::endl; 
    if(!component_->constraint_x.empty()) {
        std::cout << "[comp. cnstX] " << uppercase(component_->constraint_x) << std::endl;
    }
    if(!component_->constraint_y.empty()) {
        std::cout << "[comp. cnstY] " << uppercase(component_->constraint_y) << std::endl;
    }
    std::cout << "[ comp. rdx ] ";
    std::cout << component_->radix_degree;
    std::cout << std::endl;
    return;
}
void accelerator_t::print_spatial_component(std::ofstream &output_file_, 
                                            spatial_component_t* component_) {
    std::string type_str[2]  = {"TEMPORAL", "SPATIAL"};

    output_file_ << "[ comp. type] " << type_str[(unsigned)component_->type] << "\n"
                 << "[ comp. dimX] " << component_->dim_x << "\n"
                 << "[ comp. dimY] " << component_->dim_y << std::endl; 
    if(!component_->constraint_x.empty()) {
        output_file_ << "[comp. cnstX] " << uppercase(component_->constraint_x) << std::endl;
    }
    if(!component_->constraint_y.empty()) {
        output_file_ << "[comp. cnstY] " << uppercase(component_->constraint_y) << std::endl;
    }
    output_file_ << "[ comp. rdx ] ";
    output_file_ << component_->radix_degree;
    output_file_ << std::endl;
    return;
}
// Set buffer's bitwidth
void accelerator_t::set_bitwidth(std::vector<float> &bw_, std::string value_) {
    std::vector<std::string> splited_value;
    // Get bitwidth configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        bw_.emplace_back(std::stof(splited_value.at(i)));
    }
    splited_value.clear();
    return;
}
void accelerator_t::set_bitwidth_per_dtype(temporal_component_t* comp_) {
    if(comp_->bitwidth.size() != 0) {
        if(comp_->num_components == 3) {
            comp_->bitwidth_i = comp_->bitwidth.at((unsigned)data_t::INPUT);
            comp_->bitwidth_w = comp_->bitwidth.at((unsigned)data_t::WEIGHT);
            comp_->bitwidth_o = comp_->bitwidth.at((unsigned)data_t::OUTPUT);
        }
        else {
            comp_->bitwidth_i = comp_->bitwidth.at(0);
            comp_->bitwidth_w = comp_->bitwidth.at(0);
            comp_->bitwidth_o = comp_->bitwidth.at(0);
            if(comp_->num_components == 2) {
                if(comp_->separated_data[(unsigned)data_t::INPUT]) {
                    comp_->bitwidth_i = comp_->bitwidth.at(1);
                }
                else if(comp_->separated_data[(unsigned)data_t::WEIGHT]) {
                    comp_->bitwidth_w = comp_->bitwidth.at(1);
                }
                else if(comp_->separated_data[(unsigned)data_t::OUTPUT]) {
                    comp_->bitwidth_o = comp_->bitwidth.at(1);
                }
                else {
                    std::cerr << "" << std::endl;
                }
            }
        }
    }
    // If bitwidth is undefined, matches bitwidth to ALU precision
    else {
        comp_->bitwidth_i = precision;
        comp_->bitwidth_w = precision;
        comp_->bitwidth_o = precision;
    }
    return;
}
// Set network's radix
void accelerator_t::set_radix_degree(spatial_component_t* comp_) {
    if(comp_->radix_degree != 0) {
        comp_->radix_degree_i = comp_->radix_degree;
        comp_->radix_degree_w = comp_->radix_degree;
        comp_->radix_degree_o = comp_->radix_degree;
    }
    else {
        // If radix_degree is undefined, set the value to 1.
        comp_->radix_degree_i = 1;
        comp_->radix_degree_w = 1;
        comp_->radix_degree_o = 1;
    }
    return;
}
unsigned accelerator_t::get_num_components(component_t comp_) {
    unsigned rtn = 0;
    if(component_list[(unsigned)comp_] != nullptr) {
        if (get_type(comp_) != reuse_t::TEMPORAL) {
            std::cerr << "Invalid function access; target component is spatial" << std::endl;
            exit(0);
        }
        rtn = ((temporal_component_t*)component_list[(unsigned)comp_])->num_components;
    }
    return rtn;
}
// Parsing temporal component
void accelerator_t::init_temporal_component(temporal_component_t* component_,
                                            section_config_t section_config_) {
    std::string unrefined_value;
    // Set component type
    component_->type = reuse_t::TEMPORAL;
    // Get component name
    component_->name = section_config_.name;
    // Set component size
    component_->size.clear();
    unrefined_value = "";
    section_config_.get_setting("buffer_size", &unrefined_value);
    section_config_.get_setting("size", &unrefined_value);
    if(!unrefined_value.empty()) { 
        component_->size = set_size(unrefined_value); 
    }
    unrefined_value = "";
    section_config_.get_setting("buffer_size_kb", &unrefined_value);
    section_config_.get_setting("size_kb", &unrefined_value);
    if(!unrefined_value.empty()) { 
        component_->size = set_size_kB(unrefined_value); 
    }
    unrefined_value = "";
    section_config_.get_setting("buffer_size_mb", &unrefined_value);
    section_config_.get_setting("size_mb", &unrefined_value);
    if(!unrefined_value.empty()) { 
        component_->size = set_size_MB(unrefined_value); 
    }
    // Set dataflow
    unrefined_value = "";
    section_config_.get_setting("dataflow", &unrefined_value);
    if(!unrefined_value.empty()) {
        component_->dataflow = (dataflow_t)get_enum_type(dataflow_str, unrefined_value);
    }
    // Update num_components
    component_->num_components = component_->size.size();
    // Set separated buffer
    unrefined_value = "";
    section_config_.get_setting("separated_data", &unrefined_value);
    if(!unrefined_value.empty()) { 
        set_separated_data(component_, unrefined_value); 
    }
    // Note, all below characteristics should have the number of the `num_components`
    // Set bitwidth
    unrefined_value = "";
    section_config_.get_setting("bitwidth", &unrefined_value);
    if(!unrefined_value.empty()) { set_bitwidth(component_->bitwidth, unrefined_value); }
    // If the bitwidth is undefined
    else {
        for(unsigned i = 0; i < component_->num_components; i++) {
            component_->bitwidth.emplace_back(precision);
        }
    }
    // Check validity
    if(component_->bitwidth.size() != component_->num_components) {
        std::cerr << "Error: invalid bitwidth"
                  << "# bitwidth != # components in " 
                  << component_->name
                  << " ("
                  << component_->bitwidth.size() << "!=" << component_->num_components
                  << ")"
                  << std::endl;
        exit(-1);
    }
    set_bitwidth_per_dtype(component_);
    
    unrefined_value = "";
    section_config_.get_setting("unit_access_energy", &unrefined_value);
    if(!unrefined_value.empty()) { 
        set_cost(component_->unit_energy, unrefined_value);
        // Check validity
        if(component_->unit_energy.size() != component_->num_components) { 
            std::cerr << "Error: invalid unit energy costs"
                    << "# unit energy != # components in " 
                    << component_->name
                    << " ("
                    << component_->unit_energy.size() << "!=" << component_->num_components
                    << ")"
                    << std::endl;
            exit(-1);
        }
    }
    else {
        // If unit_access_energy is undefined, set the value to 0.
        for(unsigned i = 0; i < component_->num_components; i++) {
            component_->unit_energy.emplace_back(0.0);
        }
    }
    set_energy_per_dtype(component_);
    // Set unit static power
    unrefined_value = "";
    section_config_.get_setting("unit_static_power", &unrefined_value);
    if(!unrefined_value.empty()) { 
        set_cost(component_->unit_static, unrefined_value);
        // Check validity
        if(component_->unit_static.size() != component_->num_components) { 
            std::cerr << "Error: invalid unit static costs"
                    << "# unit static != # components in " 
                    << component_->name
                    << " ("
                    << component_->unit_static.size() << "!=" << component_->num_components
                    << ")"
                    << std::endl;
            exit(-1);
        }
    }
    else {
        // If unit_static_power is undefined, set the value to 0.
        for(unsigned i = 0; i < component_->num_components; i++) {
            component_->unit_static.emplace_back(0.0);
        }
    }
    // set_static_per_dtype(component_);
    unrefined_value = "";
    section_config_.get_setting("unit_cycle", &unrefined_value);
    if(!unrefined_value.empty()) { 
        set_cost(component_->unit_cycle, unrefined_value);
        // Check validity
        if(component_->unit_cycle.size() != component_->num_components) { 
            std::cerr << "Error: invalid unit cycle costs"
                    << "# unit cycle != # components in " 
                    << component_->name
                    << " ("
                    << component_->unit_cycle.size() << "!=" << component_->num_components
                    << ")"
                    << std::endl;
            exit(-1);
        }
    }
    else {
        // If unit_cycle is undefined, set the value to 1.
        for(unsigned i = 0; i < component_->num_components; i++) {
            component_->unit_cycle.emplace_back(1.0);
        }
    }
    set_cycle_per_dtype(component_);
    // Set component bypass
    unrefined_value = "";
    section_config_.get_setting("bypass", &unrefined_value);
    if(!unrefined_value.empty()) { set_bypass(component_, unrefined_value); }
    unrefined_value = "";
    section_config_.get_setting("constraint", &unrefined_value);
    if(!unrefined_value.empty()) { set_constraint(component_, unrefined_value); }

    return;
}
// Parsing spatial component
void accelerator_t::init_spatial_component(spatial_component_t* component_,
                                           section_config_t section_config_) {
    // Get component type
    component_->type = reuse_t::SPATIAL;
    // Get component name
    component_->name = section_config_.name;
    // Get component interconnection type
    std::string unrefined_value;
    unrefined_value = "";
    section_config_.get_setting("radix_degree", &unrefined_value);
    if(!unrefined_value.empty()) { component_->radix_degree = stoi(unrefined_value); }
    // Set radix_degree for I, W, O
    set_radix_degree(component_);
    // Get component size
    section_config_.get_setting("size_x", &component_->dim_x);
    section_config_.get_setting("size_y", &component_->dim_y);
    section_config_.get_setting("constraint_x", &component_->constraint_x);
    section_config_.get_setting("constraint_y", &component_->constraint_y);
    return;

}
// Calculate accelerator's theoretical maximum power
void accelerator_t::calc_accelerator_max_power() {
    float max_power            = 0.0;
    float max_energy_per_cycle = 0.0;
    float comp_level_static   = 0.0;
    // Sum all component's energy consumption
    max_energy_per_cycle = sum_component_energy();
    comp_level_static   = sum_component_static_power();
    // Multiply with accelerator's frequency (pJ x MHz = uW)
    max_power = max_energy_per_cycle * clock_frequency;
    // uW -> mW
    max_power = max_power / 1000;
    // Consider static power 
    max_power += comp_level_static;
    // Default unit: MW
    theoretical_peak_power = max_power;
    return;
}
void accelerator_t::calc_dram_power() {
    float dram_energy = get_energy(component_t::DRAM)[0];
    float dram_static_power = get_static(component_t::DRAM)[0];
    dram_power = (dram_energy * clock_frequency) / 1000 + dram_static_power;
    return;
}
// Sum all component's static power
float accelerator_t::sum_component_static_power() {
    float static_power = mac_static_power;
    float* comp_static_power = nullptr;
    unsigned num_comps = 1;
    component_t target_comp;
    // Traverse the component list
    for(unsigned idx = 0; idx < (unsigned)component_t::SIZE - 1; idx++) {
        target_comp = static_cast<component_t>(idx);
        // If component type is temporal
        if(get_type(target_comp) == reuse_t::TEMPORAL) {
            // Accumulate static power
            comp_static_power = get_static(target_comp);
            if(comp_static_power == nullptr) { continue; }
            num_comps = get_num_components(target_comp);
            static_power += std::accumulate(comp_static_power,
                    comp_static_power + num_comps,
                    0.0f);
        }
        else {
            // multiply the power with #comp
            if(target_comp == component_t::MAC_X || target_comp == component_t::PE_X || target_comp == component_t::CHIP_X) {
                static_power *= (get_array_size(target_comp, dimension_t::DIM_X));
            }
            else {
                static_power *= (get_array_size(target_comp, dimension_t::DIM_Y));
            }
        }
    }
    return static_power;
}
// Sum all component's unit energy
float accelerator_t::sum_component_energy() {
    float overall_energy = mac_operation_energy;
    float* comp_energy;
    unsigned num_comps = 1;
    component_t target_comp;
    // Traverse the component list
    for(unsigned idx = 0; idx < (unsigned)component_t::SIZE - 1; idx++) {
        target_comp = static_cast<component_t>(idx);
        // If component type is temporal
        if(get_type(target_comp) == reuse_t::TEMPORAL) {
            // Accumulate overall energy
            comp_energy = get_energy(target_comp);
            if(comp_energy == nullptr) { continue; }
            num_comps = get_num_components(target_comp);
            overall_energy += std::accumulate(comp_energy,
                    comp_energy + num_comps,
                    0.0f);
        }
        else {
            // multiply the power with #comp
            if(target_comp == component_t::MAC_X || target_comp == component_t::PE_X || target_comp == component_t::CHIP_X) {
                overall_energy *= (get_array_size(target_comp, dimension_t::DIM_X));
            }
            else {
                overall_energy *= (get_array_size(target_comp, dimension_t::DIM_Y));
            }
        }
    }
    return overall_energy;
}
// Set component size
std::vector<float> accelerator_t::set_size(std::string value_) const {
    std::vector<std::string> splited_value;
    std::vector<float> rtn;   
    // Get buffer size configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        rtn.push_back(std::stof(splited_value.at(i)));
    }
    splited_value.clear();
    return rtn;
}
std::vector<float> accelerator_t::set_size_kB(std::string value_) const {
    std::vector<std::string> splited_value;
    std::vector<float> rtn;   
    // Get buffer size configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        rtn.push_back(std::stof(splited_value.at(i)) * 1024);
    }
    splited_value.clear();
    return rtn;
}
std::vector<float> accelerator_t::set_size_MB(std::string value_) const {
    std::vector<std::string> splited_value;
    std::vector<float> rtn;   
    // Get buffer size configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        rtn.push_back(std::stof(splited_value.at(i)) * 1024 * 1024);
    }
    splited_value.clear();
    return rtn;
}
// Set component cost
void accelerator_t::set_cost(std::vector<float> &cost_, std::string value_) {
    std::vector<std::string> splited_value;
    // Get buffer size configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        cost_.emplace_back(std::stof(splited_value.at(i)));
    }
    splited_value.clear();

    return;
}

void accelerator_t::set_energy_per_dtype(temporal_component_t* comp_) {
    float& energy_i = comp_->unit_energy_per_dtype[(unsigned)data_t::INPUT]; 
    float& energy_w = comp_->unit_energy_per_dtype[(unsigned)data_t::WEIGHT]; 
    float& energy_o = comp_->unit_energy_per_dtype[(unsigned)data_t::OUTPUT]; 
    if(comp_->num_components == 3) {
        energy_i = comp_->unit_energy.at((unsigned)data_t::INPUT);
        energy_w = comp_->unit_energy.at((unsigned)data_t::WEIGHT);
        energy_o = comp_->unit_energy.at((unsigned)data_t::OUTPUT);
    }
    else {
        energy_i = comp_->unit_energy.at(0);
        energy_w = comp_->unit_energy.at(0);
        energy_o = comp_->unit_energy.at(0);
        if(comp_->num_components == 2) {
            if(comp_->separated_data[(unsigned)data_t::INPUT]) {
                energy_i = comp_->unit_energy.at(1);
            }
            else if(comp_->separated_data[(unsigned)data_t::WEIGHT]) {
                energy_w = comp_->unit_energy.at(1);
            }
            else if(comp_->separated_data[(unsigned)data_t::OUTPUT]) {
                energy_o = comp_->unit_energy.at(1);
            }
            else {
                std::cerr << "Error: Separated didn't set" << std::endl;
            }
        }
    }
    return;
}

void accelerator_t::set_cycle_per_dtype(temporal_component_t* comp_) {
    float& cycle_i = comp_->unit_cycle_per_dtype[(unsigned)data_t::INPUT]; 
    float& cycle_w = comp_->unit_cycle_per_dtype[(unsigned)data_t::WEIGHT]; 
    float& cycle_o = comp_->unit_cycle_per_dtype[(unsigned)data_t::OUTPUT]; 
    if(comp_->num_components == 3) {
        cycle_i = comp_->unit_cycle.at((unsigned)data_t::INPUT);
        cycle_w = comp_->unit_cycle.at((unsigned)data_t::WEIGHT);
        cycle_o = comp_->unit_cycle.at((unsigned)data_t::OUTPUT);
    }
    else {
        cycle_i = comp_->unit_cycle.at(0);
        cycle_w = comp_->unit_cycle.at(0);
        cycle_o = comp_->unit_cycle.at(0);
        if(comp_->num_components == 2) {
            if(comp_->separated_data[(unsigned)data_t::INPUT]) {
                cycle_i = comp_->unit_cycle.at(1);
            }
            else if(comp_->separated_data[(unsigned)data_t::WEIGHT]) {
                cycle_w = comp_->unit_cycle.at(1);
            }
            else if(comp_->separated_data[(unsigned)data_t::OUTPUT]) {
                cycle_o = comp_->unit_cycle.at(1);
            }
            else {
                std::cerr << "Error: Separated didn't set" << std::endl;
            }
        }
    }
    return;
}

// Set component bypass
void accelerator_t::set_bypass(temporal_component_t* comp_, std::string value_) {
    std::vector<std::string> splited_value;
    
    // Get bypass configuration
    splited_value = split(value_, ',');
    for(unsigned i = 0; i < splited_value.size(); i++) {
        if(splited_value.at(i) == "input") { 
            comp_->bypass[(unsigned)data_t::INPUT] = true;
        }
        else if(splited_value.at(i) == "weight") { 
            comp_->bypass[(unsigned)data_t::WEIGHT] = true;
        }
        else if(splited_value.at(i) == "output") { 
            comp_->bypass[(unsigned)data_t::OUTPUT] = true;
        }
        else if(splited_value.at(i) == "flexible") {
            is_flexible = true;
            data_t target_type = data_t::SIZE;

            // Exception handling: the bypass type `flexible` is valid only when 
            // the dataflow of upper level component is set
            // TODO: generalize the code
            if(get_dataflow(component_t::LB) == dataflow_t::NONE
              || get_dataflow(component_t::LB) == dataflow_t::SIZE
            ) {
                std::cerr << "Error: invalid bypass configuration; " 
                          << "LB dataflow should be set first" << std::endl;
            }
            // Set bypass depending on the dataflow of its upper level comp.
            if(get_dataflow(component_t::LB)==dataflow_t::IS) {
                target_type = data_t::INPUT;
            }
            else if(get_dataflow(component_t::LB)==dataflow_t::WS) {
                target_type = data_t::WEIGHT;
            }
            else if(get_dataflow(component_t::LB)==dataflow_t::OS) {
                target_type = data_t::OUTPUT;
            }
            // Check target_type doesn't exceed the size of data_t -1
            if((unsigned)target_type > (unsigned)data_t::SIZE - 1) {
                std::cerr << "Error: invalid bypass configuration " 
                          << splited_value.at(i)<< std::endl;
                exit(0);
            }
            comp_->bypass[(unsigned)target_type] = true;

            // Set unit access cost of the bypassed data type to zero
            backup_energy = comp_->unit_energy[(unsigned)target_type];
            backup_power  = comp_->unit_static[(unsigned)target_type];
            comp_->unit_energy[(unsigned)target_type] = 0;
            comp_->unit_static[(unsigned)target_type] = 0;
        }
        else {
            std::cerr << "Error: invalid bypass configuration " 
                      << splited_value.at(i)<< std::endl;
            exit(0);
        }
    }
    // Check bypass validity
    if((comp_->dataflow == dataflow_t::IS && comp_->bypass[unsigned(data_t::INPUT)])
    || (comp_->dataflow == dataflow_t::WS && comp_->bypass[unsigned(data_t::WEIGHT)])
    || (comp_->dataflow == dataflow_t::OS && comp_->bypass[unsigned(data_t::OUTPUT)])) {
        std::cerr << "[Error] Invalid bypass setting in `" << comp_->name 
                  << "`. Data type, affected by dataflow, should be stored in the buffer level."
                  << std::endl;
        exit(0);
    }
    return;
}

void accelerator_t::set_separated_data(temporal_component_t* comp_, 
        std::string value_) {
    std::vector<std::string> splited_value;
    splited_value = split(value_, ',');
    // Check the component validity
    if(comp_->num_components == 2 && splited_value.size() > 0) {
        // Get buffer configuration
        for(unsigned i = 0; i < splited_value.size(); i++) {
            if(splited_value.at(i) == "input") { 
                comp_->separated_data[(unsigned)data_t::INPUT] = true;
            }
            else if(splited_value.at(i) == "weight") {
                comp_->separated_data[(unsigned)data_t::WEIGHT] = true;
            }
            else if(splited_value.at(i) == "output") { 
                comp_->separated_data[(unsigned)data_t::OUTPUT] = true;
            }
        }
    }
    else {
        std::cerr << "[Error] Improper use of configuration; " 
            << comp_->name
            << " is constructed with the two separated buffers."
            << " Make sure which data type is allocaed in the secondary buffer."
            << std::endl;
        exit(-1);
    }
    if(splited_value.size() > 1) {
        std::cerr << "[Error] Improper use of configuration; Separated data type in " 
            << comp_->name
            << " should not be exceed 1."
            << std::endl;
        exit(-1);
    }
}

// Set temporal components constraint
void accelerator_t::set_constraint(temporal_component_t* comp_, std::string cnst_) {
    std::vector<std::string> splited_cnst;
    // Check the validty of input constraints
    splited_cnst = split(cnst_, ',');
    if(splited_cnst.size() != (unsigned) parameter_t::SIZE) {
        std::cerr << "[Error] Invalid memory mapping constraint; Specify mapping values for all eight parameters. ("
                  << splited_cnst.size() << " / " << "8)" << std::endl;
        exit(0);
    }
    for(unsigned i = 0; i < splited_cnst.size(); i++) {
        // Check whether value i is number
        if(!is_number(splited_cnst.at(i))) {
            std::cerr << "[Error] Invalid constraint value; all memory constraint values is number. "
                      << splited_cnst.at(i) << std::endl;
            exit(0);
        }
    }
    // Update component's constraint
    comp_->constraint = cnst_;
    return;
}
