#include "stats.h"
#include <iomanip>

static handler_t handler;

/* Accelerator stats */
accelerator_t::accelerator_t(accelerator_configs_t &accelerator_configs_)
    : accelerator_configs(accelerator_configs_) {
    init();
}

accelerator_t::~accelerator_t() {
}

void accelerator_t::init() {
    std::string parameters = "KBPQCRS";
    name = accelerator_configs.name;
    // PRECISION
    input_precision = accelerator_configs.precision.at(0);     // Input
    weight_precision = accelerator_configs.precision.at(1);    // Weight
    output_precision = accelerator_configs.precision.at(2);    // Output
    // MAC
    mac_per_pe = accelerator_configs.mac_per_pe;
    mac_width = accelerator_configs.mac_width;
    if(accelerator_configs.L0_dataflow == "IS")
        L0_dataflow = dataflow_t::IS;
    else if(accelerator_configs.L0_dataflow == "WS")
        L0_dataflow = dataflow_t::WS;
    else if(accelerator_configs.L0_dataflow == "OS")
        L0_dataflow = dataflow_t::OS;
    else if(accelerator_configs.L0_dataflow == "NO")
        L0_dataflow = dataflow_t::NO;
    else  
        handler.print_err(err_type_t::INVAILD, "L0 dataflow");
    // L1
    L1_mode = accelerator_configs.L1_mode;
    if(!L1_mode) {
        L1_total_size = accelerator_configs.L1_shared_size;
        L1_input_size = 0;
        L1_weight_size = 0;
        L1_output_size = 0;
    }
    else {
        L1_input_size = accelerator_configs.L1_separate_size.at(0);
        L1_weight_size = accelerator_configs.L1_separate_size.at(1);
        L1_output_size = accelerator_configs.L1_separate_size.at(2);
        L1_total_size = L1_input_size + L1_weight_size + L1_output_size;
    }
    if(accelerator_configs.L1_dataflow == "IS")
        L1_dataflow = dataflow_t::IS;
    else if(accelerator_configs.L1_dataflow == "WS")
        L1_dataflow = dataflow_t::WS;
    else if(accelerator_configs.L1_dataflow == "OS")
        L1_dataflow = dataflow_t::OS;
    else if(accelerator_configs.L1_dataflow == "NO")
        L1_dataflow = dataflow_t::NO;
    else  
        handler.print_err(err_type_t::INVAILD, "L0 dataflow");
    // X, Y
    array_mode = accelerator_configs.array_mode;
    array_size_x = accelerator_configs.array_size_x;
    array_size_y = accelerator_configs.array_size_y;
    if(array_mode) {
        array_map_x = parameters.find(accelerator_configs.array_map_x);
        array_map_y = parameters.find(accelerator_configs.array_map_y);
    }
    // L2
    L2_shared_size = accelerator_configs.L2_shared_size;
    if(accelerator_configs.L2_dataflow == "IS")
        L2_dataflow = dataflow_t::IS;
    else if(accelerator_configs.L2_dataflow == "WS")
        L2_dataflow = dataflow_t::WS;
    else if(accelerator_configs.L2_dataflow == "OS")
        L2_dataflow = dataflow_t::OS;
    else if(accelerator_configs.L2_dataflow == "NO")
        L2_dataflow = dataflow_t::NO;
    else  
        handler.print_err(err_type_t::INVAILD, "L0 dataflow");
}

void accelerator_t::print_stats() {
    std::string L1_mode_str = (L1_mode) ? "Separate": "Shared";
    std::string array_mode_str = (array_mode) ? "Fixed": "Flexible";
    std::string parameters = "KBPQCRS";
    handler.print_line(35, "-");
    std::cout << "# Alpha(W/O), Beta(I/O), Gamma(I/W)\n"
              << "  Alpha: K\n"
              << "  Beta : B, P, Q\n"
              << "  Gamma: C, R, S" << std::endl;
    handler.print_line(35, "-");
    std::cout << "# ACCELERATOR SPEC" << std::endl;
    std::cout << std::setw(16) << "NAME: " << name << "\n"
              // MAC
              << std::setw(16) << "MAC PER PE: " << mac_per_pe << "\n"
              << std::setw(16) << "MAC WIDTH: " << mac_width << "\n"
              << std::setw(16) << "L0 DATAFLOW: " << static_cast<unsigned>(L0_dataflow) << "\n"
              // L1
              << std::setw(16) << "L1 MODE   : " << L1_mode_str << "\n"
              << std::setw(16) << "L1 SIZE   : " << L1_total_size << " bytes\n"
              << std::setw(16) << "L1 SIZE(I): " << L1_input_size << " bytes\n"
              << std::setw(16) << "L1 SIZE(W): " << L1_weight_size << " bytes\n"
              << std::setw(16) << "L1 SIZE(O): " << L1_output_size << " bytes\n"
              << std::setw(16) << "L1 DATAFLOW: " << static_cast<unsigned>(L1_dataflow) << std::endl;
              // X, Y
    if(!array_mode) {
    std::cout << std::setw(16) << "ARRAY MODE  : " << array_mode_str << "\n"
              << std::setw(16) << "X: " << array_size_x << "\n"
              << std::setw(16) << "Y: " << array_size_y << std::endl;
    }
    else {
    std::cout << std::setw(16) << "ARRAY MODE  : " << array_mode_str << "\n"
              << std::setw(16) << "X: " << array_size_x << "\n"
              << std::setw(16) << "Y: " << array_size_y << "\n"
              << std::setw(16) << "X MAP: " << parameters.at(static_cast<unsigned>(array_map_x)) << "\n"
              << std::setw(16) << "Y MAP: " << parameters.at(static_cast<unsigned>(array_map_y)) << std::endl;
    }
              // L2
    std::cout << std::setw(16) << "L2 SHARED SIZE: " << L2_shared_size << " KB\n"
              << std::setw(16) << "L2 DATAFLOW: " << static_cast<unsigned>(L2_dataflow) << "\n"
              // PRECISION
              << std::setw(16) << "PRECISION(I): " << input_precision << " bits\n"
              << std::setw(16) << "PRECISION(W): " << weight_precision << " bits\n"
              << std::setw(16) << "PRECISION(O): " << output_precision << " bits" << std::endl;
}


/* Tile sizes */
tile_size_t::tile_size_t() {
}

tile_size_t::~tile_size_t() {
}

/* NoC information */
noc_info_t::noc_info_t() {
}

noc_info_t::~noc_info_t() {
}

/* Access counts */
access_cnts_t::access_cnts_t() {
}

access_cnts_t::~access_cnts_t() {
}

/* Energy constants */
energy_stats_t::energy_stats_t() {
}

energy_stats_t::~energy_stats_t() {
}

/* Cycle constants */
cycle_stats_t::cycle_stats_t() {
}

cycle_stats_t::~cycle_stats_t() {
}
