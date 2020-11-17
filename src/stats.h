#ifndef __STATS_H__
#define __STATS_H__

#include <iomanip>
#include <string>
#include <vector>

#include "configs.h"
#include "utils.h"

enum class dataflow_t {
    IS, WS, OS, NO, SIZE
};

/* Accelerator */
class accelerator_t {
public:
    accelerator_t(accelerator_configs_t &accelerator_configs_);
    ~accelerator_t();
    void init();
    void print_stats();

    std::string name;
    // PRECISION
    unsigned input_precision;
    unsigned weight_precision;
    unsigned output_precision;
    // MAC
    unsigned mac_per_pe;
    unsigned mac_width;
    dataflow_t L0_dataflow;
    // L1
    bool L1_mode; // Shared: 0 / Separate: 1
    unsigned L1_total_size;
    unsigned L1_input_size;
    unsigned L1_weight_size;
    unsigned L1_output_size;
    dataflow_t L1_dataflow;
    // X, Y
    bool array_mode; // flexible: 0 / fixed: 1
    unsigned array_size_x;
    unsigned array_size_y;
    unsigned array_map_x;
    unsigned array_map_y;
    // L2
    unsigned L2_shared_size;
    dataflow_t L2_dataflow;

private:
    accelerator_configs_t &accelerator_configs;
};

/* Tile sizes */
class tile_size_t {
public:
    tile_size_t();
    ~tile_size_t();
    void init();
    void print_stats();

    struct info_t {
        size_t input_tile;
        size_t weight_tile;
        size_t output_tile;
    };

    info_t MAC;
    info_t L1;
    info_t L2;
    info_t DRAM;
};

/* NoC information */
class noc_info_t {
public:
    noc_info_t();
    ~noc_info_t();
    void init();
    void print_stats();

    struct info_t {
        unsigned input_pes;
        unsigned weight_pes;
        unsigned output_pes;
    };

    unsigned total_active_pes;
    info_t requesting;
};

/* Access counts */
class access_cnts_t {
public:
    access_cnts_t();
    ~access_cnts_t();
    void init();
    void print_stats();

    struct info_t {
        info_t() : input_cnts(0), weight_cnts(0), output_cnts(0) {}
        size_t input_cnts;
        size_t weight_cnts;
        size_t output_cnts;
    };

    info_t is;
    info_t ws;
    info_t os;
};

#endif
