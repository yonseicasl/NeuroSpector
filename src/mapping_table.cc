#include "mapping_table.h"

static handler_t handler;

mapping_table_t::mapping_table_t()
    : stride(1) {
    layer_vals.assign(7, 1);
    degrees.assign(D_size * U_size, 1);
    U_str.push_back("MAC[S][T]");
    U_str.push_back(" L1  [T] ");
    U_str.push_back("  X  [S] ");
    U_str.push_back("  Y  [S] ");
    U_str.push_back(" L2  [T] ");
    U_str.push_back("  DRAM   ");
}

mapping_table_t::~mapping_table_t() {
}

void mapping_table_t::print_stats() {
    component_t last_component = static_cast<component_t>(U_size);
    parameter_t last_parameter = static_cast<parameter_t>(D_size);
    std::cout << "   U \\ D   |  ";
    for(const auto &d : enum_range<parameter_t>(last_parameter)) {
        std::cout << D_str.at(static_cast<unsigned>(d)) << "("
                  << std::setw(3) << layer_vals.at(static_cast<unsigned>(d)) << ") ";
    }
    std::cout << std::endl;
    handler.print_line(63, "-");
    for(const auto &u : enum_range<component_t>(last_component)) {
        std::cout << U_str.at(static_cast<unsigned>(u)) << "  |"; 
        for(const auto &d : enum_range<parameter_t>(last_parameter)) {
            std::cout << std::setw(7) << get_val(d, u);
        }
        std::cout << std::endl;
    }
    handler.print_line(63, "-");
    std::cout << "# TOTAL MACs                : " << layer_vals.at(0) * layer_vals.at(1) * layer_vals.at(2) 
                                                   * layer_vals.at(3) * layer_vals.at(4) * layer_vals.at(5) 
                                                   * layer_vals.at(6) << std::endl;
    std::cout << "# PE TOTAL ACTIVE PEs       : " << noc_info.total_active_pes << std::endl;
    std::cout << "# PE UTILIZATION            : " << noc_info.total_active_pes << " / TOTAL PEs" << std::endl;
    std::cout << "# REQUESTING INPUT PEs      : " << noc_info.requesting.input_pes << std::endl;
    std::cout << "# REQUESTING WEIGHT PEs     : " << noc_info.requesting.weight_pes << std::endl;
    std::cout << "# REQUESTING OUTPUT PEs     : " << noc_info.requesting.output_pes << std::endl;
    std::cout << "# NON-REQUESTING INPUT PEs  : " << noc_info.total_active_pes - noc_info.requesting.input_pes << std::endl;
    std::cout << "# NON-REQUESTING WEIGHT PEs : " << noc_info.total_active_pes - noc_info.requesting.weight_pes << std::endl;
    std::cout << "# NON-REQUESTING OUTPUT PEs : " << noc_info.total_active_pes - noc_info.requesting.output_pes << std::endl;
    print_tile_size();
    print_access_cnts();
}

void mapping_table_t::put_val(parameter_t D, component_t U, unsigned num_) {
    unsigned column = static_cast<unsigned>(D);
    unsigned row = static_cast<unsigned>(U);
    degrees.at(column + 7 * row) = num_;
}

unsigned mapping_table_t::get_val(parameter_t D, component_t U) {
    unsigned column = static_cast<unsigned>(D);
    unsigned row = static_cast<unsigned>(U);
    return degrees.at(column + 7 * row);
}

unsigned mapping_table_t::get_product(parameter_t D, component_t U) {
    component_t next_U = static_cast<component_t>(static_cast<unsigned>(U) + 1);
    unsigned column = static_cast<unsigned>(D);
    unsigned product = 1;
    for(const auto &u : enum_range<component_t>(next_U)) {
        unsigned row  = static_cast<unsigned>(u);
        product *= degrees.at(column + 7 * row);
    }
    return product;
}

unsigned mapping_table_t::get_reverse_product(parameter_t D, component_t U) {
    unsigned column = static_cast<unsigned>(D);
    unsigned product = 1;
    for(unsigned row = static_cast<unsigned>(U); row < static_cast<unsigned>(component_t::SIZE); row++) {
        product *= degrees.at(column + 7 * row);
    }
    return product;
}

void mapping_table_t::update_dram_row() {
    for(const auto &d : enum_range<parameter_t>(parameter_t::SIZE)) {
       unsigned val = layer_vals.at(static_cast<unsigned>(d)) / get_product(d, component_t::L2);
       unsigned remainder = layer_vals.at(static_cast<unsigned>(d)) % get_product(d, component_t::L2);
       if(remainder > 0)
           val++;
       put_val(d, component_t::DRAM, val); 
    }
}

void mapping_table_t::update_tile_size() {
    // MAC
    tile_sizes.MAC.input_tile = get_product(parameter_t::B, component_t::MAC) * get_product(parameter_t::C, component_t::MAC)
                              * ((get_product(parameter_t::P, component_t::MAC) - 1) * stride + get_product(parameter_t::R, component_t::MAC))
                              * ((get_product(parameter_t::Q, component_t::MAC) - 1) * stride + get_product(parameter_t::S, component_t::MAC));
    tile_sizes.MAC.weight_tile = get_product(parameter_t::K, component_t::MAC) * get_product(parameter_t::C, component_t::MAC) 
                              * get_product(parameter_t::R, component_t::MAC) * get_product(parameter_t::S, component_t::MAC);
    tile_sizes.MAC.output_tile = get_product(parameter_t::K, component_t::MAC) * get_product(parameter_t::B, component_t::MAC) 
                              * get_product(parameter_t::P, component_t::MAC) * get_product(parameter_t::Q, component_t::MAC);
    // L1
    tile_sizes.L1.input_tile = get_product(parameter_t::B, component_t::L1) * get_product(parameter_t::C, component_t::L1)
                              * ((get_product(parameter_t::P, component_t::L1) - 1) * stride + get_product(parameter_t::R, component_t::L1))
                              * ((get_product(parameter_t::Q, component_t::L1) - 1) * stride + get_product(parameter_t::S, component_t::L1));
    tile_sizes.L1.weight_tile = get_product(parameter_t::K, component_t::L1) * get_product(parameter_t::C, component_t::L1) 
                              * get_product(parameter_t::R, component_t::L1) * get_product(parameter_t::S, component_t::L1);
    tile_sizes.L1.output_tile = get_product(parameter_t::K, component_t::L1) * get_product(parameter_t::B, component_t::L1) 
                              * get_product(parameter_t::P, component_t::L1) * get_product(parameter_t::Q, component_t::L1);
    // L2
    tile_sizes.L2.input_tile = get_product(parameter_t::B, component_t::L2) * get_product(parameter_t::C, component_t::L2)
                              * ((get_product(parameter_t::P, component_t::L2) - 1) * stride + get_product(parameter_t::R, component_t::L2))
                              * ((get_product(parameter_t::Q, component_t::L2) - 1) * stride + get_product(parameter_t::S, component_t::L2));
    tile_sizes.L2.weight_tile = get_product(parameter_t::K, component_t::L2) * get_product(parameter_t::C, component_t::L2) 
                              * get_product(parameter_t::R, component_t::L2) * get_product(parameter_t::S, component_t::L2);
    tile_sizes.L2.output_tile = get_product(parameter_t::K, component_t::L2) * get_product(parameter_t::B, component_t::L2) 
                              * get_product(parameter_t::P, component_t::L2) * get_product(parameter_t::Q, component_t::L2);
    // DRAM
    tile_sizes.DRAM.input_tile = get_product(parameter_t::B, component_t::DRAM) * get_product(parameter_t::C, component_t::DRAM)
                              * ((get_product(parameter_t::P, component_t::DRAM) - 1) * stride + get_product(parameter_t::R, component_t::DRAM))
                              * ((get_product(parameter_t::Q, component_t::DRAM) - 1) * stride + get_product(parameter_t::S, component_t::DRAM));
    tile_sizes.DRAM.weight_tile = get_product(parameter_t::K, component_t::DRAM) * get_product(parameter_t::C, component_t::DRAM) 
                              * get_product(parameter_t::R, component_t::DRAM) * get_product(parameter_t::S, component_t::DRAM);
    tile_sizes.DRAM.output_tile = get_product(parameter_t::K, component_t::DRAM) * get_product(parameter_t::B, component_t::DRAM) 
                              * get_product(parameter_t::P, component_t::DRAM) * get_product(parameter_t::Q, component_t::DRAM);
}

void mapping_table_t::update_noc_info() {
    noc_info.total_active_pes = get_val(parameter_t::K, component_t::X) * get_val(parameter_t::B, component_t::X) 
                              * get_val(parameter_t::P, component_t::X) * get_val(parameter_t::Q, component_t::X)  
                              * get_val(parameter_t::C, component_t::X) * get_val(parameter_t::R, component_t::X)  
                              * get_val(parameter_t::S, component_t::X)   
                              * get_val(parameter_t::K, component_t::Y) * get_val(parameter_t::B, component_t::Y)  
                              * get_val(parameter_t::P, component_t::Y) * get_val(parameter_t::Q, component_t::Y)
                              * get_val(parameter_t::C, component_t::Y) * get_val(parameter_t::R, component_t::Y)
                              * get_val(parameter_t::S, component_t::Y); 
    noc_info.requesting.input_pes = get_val(parameter_t::B, component_t::X) * get_val(parameter_t::C, component_t::X) 
                                  * get_val(parameter_t::B, component_t::Y) * get_val(parameter_t::C, component_t::Y);
    unsigned input_p = get_val(parameter_t::P, component_t::X) * get_val(parameter_t::P, component_t::Y);
    unsigned input_r = get_val(parameter_t::R, component_t::X) * get_val(parameter_t::R, component_t::Y);
    unsigned input_q = get_val(parameter_t::Q, component_t::X) * get_val(parameter_t::Q, component_t::Y);
    unsigned input_s = get_val(parameter_t::S, component_t::X) * get_val(parameter_t::S, component_t::Y);
    if(input_p > 1 && input_r > 1) 
        noc_info.requesting.input_pes *= ((input_p - 1) * stride + input_r);
    else 
        noc_info.requesting.input_pes *= input_p * input_r;
    if(input_q > 1 && input_s > 1) 
        noc_info.requesting.input_pes *= ((input_q - 1) * stride + input_s);
    else 
        noc_info.requesting.input_pes *= input_q * input_s;
    noc_info.requesting.weight_pes = get_val(parameter_t::K, component_t::X) * get_val(parameter_t::C, component_t::X) 
                                   * get_val(parameter_t::R, component_t::X) * get_val(parameter_t::S, component_t::X)  
                                   * get_val(parameter_t::K, component_t::Y) * get_val(parameter_t::C, component_t::Y)  
                                   * get_val(parameter_t::R, component_t::Y) * get_val(parameter_t::S, component_t::Y);
    noc_info.requesting.output_pes = get_val(parameter_t::K, component_t::X) * get_val(parameter_t::B, component_t::X) 
                                   * get_val(parameter_t::P, component_t::X) * get_val(parameter_t::Q, component_t::X)  
                                   * get_val(parameter_t::K, component_t::Y) * get_val(parameter_t::B, component_t::Y)  
                                   * get_val(parameter_t::P, component_t::Y) * get_val(parameter_t::Q, component_t::Y);
}

void mapping_table_t::update_access_cnts() {
    update_noc_info();
    // L1 access counts
    size_t L1_total_access_cnts = get_reverse_product(parameter_t::K, component_t::L1) * get_reverse_product(parameter_t::B, component_t::L1) 
                                * get_reverse_product(parameter_t::P, component_t::L1) * get_reverse_product(parameter_t::Q, component_t::L1) 
                                * get_reverse_product(parameter_t::C, component_t::L1) * get_reverse_product(parameter_t::R, component_t::L1) 
                                * get_reverse_product(parameter_t::S, component_t::L1);  
    // IS
    L1_access_cnts.is.input_cnts = L1_total_access_cnts / get_val(parameter_t::K, component_t::L1);
    L1_access_cnts.is.weight_cnts = L1_total_access_cnts; 
    L1_access_cnts.is.output_cnts = L1_total_access_cnts * 2;
    // WS
    L1_access_cnts.ws.input_cnts = L1_total_access_cnts; 
    L1_access_cnts.ws.weight_cnts = L1_total_access_cnts / get_val(parameter_t::B, component_t::L1) 
                                  / get_val(parameter_t::P, component_t::L1) / get_val(parameter_t::Q, component_t::L1);
    L1_access_cnts.ws.output_cnts = L1_total_access_cnts * 2;
    // OS
    L1_access_cnts.os.input_cnts = L1_total_access_cnts;
    L1_access_cnts.os.weight_cnts = L1_total_access_cnts;
    L1_access_cnts.os.output_cnts = L1_total_access_cnts / get_val(parameter_t::C, component_t::L1) 
                                  / get_val(parameter_t::R, component_t::L1) / get_val(parameter_t::S, component_t::L1);

    // L2 access counts
    size_t L2_total_access_cnts = get_reverse_product(parameter_t::K, component_t::L2) * get_reverse_product(parameter_t::B, component_t::L2) 
                                * get_reverse_product(parameter_t::P, component_t::L2) * get_reverse_product(parameter_t::Q, component_t::L2) 
                                * get_reverse_product(parameter_t::C, component_t::L2) * get_reverse_product(parameter_t::R, component_t::L2) 
                                * get_reverse_product(parameter_t::S, component_t::L2);  
    // IS
    L2_access_cnts.is.input_cnts = L2_total_access_cnts / get_val(parameter_t::K, component_t::L2);
    L2_access_cnts.is.weight_cnts = L2_total_access_cnts; 
    L2_access_cnts.is.output_cnts = L2_total_access_cnts * 2;
    // WS
    L2_access_cnts.ws.input_cnts = L2_total_access_cnts; 
    L2_access_cnts.ws.weight_cnts = L2_total_access_cnts / get_val(parameter_t::B, component_t::L2) 
                                  / get_val(parameter_t::P, component_t::L2) / get_val(parameter_t::Q, component_t::L2);
    L2_access_cnts.ws.output_cnts = L2_total_access_cnts * 2;
    // OS
    L2_access_cnts.os.input_cnts = L2_total_access_cnts;
    L2_access_cnts.os.weight_cnts = L2_total_access_cnts;
    L2_access_cnts.os.output_cnts = L2_total_access_cnts / get_val(parameter_t::C, component_t::L2) 
                                  / get_val(parameter_t::R, component_t::L2) / get_val(parameter_t::S, component_t::L2);
    if(noc_exists) {
        unsigned non_requesting_input_pes = noc_info.total_active_pes - noc_info.requesting.input_pes;
        unsigned non_requesting_weight_pes = noc_info.total_active_pes - noc_info.requesting.weight_pes;
        unsigned non_requesting_output_pes = noc_info.total_active_pes - noc_info.requesting.input_pes;
        // NoC access counts
        // IS
        noc_access_cnts.is.input_cnts = L2_access_cnts.is.input_cnts * non_requesting_input_pes;
        noc_access_cnts.is.weight_cnts = L2_access_cnts.is.weight_cnts * non_requesting_weight_pes;
        noc_access_cnts.is.output_cnts = L2_access_cnts.is.output_cnts * non_requesting_output_pes;
        // WS
        noc_access_cnts.ws.input_cnts = L2_access_cnts.ws.input_cnts * non_requesting_input_pes;
        noc_access_cnts.ws.weight_cnts = L2_access_cnts.ws.weight_cnts * non_requesting_weight_pes;
        noc_access_cnts.ws.output_cnts = L2_access_cnts.ws.output_cnts * non_requesting_output_pes;
        // OS
        noc_access_cnts.os.input_cnts = L2_access_cnts.os.input_cnts * non_requesting_input_pes;
        noc_access_cnts.os.weight_cnts = L2_access_cnts.os.weight_cnts * non_requesting_weight_pes;
        noc_access_cnts.os.output_cnts = L2_access_cnts.os.output_cnts * non_requesting_output_pes;
        // Multiply # of requesting PEs
        L2_access_cnts.is.input_cnts *= noc_info.requesting.input_pes;
        L2_access_cnts.is.weight_cnts *= noc_info.requesting.weight_pes; 
        L2_access_cnts.is.output_cnts *= noc_info.requesting.output_pes;
        L2_access_cnts.ws.input_cnts *= noc_info.requesting.input_pes; 
        L2_access_cnts.ws.weight_cnts *= noc_info.requesting.weight_pes;
        L2_access_cnts.ws.output_cnts *= noc_info.requesting.output_pes;
        L2_access_cnts.os.input_cnts *= noc_info.requesting.input_pes;
        L2_access_cnts.os.weight_cnts *= noc_info.requesting.weight_pes;
        L2_access_cnts.os.output_cnts *= noc_info.requesting.output_pes;
    }
    else {
        // Multiply # of active PEs
        L2_access_cnts.is.input_cnts *= noc_info.total_active_pes;
        L2_access_cnts.is.weight_cnts *=noc_info.total_active_pes; 
        L2_access_cnts.is.output_cnts *=noc_info.total_active_pes;
        L2_access_cnts.ws.input_cnts *= noc_info.total_active_pes; 
        L2_access_cnts.ws.weight_cnts *= noc_info.total_active_pes;
        L2_access_cnts.ws.output_cnts *= noc_info.total_active_pes;
        L2_access_cnts.os.input_cnts *= noc_info.total_active_pes;
        L2_access_cnts.os.weight_cnts *= noc_info.total_active_pes;
        L2_access_cnts.os.output_cnts *= noc_info.total_active_pes;
        // NoC access counts: all 0
    }
    // DRAM access counts
    size_t DRAM_total_access_cnts = get_val(parameter_t::K, component_t::DRAM) * get_val(parameter_t::B, component_t::DRAM) 
                                  * get_val(parameter_t::P, component_t::DRAM) * get_val(parameter_t::Q, component_t::DRAM) 
                                  * get_val(parameter_t::C, component_t::DRAM) * get_val(parameter_t::R, component_t::DRAM) 
                                  * get_val(parameter_t::S, component_t::DRAM);  
    // IS
    DRAM_access_cnts.is.input_cnts = DRAM_total_access_cnts / get_val(parameter_t::K, component_t::DRAM);
    DRAM_access_cnts.is.weight_cnts = DRAM_total_access_cnts; 
    DRAM_access_cnts.is.output_cnts = DRAM_total_access_cnts * 2;
    // WS
    DRAM_access_cnts.ws.input_cnts = DRAM_total_access_cnts; 
    DRAM_access_cnts.ws.weight_cnts = DRAM_total_access_cnts / get_val(parameter_t::B, component_t::DRAM) 
                                    / get_val(parameter_t::P, component_t::DRAM) / get_val(parameter_t::Q, component_t::DRAM);
    DRAM_access_cnts.ws.output_cnts = DRAM_total_access_cnts * 2;
    // OS
    DRAM_access_cnts.os.input_cnts = DRAM_total_access_cnts;
    DRAM_access_cnts.os.weight_cnts = DRAM_total_access_cnts;
    DRAM_access_cnts.os.output_cnts = DRAM_total_access_cnts / get_val(parameter_t::C, component_t::DRAM) 
                                    / get_val(parameter_t::R, component_t::DRAM) / get_val(parameter_t::S, component_t::DRAM);
}

void mapping_table_t::print_tile_size() {
    std::cout << "# TILE SIZES" << std::endl;
    std::cout << std::setw(12) << "TILES  |" 
              << std::setw(10) << "INPUT"
              << std::setw(10) << "WEIGHT"
              << std::setw(10) << "OUTPUT" << std::endl; 
    handler.print_line(43, "-");
    std::cout << std::setw(12) << "  MAC  |" 
              << std::setw(10) << tile_sizes.MAC.input_tile
              << std::setw(10) << tile_sizes.MAC.weight_tile
              << std::setw(10) << tile_sizes.MAC.output_tile << std::endl; 
    std::cout << std::setw(12) << "  L1   |" 
              << std::setw(10) << tile_sizes.L1.input_tile
              << std::setw(10) << tile_sizes.L1.weight_tile
              << std::setw(10) << tile_sizes.L1.output_tile << std::endl; 
    std::cout << std::setw(12) << "  L2   |" 
              << std::setw(10) << tile_sizes.L2.input_tile
              << std::setw(10) << tile_sizes.L2.weight_tile
              << std::setw(10) << tile_sizes.L2.output_tile << std::endl; 
    std::cout << std::setw(12) << " DRAM  |" 
              << std::setw(10) << tile_sizes.DRAM.input_tile
              << std::setw(10) << tile_sizes.DRAM.weight_tile
              << std::setw(10) << tile_sizes.DRAM.output_tile << std::endl; 
    handler.print_line(43, "-");
}

void mapping_table_t::print_access_cnts() {
    std::cout << "# DRAM ACCESS COUNTS" << std::endl;
    std::cout << std::setw(12) << " DATAFLOW |" 
              << std::setw(15) << " IS "
              << std::setw(15) << " WS "
              << std::setw(15) << " OS " << std::endl; 
    handler.print_line(60, "-");
    std::cout << std::setw(12) << "   INPUT  |" 
              << std::setw(15) << DRAM_access_cnts.is.input_cnts 
              << std::setw(15) << DRAM_access_cnts.ws.input_cnts
              << std::setw(15) << DRAM_access_cnts.os.input_cnts << std::endl; 
    std::cout << std::setw(12) << "  FILTER  |" 
              << std::setw(15) << DRAM_access_cnts.is.weight_cnts 
              << std::setw(15) << DRAM_access_cnts.ws.weight_cnts
              << std::setw(15) << DRAM_access_cnts.os.weight_cnts << std::endl; 
    std::cout << std::setw(12) << "  OUTPUT  |" 
              << std::setw(15) << DRAM_access_cnts.is.output_cnts 
              << std::setw(15) << DRAM_access_cnts.ws.output_cnts
              << std::setw(15) << DRAM_access_cnts.os.output_cnts << std::endl; 
    handler.print_line(60, "-");
    std::cout << "# L2 ACCESS COUNTS (x REQUESTING PEs)" << std::endl;
    std::cout << std::setw(12) << " DATAFLOW |" 
              << std::setw(15) << " IS "
              << std::setw(15) << " WS "
              << std::setw(15) << " OS " << std::endl; 
    handler.print_line(60, "-");
    std::cout << std::setw(12) << "   INPUT  |" 
              << std::setw(15) << L2_access_cnts.is.input_cnts
              << std::setw(15) << L2_access_cnts.ws.input_cnts
              << std::setw(15) << L2_access_cnts.os.input_cnts << std::endl; 
    std::cout << std::setw(12) << "  FILTER  |" 
              << std::setw(15) << L2_access_cnts.is.weight_cnts
              << std::setw(15) << L2_access_cnts.ws.weight_cnts
              << std::setw(15) << L2_access_cnts.os.weight_cnts << std::endl; 
    std::cout << std::setw(12) << "  OUTPUT  |" 
              << std::setw(15) << L2_access_cnts.is.output_cnts
              << std::setw(15) << L2_access_cnts.ws.output_cnts
              << std::setw(15) << L2_access_cnts.os.output_cnts << std::endl; 
    handler.print_line(60, "-");
    std::cout << "# NoC ACCESS COUNTS (x NON-REQUESTING PEs)" << std::endl;
    std::cout << std::setw(12) << " DATAFLOW |" 
              << std::setw(15) << " IS "
              << std::setw(15) << " WS "
              << std::setw(15) << " OS " << std::endl; 
    handler.print_line(60, "-");
    std::cout << std::setw(12) << "   INPUT  |" 
              << std::setw(15) << noc_access_cnts.is.input_cnts
              << std::setw(15) << noc_access_cnts.ws.input_cnts
              << std::setw(15) << noc_access_cnts.os.input_cnts << std::endl; 
    std::cout << std::setw(12) << "  FILTER  |" 
              << std::setw(15) << noc_access_cnts.is.weight_cnts
              << std::setw(15) << noc_access_cnts.ws.weight_cnts
              << std::setw(15) << noc_access_cnts.os.weight_cnts << std::endl; 
    std::cout << std::setw(12) << "  OUTPUT  |" 
              << std::setw(15) << noc_access_cnts.is.output_cnts
              << std::setw(15) << noc_access_cnts.ws.output_cnts
              << std::setw(15) << noc_access_cnts.os.output_cnts << std::endl; 
    handler.print_line(60, "-");
    std::cout << "# L1 ACCESS COUNTS (x TOTAL ACTIVE PEs)" << std::endl;
    std::cout << std::setw(12) << " DATAFLOW |" 
              << std::setw(15) << " IS "
              << std::setw(15) << " WS "
              << std::setw(15) << " OS " << std::endl; 
    handler.print_line(60, "-");
    std::cout << std::setw(12) << "   INPUT  |" 
              << std::setw(15) << L1_access_cnts.is.input_cnts
              << std::setw(15) << L1_access_cnts.ws.input_cnts
              << std::setw(15) << L1_access_cnts.os.input_cnts << std::endl; 
    std::cout << std::setw(12) << "  FILTER  |" 
              << std::setw(15) << L1_access_cnts.is.weight_cnts
              << std::setw(15) << L1_access_cnts.ws.weight_cnts
              << std::setw(15) << L1_access_cnts.os.weight_cnts << std::endl; 
    std::cout << std::setw(12) << "  OUTPUT  |" 
              << std::setw(15) << L1_access_cnts.is.output_cnts
              << std::setw(15) << L1_access_cnts.ws.output_cnts
              << std::setw(15) << L1_access_cnts.os.output_cnts << std::endl; 
    handler.print_line(60, "-");
}

//unsigned mapping_table_t::get_row_product(component_t U) {
//    parameter_t last_parameter = static_cast<parameter_t>(D_size);
//    unsigned product = 1;
//    for(const auto &d : enum_range<parameter_t>(last_parameter)) {
//        product *= get_val(d, U);
//    }
//    return product;
//}

//void mapping_table_t::expand(component_t U, unsigned max_expanded) { 
//    parameter_t last_parameter = static_cast<parameter_t>(D_size);
//    unsigned cnt = 0;
//    unsigned max_idx = 0;
//    unsigned max_val = 1;
//    for(const auto &d : enum_range<parameter_t>(last_parameter)) {
//        if(cnt == 0) 
//            max_val = layer_vals.at(cnt) / get_product(d, component_t::L2);
//        else {
//            if(max_val < layer_vals.at(cnt) / get_product(d, component_t::L2)) {
//                max_val = layer_vals.at(cnt) / get_product(d, component_t::L2);
//                max_idx = cnt;
//            }
//        }
//        cnt++;
//    }
//    if(max_expanded <= max_val) {
//        // TODO: eyeriss conv2
//        put_val(static_cast<parameter_t>(max_idx), U, max_expanded);
//    }
//    else {
//        put_val(static_cast<parameter_t>(max_idx), U, max_val);
//    }
//    // TODO: if not full -> next max
//    return;
//}
//
//void mapping_table_t::alpha_expand(component_t U, unsigned max_expanded) { 
//    unsigned max_val = layer_vals.at(0) / get_product(parameter_t::K, component_t::L2, 0, 0);
//    if(max_expanded <= max_val) {
//        // TODO: eyeriss conv2
//        put_val(parameter_t::K, U, max_expanded);
//    }
//    else {
//        put_val(parameter_t::K, U, max_val);
//    }
//    // TODO: if not full -> next max
//    return;
//}
//
//void mapping_table_t::beta_expand(component_t U, unsigned max_expanded) { 
//    return;
//}
//
//void mapping_table_t::gamma_expand(component_t U, unsigned max_expanded) { 
//    return;
//}
