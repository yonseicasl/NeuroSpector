#include "accelerator.h"

static handler_t handler;

/* Accelerator */
accelerator_t::accelerator_t(const std::string &cfg_path_)
    : acc_cfg(new acc_cfg_t(cfg_path_)) {

}

accelerator_t::~accelerator_t() {
    delete acc_cfg;
}

// Accelerator APIs
void accelerator_t::print_stats() const {
    unsigned setw_num = 20;
    unsigned line_num = 40;
    std::string mac_type_str;
    std::string precision_str;
    std::string mac_dataflow_str;
    std::string l1_dataflow_str;
    std::string l2_dataflow_str;

    if(acc_cfg->macs_per_pe == 1 && acc_cfg->mac_width == 1) mac_type_str = "SDSO";
    else if(acc_cfg->macs_per_pe > 1 && acc_cfg->mac_width == 1) mac_type_str = "MDMO";
    else if(acc_cfg->macs_per_pe == 1 && acc_cfg->mac_width > 1) mac_type_str = "SVSO";
    else if(acc_cfg->macs_per_pe > 1 && acc_cfg->mac_width > 1) mac_type_str = "MVMO";
    else handler.print_err(err_type_t::INVAILD, "MAC VARIABLES");

    switch(acc_cfg->precision) {
        case precision_t::FP8: precision_str = "FP8"; break;
        case precision_t::FP16: precision_str = "FP16"; break;
        case precision_t::FP32: precision_str = "FP32"; break;
        case precision_t::INT8: precision_str = "INT8"; break;
        case precision_t::INT16: precision_str = "INT16"; break;
        case precision_t::INT32: precision_str = "INT32"; break;
        default: handler.print_err(err_type_t::INVAILD, "PRECISION"); break;
    }

    switch(acc_cfg->mac_dataflow) {
        case dataflow_t::IS: mac_dataflow_str = "IS"; break;
        case dataflow_t::WS: mac_dataflow_str = "WS"; break;
        case dataflow_t::NONE: mac_dataflow_str = "NONE"; break;
        default: handler.print_err(err_type_t::INVAILD, "MAC DATAFLOW"); break;
    }

    switch(acc_cfg->l1_dataflow) {
        case dataflow_t::IS: l1_dataflow_str = "IS"; break;
        case dataflow_t::WS: l1_dataflow_str = "WS"; break;
        case dataflow_t::OS: l1_dataflow_str = "OS"; break;
        default: handler.print_err(err_type_t::INVAILD, "L1 DATAFLOW"); break;
    }

    switch(acc_cfg->l2_dataflow) {
        case dataflow_t::IS: l2_dataflow_str = "IS"; break;
        case dataflow_t::WS: l2_dataflow_str = "WS"; break;
        case dataflow_t::OS: l2_dataflow_str = "OS"; break;
        default: handler.print_err(err_type_t::INVAILD, "L2 DATAFLOW"); break;
    }
    // Start printing out
    std::cout << "# ACCELERATOR: " << acc_cfg->name << std::endl;
    handler.print_line(line_num, "-");
    // MAC [T]
    std::cout << "# MAC [T]" << std::endl;
    handler.print_line(setw_num, "*");
    std::cout << std::setw(setw_num) << "PRECISION   : " << precision_str << "\n"
              << std::setw(setw_num) << "MAC DATAFLOW: " << mac_dataflow_str << std::endl;
    handler.print_line(line_num, "-");
    // S0 [S]
    std::cout << "# S0 [S]" << std::endl;
    handler.print_line(setw_num, "*");
    std::cout << std::setw(setw_num) << "MAC UNIT TYPE: " << mac_type_str << "\n"
              << std::setw(setw_num) << "MACS PER PE  : " << acc_cfg->macs_per_pe << "\n"
              << std::setw(setw_num) << "MAC WIDTH    : " << acc_cfg->mac_width << std::endl;
    handler.print_line(line_num, "-");
    // L1 [T]
    if(acc_cfg->l1_type != buffer_type_t::NONE) {
        std::cout << "# L1 [T]" << std::endl;
        handler.print_line(setw_num, "*");
        if(acc_cfg->l1_type == buffer_type_t::NONE)
            std::cout << "NONE" << std::endl;
        else {
                std::cout << std::setw(setw_num) << "L1 DATAFLOW   : " << l1_dataflow_str << std::endl;
            switch(acc_cfg->l1_type) {
                case buffer_type_t::SEPARATED: 
                    std::cout << std::setw(setw_num) << "L1 TYPE       : " << "SEPARATED "<< std::endl;
                    if(acc_cfg->l1_input_bypass)
                        std::cout << std::setw(setw_num) << "L1 INPUT SIZE : " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L1 INPUT SIZE : " << std::setw(4) << acc_cfg->l1_input_size << " B" << std::endl;
                    if(acc_cfg->l1_filter_bypass)
                        std::cout << std::setw(setw_num) << "L1 FILTER SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L1 FILTER SIZE: " << std::setw(4) << acc_cfg->l1_filter_size << " B" << std::endl;
                    if(acc_cfg->l1_output_bypass)
                        std::cout << std::setw(setw_num) << "L1 OUTPUT SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L1 OUTPUT SIZE: " << std::setw(4) << acc_cfg->l1_output_size << " B" << std::endl;
                    break;
                case buffer_type_t::SHARED: 
                    std::cout 
                    << std::setw(setw_num) << "L1 TYPE       : " << "SHARED" << "\n"
                    << std::setw(setw_num) << "L1 SHARED SIZE: " << std::setw(4) << acc_cfg->l1_shared_size << " B" << std::endl;
                    break;
                case buffer_type_t::SHARED_IF: 
                    std::cout << std::setw(setw_num) << "L1 TYPE       : " << "SHARED_IF" << "\n"
                              << std::setw(setw_num) << "L1 SHARED SIZE: " << std::setw(4) << acc_cfg->l1_shared_size << " B" << std::endl;
                    if(acc_cfg->l1_output_bypass)
                        std::cout << std::setw(setw_num) << "L1 OUTPUT SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L1 OUTPUT SIZE: " << std::setw(4) << acc_cfg->l1_output_size << " B" << std::endl;
                    break;
                case buffer_type_t::SHARED_FO: 
                    std::cout << std::setw(setw_num) << "L1 TYPE       : " << "SHARED_FO" << "\n"
                              << std::setw(setw_num) << "L1 SHARED SIZE: " << std::setw(4) << acc_cfg->l1_shared_size << " B" << std::endl;
                    if(acc_cfg->l1_input_bypass)
                        std::cout << std::setw(setw_num) << "L1 INPUT SIZE : " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L1 INPUT SIZE : " << std::setw(4) << acc_cfg->l1_input_size << " B" << std::endl;
                    break;
                case buffer_type_t::SHARED_OI: 
                    std::cout << std::setw(setw_num) << "L1 TYPE       : " << "SHARED_OI" << "\n"
                              << std::setw(setw_num) << "L1 SHARED SIZE: " << std::setw(4) << acc_cfg->l1_shared_size << " B" << std::endl;
                    if(acc_cfg->l1_filter_bypass)
                        std::cout << std::setw(setw_num) << "L1 FILTER SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L1 FILTER SIZE: " << std::setw(4) << acc_cfg->l1_filter_size << " B" << std::endl;
                    break;
                default: handler.print_err(err_type_t::INVAILD, "L1 TYPE"); break;
            }
        }
        handler.print_line(line_num, "-");
    }
    // S1_X & S1_Y [S]
    std::cout << "# S1_X & S1_Y [S]" << std::endl;
    handler.print_line(setw_num, "*");
    std::cout << std::setw(setw_num) << "S1 NOC EXISTS: " << acc_cfg->s1_noc_exists << "\n"
              << std::setw(setw_num) << "S1 SIZE X    : " << acc_cfg->s1_size_x << "\n"
              << std::setw(setw_num) << "S1 SIZE Y    : " << acc_cfg->s1_size_y << std::endl;;
    handler.print_line(line_num, "-");
    // L2 [T]
    if(acc_cfg->l2_type != buffer_type_t::NONE) {
        std::cout << "# L2 [T]" << std::endl;
        handler.print_line(setw_num, "*");
        if(acc_cfg->l2_type == buffer_type_t::NONE)
            std::cout << "NONE" << std::endl;
        else {
                std::cout << std::setw(setw_num) << "L2 DATAFLOW   : " << l2_dataflow_str << std::endl;
            switch(acc_cfg->l2_type) {
                case buffer_type_t::SEPARATED: 
                    std::cout << std::setw(setw_num) << "L2 TYPE       : " << "SEPARATED "<< std::endl;
                    if(acc_cfg->l2_input_bypass)
                        std::cout << std::setw(setw_num) << "L2 INPUT SIZE : " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L2 INPUT SIZE : " << std::setw(4) << acc_cfg->l2_input_size << " KB" << std::endl;
                    if(acc_cfg->l2_filter_bypass)
                        std::cout << std::setw(setw_num) << "L2 FILTER SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L2 FILTER SIZE: " << std::setw(4) << acc_cfg->l2_filter_size << " KB" << std::endl;
                    if(acc_cfg->l2_output_bypass)
                        std::cout << std::setw(setw_num) << "L2 OUTPUT SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L2 OUTPUT SIZE: " << std::setw(4) << acc_cfg->l2_output_size << " KB" << std::endl;
                    break;
                case buffer_type_t::SHARED: 
                    std::cout << std::setw(setw_num) << "L2 TYPE       : " << "SHARED" << "\n"
                              << std::setw(setw_num) << "L2 SHARED SIZE: " << std::setw(4) << acc_cfg->l2_shared_size << " KB" << std::endl;
                    break;
                case buffer_type_t::SHARED_IF: 
                    std::cout << std::setw(setw_num) << "L2 TYPE       : " << "SHARED_IF" << "\n"
                              << std::setw(setw_num) << "L2 SHARED SIZE: " << std::setw(4) << acc_cfg->l2_shared_size << " KB" << std::endl;
                    if(acc_cfg->l2_output_bypass)
                        std::cout << std::setw(setw_num) << "L2 OUTPUT SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L2 OUTPUT SIZE: " << std::setw(4) << acc_cfg->l2_output_size << " KB" << std::endl;
                    break;
                case buffer_type_t::SHARED_FO: 
                    std::cout << std::setw(setw_num) << "L2 TYPE       : " << "SHARED_FO" << "\n"
                              << std::setw(setw_num) << "L2 SHARED SIZE: " << std::setw(4) << acc_cfg->l2_shared_size << " KB" << std::endl;
                    if(acc_cfg->l2_input_bypass)
                        std::cout << std::setw(setw_num) << "L2 INPUT SIZE : " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L2 INPUT SIZE : " << std::setw(4) << acc_cfg->l2_input_size << " KB" << std::endl;
                    break;
                case buffer_type_t::SHARED_OI: 
                    std::cout << std::setw(setw_num) << "L2 TYPE       : " << "SHARED_OI" << "\n"
                              << std::setw(setw_num) << "L2 SHARED SIZE: " << std::setw(4) << acc_cfg->l2_shared_size << " KB" << std::endl;
                    if(acc_cfg->l2_filter_bypass)
                        std::cout << std::setw(setw_num) << "L2 FILTER SIZE: " << "BYPASS" << std::endl;
                    else
                        std::cout << std::setw(setw_num) << "L2 FILTER SIZE: " << std::setw(4) << acc_cfg->l2_filter_size << " KB" << std::endl;
                    break;
                default: handler.print_err(err_type_t::INVAILD, "L2 TYPE"); break;
            }
        }
        handler.print_line(line_num, "-");
    }
    // S2 [S]
    if(acc_cfg->s2_size > 1) {
        std::cout << "# S2 [S]" << std::endl;
        handler.print_line(setw_num, "*");
        std::cout << std::setw(setw_num) << "S2 SIZE      : " << acc_cfg->s2_size << std::endl;
        handler.print_line(line_num, "-");
    }
    // Energy references 
    std::cout << "# ENERGY REFERENCES" << std::endl;
    handler.print_line(setw_num, "*");
    std::cout << "   MAC OPERATION           : " << acc_cfg->energy_ref.mac_operation << std::endl;
    handler.print_line(line_num, "-");
    std::cout << "   L1 INPUT  INGRESS/EGRESS: " << acc_cfg->energy_ref.l1_input_ingress << "/" << acc_cfg->energy_ref.l1_input_egress << "\n"
              << "   L1 FILTER INGRESS/EGRESS: " << acc_cfg->energy_ref.l1_filter_ingress << "/" << acc_cfg->energy_ref.l1_filter_egress << "\n"
              << "   L1 OUTPUT INGRESS/EGRESS: " << acc_cfg->energy_ref.l1_output_ingress << "/" << acc_cfg->energy_ref.l1_output_egress << std::endl;
    handler.print_line(line_num, "-");
    std::cout << "   L2 INPUT  INGRESS/EGRESS: " << acc_cfg->energy_ref.l2_input_ingress << "/" << acc_cfg->energy_ref.l2_input_egress << "\n"
              << "   L2 FILTER INGRESS/EGRESS: " << acc_cfg->energy_ref.l2_filter_ingress << "/" << acc_cfg->energy_ref.l2_filter_egress << "\n"
              << "   L2 OUTPUT INGRESS/EGRESS: " << acc_cfg->energy_ref.l2_output_ingress << "/" << acc_cfg->energy_ref.l2_output_egress << std::endl;
    handler.print_line(line_num, "-");
    std::cout << " - DRAM INGRESS/EGRESS     : " << acc_cfg->energy_ref.dram_ingress << "/" << acc_cfg->energy_ref.dram_egress << std::endl;
    handler.print_line(line_num, "-");
}

std::string accelerator_t::get_name() const { return acc_cfg->name; }

// MAC [T] 
precision_t accelerator_t::precision() const { return acc_cfg->precision; }
dataflow_t accelerator_t::mac_dataflow() const { return acc_cfg->mac_dataflow; }

// S0 [S]
unsigned accelerator_t::macs_per_pe() const { return acc_cfg->macs_per_pe; }
unsigned accelerator_t::mac_width() const { return acc_cfg->mac_width; }

// L1 [T]
bool accelerator_t::l1_input_bypass() const { return acc_cfg->l1_input_bypass; }
bool accelerator_t::l1_filter_bypass() const { return acc_cfg->l1_filter_bypass; }
bool accelerator_t::l1_output_bypass() const { return acc_cfg->l1_output_bypass; }
unsigned accelerator_t::l1_input_size() const { 
    unsigned rtn = acc_cfg->l1_input_size * 8;  // Byte to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
unsigned accelerator_t::l1_filter_size() const { 
    unsigned rtn = acc_cfg->l1_filter_size * 8;  // Byte to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
unsigned accelerator_t::l1_output_size() const { 
    unsigned rtn = acc_cfg->l1_output_size * 8;  // Byte to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
unsigned accelerator_t::l1_shared_size() const { 
    unsigned rtn = acc_cfg->l1_shared_size * 8;  // Byte to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
buffer_type_t accelerator_t::l1_type() const { return acc_cfg->l1_type; } 
dataflow_t accelerator_t::l1_dataflow() const { return acc_cfg->l1_dataflow; }

// S1_X & S1_Y [S]
bool accelerator_t::s1_noc_exists() const { return acc_cfg->s1_noc_exists; }
unsigned accelerator_t::s1_size_x() const { return acc_cfg->s1_size_x; }
unsigned accelerator_t::s1_size_y() const { return acc_cfg->s1_size_y; }
std::string accelerator_t::s1_constraints_x() const { return acc_cfg->s1_constraints_x; }
std::string accelerator_t::s1_constraints_y() const { return acc_cfg->s1_constraints_y; }

// L2 [T]
bool accelerator_t::l2_input_bypass() const { return acc_cfg->l2_input_bypass; }
bool accelerator_t::l2_filter_bypass() const { return acc_cfg->l2_filter_bypass; }
bool accelerator_t::l2_output_bypass() const { return acc_cfg->l2_output_bypass; }
unsigned accelerator_t::l2_input_size() const { 
    unsigned rtn = acc_cfg->l2_input_size * 8 * 1024;  // KB to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
unsigned accelerator_t::l2_filter_size() const { 
    unsigned rtn = acc_cfg->l2_filter_size * 8 * 1024;  // KB to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
unsigned accelerator_t::l2_output_size() const { 
    unsigned rtn = acc_cfg->l2_output_size * 8 * 1024;  // KB to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
unsigned accelerator_t::l2_shared_size() const { 
    unsigned rtn = acc_cfg->l2_shared_size * 8 * 1024;  // KB to bit
    if(acc_cfg->precision == precision_t::FP8 || acc_cfg->precision == precision_t::INT8) rtn /= 8;
    else if(acc_cfg->precision == precision_t::FP16 || acc_cfg->precision == precision_t::INT16) rtn /= 16; 
    else if(acc_cfg->precision == precision_t::FP32 || acc_cfg->precision == precision_t::INT32) rtn /= 32;
    else handler.print_err(err_type_t::INVAILD, "PRECISION"); 
    return rtn; 
}
buffer_type_t accelerator_t::l2_type() const { return acc_cfg->l2_type; }
dataflow_t accelerator_t::l2_dataflow() const { return acc_cfg->l2_dataflow; }

// S2 [S]
unsigned accelerator_t::s2_size() const { return acc_cfg->s2_size; }

// Energy references
float accelerator_t::E_mac_op() const { return acc_cfg->energy_ref.mac_operation; }
float accelerator_t::E_l1_i_igrs() const { return acc_cfg->energy_ref.l1_input_ingress; }
float accelerator_t::E_l1_i_egrs() const { return acc_cfg->energy_ref.l1_input_egress; }
float accelerator_t::E_l1_f_igrs() const { return acc_cfg->energy_ref.l1_filter_ingress; }
float accelerator_t::E_l1_f_egrs() const { return acc_cfg->energy_ref.l1_filter_egress; }
float accelerator_t::E_l1_o_igrs() const { return acc_cfg->energy_ref.l1_output_ingress; }
float accelerator_t::E_l1_o_egrs() const { return acc_cfg->energy_ref.l1_output_egress; }
float accelerator_t::E_l2_i_igrs() const { return acc_cfg->energy_ref.l2_input_ingress; }
float accelerator_t::E_l2_i_egrs() const { return acc_cfg->energy_ref.l2_input_egress; }
float accelerator_t::E_l2_f_igrs() const { return acc_cfg->energy_ref.l2_filter_ingress; }
float accelerator_t::E_l2_f_egrs() const { return acc_cfg->energy_ref.l2_filter_egress; }
float accelerator_t::E_l2_o_igrs() const { return acc_cfg->energy_ref.l2_output_ingress; }
float accelerator_t::E_l2_o_egrs() const { return acc_cfg->energy_ref.l2_output_egress; }
float accelerator_t::E_dram_igrs() const { return acc_cfg->energy_ref.dram_ingress; }
float accelerator_t::E_dram_egrs() const { return acc_cfg->energy_ref.dram_egress; }

// Cycle references
float accelerator_t::C_mac_op() const { return acc_cfg->cycle_ref.mac_operation; }
float accelerator_t::C_l1_access() const { return acc_cfg->cycle_ref.l1_access; }
float accelerator_t::C_l2_access() const { return acc_cfg->cycle_ref.l2_access; }
float accelerator_t::C_dram_access() const { return acc_cfg->cycle_ref.dram_access; }
