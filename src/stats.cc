#include "stats.h"
#include <iomanip>

static handler_t handler;

/* Accelerator stats */
accelerator_t::accelerator_t() {
}

accelerator_t::~accelerator_t() {
}

void accelerator_t::init() {
}

void accelerator_t::print_stats() {
    handler.print_line(35, "-");
    std::cout << "# BYPASS MASK: 0 ~ 7\n"
              << "  0(XXX), 1(XXO), 2(XWX), 3(XWO),\n"
              << "  4(IXX), 5(IXO), 6(IWX), 7(IWO)" << std::endl;
    std::cout << "# UNROLL PARAMETER: Alpha(W/O), Beta(I/O), Gamma(I/W)\n"
              << "  Alpha: K(0)\n"
              << "  Beta : B(1), P(2), Q(3)\n"
              << "  Gamma: C(4), R(5), S(6)" << std::endl;
    handler.print_line(35, "-");
    std::cout << "# ACCELERATOR SPEC" << std::endl;
    std::cout << std::setw(15) << "NAME: " << name << "\n"
              // L0
              << std::setw(15) << "MAC PER PE: " << mac_per_pe << "\n"
              << std::setw(15) << "MAC WIDTH: " << mac_width << "\n"
              // L1
              << std::setw(15) << "L1 SIZE(I): " << input_L1_sizes << " bytes\n"
              << std::setw(15) << "L1 SIZE(W): " << weight_L1_sizes << " bytes\n"
              << std::setw(15) << "L1 SIZE(O): " << output_L1_sizes << " bytes\n"
              << std::setw(15) << "L1 BYPASS: " << static_cast<unsigned>(L1_bypass) << "\n"
              << std::setw(15) << "L1 STATIONAYR: " << L1_stationary << "\n"
              // X, Y
              << std::setw(15) << "X: " << array_size_x << "\n"
              << std::setw(15) << "Y: " << array_size_y << "\n"
              << std::setw(15) << "X UNROLL: " << static_cast<unsigned>(array_unroll_x) << "\n"
              << std::setw(15) << "Y UNROLL: " << static_cast<unsigned>(array_unroll_y) << "\n"
              // L2
              << std::setw(15) << "L2 SIZE: " << L2_size << " KB\n"
              << std::setw(15) << "L2 BYPASS: " << static_cast<unsigned>(L2_bypass) << "\n"
              << std::setw(15) << "L2 STATIONAYR: " << L2_stationary << "\n"
              // PRECISION
              << std::setw(15) << "PRECISION(I): " << input_precision << " bits\n"
              << std::setw(15) << "PRECISION(W): " << weight_precision << " bits\n"
              << std::setw(15) << "PRECISION(O): " << output_precision << " bits" << std::endl;
}

/* Tiles stats */
tiles_t::tiles_t(mapping_table_t *mapping_table_, accelerator_t *accelerator_) 
    : mapping_table(mapping_table_), accelerator(accelerator_) {
    init();
}

tiles_t::~tiles_t() {
}

void tiles_t::init() {
    tile_t mac_tile("  MAC TILE");      // 0
    mac_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::L0);
    mac_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::L0);
    mac_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::L0);
    tiles.push_back(mac_tile);
    tile_t pe_tile("   PE TILE");       // 1
    pe_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::L1);
    pe_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::L1);
    pe_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::L1);
    tiles.push_back(pe_tile);
    tile_t array_tile("ARRAY TILE");    // 2
    array_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::Y);
    array_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::Y);
    array_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::Y);
    tiles.push_back(array_tile);
    tile_t glb_tile("  GLB TILE");      // 3
    glb_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::L2);
    glb_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::L2);
    glb_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::L2);
    tiles.push_back(glb_tile);
    tile_t chip_tile(" CHIP TILE");     // 4
    chip_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::CHIP);
    chip_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::CHIP);
    chip_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::CHIP);
    tiles.push_back(chip_tile);
}

void tiles_t::print_stats() {
    std::cout << "\n# TILE SIZES" << std::endl;
    std::cout << std::setw(12) << "TILES |" 
              << std::setw(10) << "INPUT"
              << std::setw(10) << "WEIGHT"
              << std::setw(10) << "OUTPUT" << std::endl; 
    handler.print_line(43, "-");
    for(size_t t = 0; t < tiles.size(); t++) {
        std::cout << tiles.at(t).name << " |" 
                  << std::setw(10) << tiles.at(t).num_input
                  << std::setw(10) << tiles.at(t).num_weight 
                  << std::setw(10) << tiles.at(t).num_output << std::endl;
    }
}

size_t tiles_t::calculate_tile_size(data_type_t type_, component_t U) {
    bool is_L1_bypass = false;
    bool is_L2_bypass = false;
    size_t rtn_size = 1;
    switch(type_) {
        case data_type_t::INPUT:
            // Bypass check
            is_L1_bypass = (accelerator->L1_bypass == bypass_t::IXX 
                         || accelerator->L1_bypass == bypass_t::IXO
                         || accelerator->L1_bypass == bypass_t::IWX
                         || accelerator->L1_bypass == bypass_t::IWO) ? true : false;
            is_L2_bypass = (accelerator->L2_bypass == bypass_t::IXX 
                         || accelerator->L2_bypass == bypass_t::IXO
                         || accelerator->L2_bypass == bypass_t::IWX
                         || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
            rtn_size *= mapping_table->product(parameter_t::B, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= (mapping_table->product(parameter_t::P, U, is_L1_bypass, is_L2_bypass) - 1) * mapping_table->stride 
                      + mapping_table->product(parameter_t::R, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= (mapping_table->product(parameter_t::Q, U, is_L1_bypass, is_L2_bypass) - 1) * mapping_table->stride
                      + mapping_table->product(parameter_t::S, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= mapping_table->product(parameter_t::C, U, is_L1_bypass, is_L2_bypass);
            break;
        case data_type_t::WEIGHT:
            // Bypass check
            is_L1_bypass = (accelerator->L1_bypass == bypass_t::XWX 
                         || accelerator->L1_bypass == bypass_t::XWO
                         || accelerator->L1_bypass == bypass_t::IWX
                         || accelerator->L1_bypass == bypass_t::IWO) ? true : false;
            is_L2_bypass = (accelerator->L2_bypass == bypass_t::XWX 
                         || accelerator->L2_bypass == bypass_t::XWO
                         || accelerator->L2_bypass == bypass_t::IWX
                         || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
            rtn_size *= mapping_table->product(parameter_t::K, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= mapping_table->product(parameter_t::R, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= mapping_table->product(parameter_t::S, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= mapping_table->product(parameter_t::C, U, is_L1_bypass, is_L2_bypass);
            break;
        case data_type_t::OUTPUT:
            // Bypass check
            is_L1_bypass = (accelerator->L1_bypass == bypass_t::XXO 
                         || accelerator->L1_bypass == bypass_t::XWO
                         || accelerator->L1_bypass == bypass_t::IXO
                         || accelerator->L1_bypass == bypass_t::IWO) ? true : false;
            is_L2_bypass = (accelerator->L2_bypass == bypass_t::XXO
                         || accelerator->L2_bypass == bypass_t::XWO
                         || accelerator->L2_bypass == bypass_t::IXO
                         || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
            rtn_size *= mapping_table->product(parameter_t::K, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= mapping_table->product(parameter_t::B, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= mapping_table->product(parameter_t::P, U, is_L1_bypass, is_L2_bypass);
            rtn_size *= mapping_table->product(parameter_t::Q, U, is_L1_bypass, is_L2_bypass);
            break;
        default: handler.print_err(err_type_t::INVAILD, "Data type");
    }
    return rtn_size;
}

/* Accesses stats */
accesses_t::accesses_t(mapping_table_t *mapping_table_, accelerator_t *accelerator_, tiles_t *tiles_) 
: mapping_table(mapping_table_), accelerator(accelerator_), tiles(tiles_){
    init();
}

accesses_t::~accesses_t() {
}

void accesses_t::init() {
//    access_t glb_access("  GLB ACCESSES");  // 1
//    glb_access.cnts_input = calculate_access_counts(data_type_t::INPUT, component_t::Y);
//    glb_access.cnts_weight = calculate_access_counts(data_type_t::INPUT, component_t::Y);
//    glb_access.cnts_output = calculate_access_counts(data_type_t::INPUT, component_t::Y);
//    glb_access.counts_to_mb(accelerator);
    //accesses.push_back(glb_access);
    access_t dram_access(" DRAM ACCESSES"); // 2
    dram_access.cnts_input = calculate_access_counts(data_type_t::INPUT, component_t::CHIP);
    dram_access.cnts_weight = calculate_access_counts(data_type_t::WEIGHT, component_t::CHIP);
    dram_access.cnts_output = calculate_access_counts(data_type_t::OUTPUT, component_t::CHIP);
    dram_access.counts_to_mb(accelerator);
    accesses.push_back(dram_access);
}

void accesses_t::print_stats() {
//    std::cout << "\n# STORAGE ACCESSES" << std::endl;
//    std::cout << std::setw(15) << "ACCESSES |" 
//              << std::setw(20) << "INPUT"
//              << std::setw(20) << "WEIGHT"
//              << std::setw(20) << "OUTPUT" << std::endl; 
//    handler.print_line(43);
//    for(size_t a = 0; a < accesses.size(); a++) {
//        std::cout << std::setw(15) << accesses.at(a).name << " |" 
//                  << std::setw(20) << accesses.at(a).cnts_input
//                  << std::setw(20) << accesses.at(a).cnts_weight 
//                  << std::setw(20) << accesses.at(a).cnts_output << std::endl;
//    }
    std::cout << "\n# STORAGE ACCESSES(MB)" << std::endl;
    std::cout << std::setw(17) << "ACCESSES |" 
              << std::setw(10) << "INPUT"
              << std::setw(10) << "WEIGHT"
              << std::setw(10) << "OUTPUT"
              << std::setw(10) << "TOTAL" << std::endl; 
    handler.print_line(58, "-");
    for(size_t a = 0; a < accesses.size(); a++) {
        std::cout << std::setw(15) << accesses.at(a).name << " |" 
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_input
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_weight
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_output
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_total << std::endl;
    }
}

size_t accesses_t::calculate_access_counts(data_type_t type_, component_t U) {
    bool is_L1_bypass = false;
    bool is_L2_bypass = false;
    size_t rtn_size = 1;
    
    // No bypass
    if(accelerator->L2_bypass == bypass_t::XXX) {

    }
    // One data bypass
    else if(accelerator->L2_bypass == bypass_t::XXO 
       || accelerator->L2_bypass == bypass_t::XWX 
       || accelerator->L2_bypass == bypass_t::IXX) {

    }
    // Two data bypass 
    else if(accelerator->L2_bypass == bypass_t::XWO 
       || accelerator->L2_bypass == bypass_t::IXO 
       || accelerator->L2_bypass == bypass_t::IWX) {

    }
    // All bypass: Same as without GLB but latency is reduced
    else {

    }

    

    switch(type_) {
        case data_type_t::INPUT:
            // DRAM accesses
            if(U == component_t::CHIP) {
                // L2 Bypass check
                is_L2_bypass = (accelerator->L2_bypass == bypass_t::IXX 
                             || accelerator->L2_bypass == bypass_t::IXO
                             || accelerator->L2_bypass == bypass_t::IWX
                             || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
                // CHIP tile size x dividers
                rtn_size *= tiles->tiles.at(4).num_input;
                //rtn_size *= mapping_table->quotient(parameter_t::K, U, is_L1_bypass, false);
                rtn_size *= mapping_table->divider(parameter_t::B, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::P, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::Q, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::C, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::R, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::S, U, is_L1_bypass, is_L2_bypass);
                // Bypass effect
                if(is_L2_bypass) {
                    // CHIP tile size x quotient (because K is not an input parameter)
                    rtn_size *= mapping_table->quotient(parameter_t::K, U, is_L1_bypass, false);
                }
            }
            break;
        case data_type_t::WEIGHT:
            // DRAM accesses
            if(U == component_t::CHIP) {
                // L2 Bypass check
                is_L2_bypass = (accelerator->L2_bypass == bypass_t::XWX 
                             || accelerator->L2_bypass == bypass_t::XWO
                             || accelerator->L2_bypass == bypass_t::IWX
                             || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
                // Chip tile size x dividers
                rtn_size *= tiles->tiles.at(4).num_weight;
                rtn_size *= mapping_table->divider(parameter_t::K, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::R, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::S, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::C, U, is_L1_bypass, is_L2_bypass);
                // Bypass effect
                if(is_L2_bypass) {
                    // CHIP tile size x quotients (because B,P,Q are not weight parameters)
                    rtn_size *= mapping_table->quotient(parameter_t::B, U, is_L1_bypass, false);
                    rtn_size *= mapping_table->quotient(parameter_t::P, U, is_L1_bypass, false);
                    rtn_size *= mapping_table->quotient(parameter_t::Q, U, is_L1_bypass, false);
                }
            }
            // GLB accesses
//            else if(U == component_t::Y) {
//                // L1 Bypass check
//                is_L2_bypass = (accelerator->L2_bypass == bypass_t::XWX 
//                             || accelerator->L2_bypass == bypass_t::XWO
//                             || accelerator->L2_bypass == bypass_t::IWX
//                             || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
//                // L2 Bypass check
//                is_L2_bypass = (accelerator->L2_bypass == bypass_t::XWX 
//                             || accelerator->L2_bypass == bypass_t::XWO
//                             || accelerator->L2_bypass == bypass_t::IWX
//                             || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
//                // Chip tile size x dividers
//                rtn_size *= tiles->tiles.at(2).num_weight;
//                rtn_size *= mapping_table->divider(parameter_t::K, U, is_L1_bypass, is_L2_bypass);
//                rtn_size *= mapping_table->divider(parameter_t::R, U, is_L1_bypass, is_L2_bypass);
//                rtn_size *= mapping_table->divider(parameter_t::S, U, is_L1_bypass, is_L2_bypass);
//                rtn_size *= mapping_table->divider(parameter_t::C, U, is_L1_bypass, is_L2_bypass);
//                // Multiply bypass effects 
//                if(is_L2_bypass) {
//                    rtn_size *= mapping_table->divider(parameter_t::B, U, is_L1_bypass, false);
//                    rtn_size *= mapping_table->divider(parameter_t::P, U, is_L1_bypass, false);
//                    rtn_size *= mapping_table->divider(parameter_t::Q, U, is_L1_bypass, false);
//                }
//            }
            break;
        case data_type_t::OUTPUT:
            // DRAM accesses
            if(U == component_t::CHIP) {
                // L2 Bypass check
                is_L2_bypass = (accelerator->L2_bypass == bypass_t::XXO
                             || accelerator->L2_bypass == bypass_t::XWO
                             || accelerator->L2_bypass == bypass_t::IXO
                             || accelerator->L2_bypass == bypass_t::IWO) ? true : false;
                // CHIP tile size x dividers
                rtn_size *= tiles->tiles.at(4).num_output;
                rtn_size *= mapping_table->divider(parameter_t::K, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::B, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::P, U, is_L1_bypass, is_L2_bypass);
                rtn_size *= mapping_table->divider(parameter_t::Q, U, is_L1_bypass, is_L2_bypass);
                // Bypass effect 
                if(is_L2_bypass) {
                    // CHIP tile size x quotients (because C,R,S are not output parameters)
                    rtn_size *= mapping_table->quotient(parameter_t::C, U, is_L1_bypass, false);
                    rtn_size *= mapping_table->quotient(parameter_t::R, U, is_L1_bypass, false);
                    rtn_size *= mapping_table->quotient(parameter_t::S, U, is_L1_bypass, false);
                    rtn_size *= 2; // Read/Write
                }
            }
            break;
        default: handler.print_err(err_type_t::INVAILD, "Data type");
    }
    return rtn_size;
}
