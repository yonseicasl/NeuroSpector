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

    float pe_utilization;
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

/* Energy stats */
class energy_stats_t {
public:
    energy_stats_t();
    ~energy_stats_t();
    void init();
    void print_stats();
    void total() { total_energy = MAC_total_energy + L1_total_energy + L2_total_energy + DRAM_total_energy; }

    float MAC_operand_avg = 1;
    float L1_read_write_input = 1.38;
    float L1_read_write_weight = 2.58;
    float L1_read_write_output = 1.13;
    float L2_read_write_avg = 16.10;
    float DRAM_read_write_avg = 200;

    double MAC_total_energy;
    // Affected by MAC dataflow & non-requesting PEs
    double L1_total_energy;
    // Affected by L2 dataflow & requesting PEs
    double L2_total_energy;
    // Affected by L2 dataflow
    double DRAM_total_energy;
    double total_energy;
};

/* Cycle stats */
class cycle_stats_t {
public:
    cycle_stats_t();
    ~cycle_stats_t();
    void init();
    void print_stats();
    void total() { total_cycle = MAC_total_cycle + L1_total_cycle + noc_total_cycle + L2_total_cycle + DRAM_total_cycle; }

    float MAC_latency = 1;
    float L1_latency = 2;
    float noc_latency = 2;
    float L2_latency = 15;
    float DRAM_latency = 150;

    double MAC_total_cycle;
    double L1_total_cycle;
    double noc_total_cycle;
    double L2_total_cycle;
    double DRAM_total_cycle;
    double total_cycle;
};

#endif
