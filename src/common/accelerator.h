#ifndef __ACCELERATOR_H__
#define __ACCELERATOR_H__

#include <iomanip>
#include <iostream>
#include <string>

#include "configs.h"
#include "enums.h"
#include "utils.h"

/* Accelerator */
class accelerator_t {
public:
    accelerator_t(const std::string &cfg_path_);
    ~accelerator_t();
    // Accelerator APIs
    void print_stats() const;
    std::string get_name() const;
    // MAC [T] 
    precision_t precision() const;
    dataflow_t mac_dataflow() const;
    // S0 [S]
    unsigned macs_per_pe() const;
    unsigned mac_width() const;
    // L1 [T]
    bool l1_input_bypass() const;
    bool l1_filter_bypass() const;
    bool l1_output_bypass() const;
    unsigned l1_input_size() const;
    unsigned l1_filter_size() const;
    unsigned l1_output_size() const;
    unsigned l1_shared_size() const;
    buffer_type_t l1_type() const;
    dataflow_t l1_dataflow() const;
    // S1_X & S1_Y [S]
    bool s1_noc_exists() const;
    unsigned s1_size_x() const;
    unsigned s1_size_y() const;
    std::string s1_constraints_x() const;
    std::string s1_constraints_y() const;
    // L2 [T]
    bool l2_input_bypass() const;
    bool l2_filter_bypass() const;
    bool l2_output_bypass() const;
    unsigned l2_input_size() const;
    unsigned l2_filter_size() const;
    unsigned l2_output_size() const;
    unsigned l2_shared_size() const;
    buffer_type_t l2_type() const;
    dataflow_t l2_dataflow() const;
    // S2 [S]
    unsigned s2_size() const;
    // Energy references
    float E_mac_op() const;
    float E_l1_i_igrs() const;
    float E_l1_i_egrs() const;
    float E_l1_f_igrs() const;
    float E_l1_f_egrs() const;
    float E_l1_o_igrs() const;
    float E_l1_o_egrs() const;
    float E_l2_i_igrs() const;
    float E_l2_i_egrs() const;
    float E_l2_f_igrs() const;
    float E_l2_f_egrs() const;
    float E_l2_o_igrs() const;
    float E_l2_o_egrs() const;
    float E_dram_igrs() const;
    float E_dram_egrs() const;
    // Cycle references
    float C_mac_op() const;
    float C_l1_access() const;
    float C_l2_access() const;
    float C_dram_access() const;
    // Leverage
    unsigned seq1_max(dataflow_t df_) const;
    unsigned seq2_max(dataflow_t df_) const;
    
private:
    acc_cfg_t *acc_cfg;
    unsigned seq1_input_max;
    unsigned seq1_filter_max;
    unsigned seq1_output_max;
    unsigned seq2_input_max;
    unsigned seq2_filter_max;
    unsigned seq2_output_max;
};

#endif
