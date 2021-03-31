#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

#include "enums.h"
#include "utils.h"

/* Configuration */
class configs_t {
public:
    configs_t(const std::string& cfg_path_);
    virtual ~configs_t() {}
    virtual void parse() = 0;
    // Get an item in the line
    unsigned get_line_uint(const std::string &line_, const unsigned bias_);
    std::string get_line_string(const std::string &line_, const unsigned bias_);
    // Get values in the line
    void get_line_vals(const std::string &line_, const unsigned bias_, 
                       const unsigned num_, std::vector<unsigned>& dst_);
protected:
    std::ifstream config_file;
};

/* Accelerator configuration */
class acc_cfg_t : public configs_t {
public:
    acc_cfg_t(const std::string& cfg_path_);
    ~acc_cfg_t();
    void parse();
    std::string name;
    // MAC [T] 
    precision_t precision;
    dataflow_t mac_dataflow;
    // S0 [S]
    unsigned macs_per_pe;
    unsigned mac_width;
    // L1 [T]
    bool l1_input_bypass;
    bool l1_filter_bypass;
    bool l1_output_bypass;
    unsigned l1_input_size;
    unsigned l1_filter_size;
    unsigned l1_output_size;
    unsigned l1_shared_size;
    buffer_type_t l1_type;
    dataflow_t l1_dataflow;
    // S1_X & S1_Y [S]
    bool s1_noc_exists;
    unsigned s1_size_x;
    unsigned s1_size_y;
    // L2 [T]
    bool l2_input_bypass;
    bool l2_filter_bypass;
    bool l2_output_bypass;
    unsigned l2_input_size;
    unsigned l2_filter_size;
    unsigned l2_output_size;
    unsigned l2_shared_size;
    buffer_type_t l2_type;
    dataflow_t l2_dataflow;
    // S2 [S]
    unsigned s2_size;
};

/* Mapping table configuration (for analyzer) */
class map_cfg_t : public configs_t {
public:
    map_cfg_t(const std::string& cfg_path_);
    ~map_cfg_t();
    void parse();
    struct mapping_table_t {
        bool is_grouped;                    // For group conv
        unsigned stride;                    // Layer stride
        std::string name;                   // Layer name
        std::vector<unsigned> values;       // Layer parameter values (GKBPQCSR)
        std::vector<unsigned> degrees;      // Mapping degrees
    };
    std::string network_name;
    std::vector<mapping_table_t> mappings;
};

/* Network configuration (for optimizer) */
class net_cfg_t : public configs_t {
public:
    net_cfg_t(const std::string& cfg_path_);
    ~net_cfg_t();
    void parse();
    struct layer_t {
        bool is_grouped;                    // For group conv
        unsigned stride;                    // Layer stride
        std::string name;                   // Layer name
        std::vector<unsigned> values;       // Layer parameter values (GKBPQCSR)
    };
    std::string network_name;
    std::vector<layer_t> layers;
};

#endif
