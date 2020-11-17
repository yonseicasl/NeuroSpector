#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/* Configuration */
class configs_t {
public:
    configs_t(std::string config_file_);
    virtual ~configs_t() {}
    virtual void parse() = 0;
    template<typename T>
    T get_line_val(std::string &line_);
    void get_line_vals(std::string &line_, size_t bias_, std::vector<unsigned> &container);

protected:
    std::ifstream config_file;
};

/* Accelerator configuration */
class accelerator_configs_t : public configs_t {
public:
    accelerator_configs_t(std::string config_file_);
    ~accelerator_configs_t();
    void parse();

    std::string name;
    // PRECISION
    std::vector<unsigned> precision;
    // MAC
    unsigned mac_per_pe;
    unsigned mac_width;
    std::string L0_dataflow;
    // L1
    unsigned L1_exists;
    unsigned L1_mode;
    unsigned L1_shared_size;
    std::vector<unsigned> L1_separate_size;
    std::string L1_dataflow;
    // X, Y
    unsigned array_mode;
    unsigned array_size_x;
    unsigned array_size_y;
    std::string array_map_x;
    std::string array_map_y;
    // L2
    unsigned L2_exists;
    unsigned L2_shared_size;
    std::string L2_dataflow;
};

/* Network configuration */
class network_configs_t : public configs_t {
public:
    network_configs_t(std::string config_file_);
    ~network_configs_t();
    void parse();
    
    struct layer_t {
        std::string name;
        std::vector<unsigned> layer_vals;   // KBPQCRS and stride
    };
    std::string network_name;
    std::vector<layer_t> layers;
};

/* Mapping configuration */
class mapping_configs_t : public configs_t {
public:
    mapping_configs_t(std::string config_file_);
    ~mapping_configs_t();
    void parse();
    
    struct mapping_t {
        std::string name;
        std::vector<unsigned> layer_vals;   // KBPQCRS and stride
        std::vector<unsigned> map_vals;     // [(D,U)s and exists] x 5
    };
    std::string network_name;
    std::vector<mapping_t> mappings;
};
#endif
