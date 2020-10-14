#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <fstream>
#include <iostream>
//#include <sstream>
#include <string>
#include <vector>

#define MAP_TABLE_ROWS 5
#define MAP_TABLE_COLUMNS 7

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

/* Analyzer configuration */
class analyzer_configs_t : public configs_t {
public:
    analyzer_configs_t(std::string config_file_);
    ~analyzer_configs_t();
    void parse();
    struct acc_t {
        std::string name;
        // MAC
        unsigned mac_per_pe;
        unsigned mac_width;
        std::string mac_stationary;
        // L1
        std::vector<unsigned> L1_sizes;
        std::vector<unsigned> L1_bypass;
        std::string L1_stationary;
        // X, Y
        unsigned array_size_x;
        unsigned array_size_y;
        std::string array_unroll_x;
        std::string array_unroll_y;
        std::string array_stationary;
        // L2
        unsigned L2_size;
        std::vector<unsigned> L2_bypass;
        std::string L2_stationary;
        // PRECISION
        std::vector<unsigned> precision;
    };
    struct layer_and_map_t {
        std::string name;
        std::vector<unsigned> layer_vals;   // KBPQCRS and stride
        std::vector<unsigned> map_vals;     // [(D,U)s and exists] x MAP_TABLE_ROWS
    };
    acc_t accelerator;
    std::string network_name;
    std::vector<layer_and_map_t> layers_and_maps;

private:
    bool is_hw_parsed;
    bool is_net_parsed;
    bool is_map_parsed;
    unsigned layer_and_map_cnt;
};

/* Optimizer configuration */
class optimizer_configs_t : public configs_t {
public:
    optimizer_configs_t(std::string config_file_);
    ~optimizer_configs_t();
    void parse();

private:
};

#endif
