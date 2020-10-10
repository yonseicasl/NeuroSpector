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
    handler.print_line(35);
    std::cout << "# BYPASS MASK: 0 ~ 7\n"
              << "  0(XXX), 1(XXO), 2(XWX), 3(XWO),\n"
              << "  4(IXX), 5(IXO), 6(IWX), 7(IWO)" << std::endl;
    handler.print_line(35);
    std::cout << "# ACCELERATOR SPEC" << std::endl;
    std::cout << std::setw(15) << "NAME: " << name << "\n"
              << std::setw(15) << "X: " << array_size_x << "\n"
              << std::setw(15) << "Y: " << array_size_y << "\n"
              << std::setw(15) << "PRECISION(I): " << input_precision << "\n"
              << std::setw(15) << "PRECISION(W): " << weight_precision << "\n"
              << std::setw(15) << "PRECISION(O): " << output_precision << "\n"
              << std::setw(15) << "L1 BYPASS: " << static_cast<unsigned>(bypass_L1) << "\n"
              << std::setw(15) << "L2 BYPASS: " << static_cast<unsigned>(bypass_L2) << std::endl;
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
    tile_t pe_tile("   PE TILE");       // 1
    tile_t array_tile("ARRAY TILE");    // 2
    tile_t glb_tile("  GLB TILE");      // 3
    tile_t chip_tile(" CHIP TILE");     // 4
    mac_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::L0);
    mac_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::L0);
    mac_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::L0);
    pe_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::L1);
    pe_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::L1);
    pe_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::L1);
    array_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::Y);
    array_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::Y);
    array_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::Y);
    glb_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::L2);
    glb_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::L2);
    glb_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::L2);
    chip_tile.num_input = calculate_tile_size(data_type_t::INPUT, component_t::CHIP);
    chip_tile.num_weight = calculate_tile_size(data_type_t::WEIGHT, component_t::CHIP);
    chip_tile.num_output = calculate_tile_size(data_type_t::OUTPUT, component_t::CHIP);
    tiles.push_back(mac_tile);
    tiles.push_back(pe_tile);
    tiles.push_back(array_tile);
    tiles.push_back(glb_tile);
    tiles.push_back(chip_tile);
}

void tiles_t::print_stats() {
    std::cout << "\n# TILE SIZES" << std::endl;
    std::cout << std::setw(12) << "TILES |" 
              << std::setw(10) << "INPUT"
              << std::setw(10) << "WEIGHT"
              << std::setw(10) << "OUTPUT" << std::endl; 
    handler.print_line(43);
    for(size_t t = 0; t < tiles.size(); t++) {
        std::cout << tiles.at(t).name << " |" 
                  << std::setw(10) << tiles.at(t).num_input
                  << std::setw(10) << tiles.at(t).num_weight 
                  << std::setw(10) << tiles.at(t).num_output << std::endl;
    }
}

size_t tiles_t::calculate_tile_size(data_type_t type_, component_t U) {
    bool is_bypass_L1 = false;
    bool is_bypass_L2 = false;
    size_t rtn_size = 1;
    switch(type_) {
        case data_type_t::INPUT:
            // Bypass check
            is_bypass_L1 = (accelerator->bypass_L1 == bypass_t::IXX 
                         || accelerator->bypass_L1 == bypass_t::IXO
                         || accelerator->bypass_L1 == bypass_t::IWX
                         || accelerator->bypass_L1 == bypass_t::IWO) ? true : false;
            is_bypass_L2 = (accelerator->bypass_L2 == bypass_t::IXX 
                         || accelerator->bypass_L2 == bypass_t::IXO
                         || accelerator->bypass_L2 == bypass_t::IWX
                         || accelerator->bypass_L2 == bypass_t::IWO) ? true : false;
            rtn_size *= mapping_table->product(parameter_t::B, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= (mapping_table->product(parameter_t::P, U, is_bypass_L1, is_bypass_L2) - 1) * mapping_table->stride 
                      + mapping_table->product(parameter_t::R, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= (mapping_table->product(parameter_t::Q, U, is_bypass_L1, is_bypass_L2) - 1) * mapping_table->stride
                      + mapping_table->product(parameter_t::S, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= mapping_table->product(parameter_t::C, U, is_bypass_L1, is_bypass_L2);
            break;
        case data_type_t::WEIGHT:
            // Bypass check
            is_bypass_L1 = (accelerator->bypass_L1 == bypass_t::XWX 
                         || accelerator->bypass_L1 == bypass_t::XWO
                         || accelerator->bypass_L1 == bypass_t::IWX
                         || accelerator->bypass_L1 == bypass_t::IWO) ? true : false;
            is_bypass_L2 = (accelerator->bypass_L2 == bypass_t::XWX 
                         || accelerator->bypass_L2 == bypass_t::XWO
                         || accelerator->bypass_L2 == bypass_t::IWX
                         || accelerator->bypass_L2 == bypass_t::IWO) ? true : false;
            rtn_size *= mapping_table->product(parameter_t::K, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= mapping_table->product(parameter_t::R, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= mapping_table->product(parameter_t::S, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= mapping_table->product(parameter_t::C, U, is_bypass_L1, is_bypass_L2);
            break;
        case data_type_t::OUTPUT:
            // Bypass check
            is_bypass_L1 = (accelerator->bypass_L1 == bypass_t::XXO 
                         || accelerator->bypass_L1 == bypass_t::XWO
                         || accelerator->bypass_L1 == bypass_t::IXO
                         || accelerator->bypass_L1 == bypass_t::IWO) ? true : false;
            is_bypass_L2 = (accelerator->bypass_L2 == bypass_t::XXO
                         || accelerator->bypass_L2 == bypass_t::XWO
                         || accelerator->bypass_L2 == bypass_t::IXO
                         || accelerator->bypass_L2 == bypass_t::IWO) ? true : false;
            rtn_size *= mapping_table->product(parameter_t::K, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= mapping_table->product(parameter_t::B, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= mapping_table->product(parameter_t::P, U, is_bypass_L1, is_bypass_L2);
            rtn_size *= mapping_table->product(parameter_t::Q, U, is_bypass_L1, is_bypass_L2);
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
    //access_t glb_access("  GLB ACCESSES");
    access_t dram_access(" DRAM ACCESSES");
//    glb_access.cnts_input = calculate_access_counts(data_type_t::INPUT, component_t::L0);
//    glb_access.cnts_weight = calculate_access_counts(data_type_t::INPUT, component_t::L0);
//    glb_access.cnts_output = calculate_access_counts(data_type_t::INPUT, component_t::L0);
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
    handler.print_line(58);
    for(size_t a = 0; a < accesses.size(); a++) {
        std::cout << std::setw(15) << accesses.at(a).name << " |" 
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_input
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_weight
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_output
                  << std::setw(10) << std::fixed << std::setprecision(2) << accesses.at(a).mb_total << std::endl;
    }
}

size_t accesses_t::calculate_access_counts(data_type_t type_, component_t U) {
    bool is_bypass_L1 = false;
    bool is_bypass_L2 = false;
    size_t rtn_size = 1;
    switch(type_) {
        case data_type_t::INPUT:
            // DRAM accesses
            if(U == component_t::CHIP) {
                // L2 Bypass check
                is_bypass_L2 = (accelerator->bypass_L2 == bypass_t::IXX 
                             || accelerator->bypass_L2 == bypass_t::IXO
                             || accelerator->bypass_L2 == bypass_t::IWX
                             || accelerator->bypass_L2 == bypass_t::IWO) ? true : false;
                // GLB tile size x dividers
                rtn_size *= tiles->tiles.at(4).num_input;
                rtn_size *= mapping_table->divider(parameter_t::B, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::P, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::Q, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::C, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::R, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::S, U, is_bypass_L1, is_bypass_L2);
                // Multiply bypass effects 
                if(is_bypass_L2) {
                    rtn_size *= mapping_table->divider(parameter_t::K, U, is_bypass_L1, false);
                }
            }
            break;
        case data_type_t::WEIGHT:
            // DRAM accesses
            if(U == component_t::CHIP) {
                // L2 Bypass check
                is_bypass_L2 = (accelerator->bypass_L2 == bypass_t::XWX 
                             || accelerator->bypass_L2 == bypass_t::XWO
                             || accelerator->bypass_L2 == bypass_t::IWX
                             || accelerator->bypass_L2 == bypass_t::IWO) ? true : false;
                // GLB tile size x dividers
                rtn_size *= tiles->tiles.at(4).num_weight;
                rtn_size *= mapping_table->divider(parameter_t::K, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::R, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::S, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::C, U, is_bypass_L1, is_bypass_L2);
                // Multiply bypass effects 
                if(is_bypass_L2) {
                    rtn_size *= mapping_table->divider(parameter_t::B, U, is_bypass_L1, false);
                    rtn_size *= mapping_table->divider(parameter_t::P, U, is_bypass_L1, false);
                    rtn_size *= mapping_table->divider(parameter_t::Q, U, is_bypass_L1, false);
                }
            }
            break;
        case data_type_t::OUTPUT:
            // DRAM accesses
            if(U == component_t::CHIP) {
                // L2 Bypass check
                is_bypass_L2 = (accelerator->bypass_L2 == bypass_t::XXO
                             || accelerator->bypass_L2 == bypass_t::XWO
                             || accelerator->bypass_L2 == bypass_t::IXO
                             || accelerator->bypass_L2 == bypass_t::IWO) ? true : false;
                // GLB tile size x dividers
                rtn_size *= tiles->tiles.at(4).num_output;
                rtn_size *= mapping_table->divider(parameter_t::K, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::B, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::P, U, is_bypass_L1, is_bypass_L2);
                rtn_size *= mapping_table->divider(parameter_t::Q, U, is_bypass_L1, is_bypass_L2);
                // Multiply bypass effects 
                if(is_bypass_L2) {
                    rtn_size *= mapping_table->divider(parameter_t::R, U, is_bypass_L1, false);
                    rtn_size *= mapping_table->divider(parameter_t::S, U, is_bypass_L1, false);
                    rtn_size *= mapping_table->divider(parameter_t::C, U, is_bypass_L1, false);
                }
            }
            break;
        default: handler.print_err(err_type_t::INVAILD, "Data type");
    }
    return rtn_size;
}
