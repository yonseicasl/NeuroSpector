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
    // S2_X & S2_Y [S]
    bool s2_noc_exists() const;
    unsigned s2_size_x() const;
    unsigned s2_size_y() const;
    
private:
    acc_cfg_t *acc_cfg;
};

#endif
