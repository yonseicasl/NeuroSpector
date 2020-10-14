#ifndef __STATS_H__
#define __STATS_H__

#include <iomanip>
#include <string>
#include <vector>

#include "mapping_table.h"
#include "utils.h"

#define KB_UNIT 8192
#define MB_UNIT 8388608

enum class data_type_t {
    INPUT, WEIGHT, OUTPUT, SIZE
};

enum class stationary_t {
    IS, WS, OS, SIZE
};

enum class bypass_t {
// BYPASS MASK: 0 ~ 7
//   0,   1,   2,   3,   4,   5,   6,   7 
    XXX, XXO, XWX, XWO, IXX, IXO, IWX, IWO
};

/* Accelerator stats */
class accelerator_t {
public:
    accelerator_t();
    ~accelerator_t();
    void init();
    void print_stats();

    std::string name;
    // MAC
    unsigned mac_per_pe;
    unsigned mac_width;
    std::string mac_stationary;
    // L1
    unsigned input_L1_sizes;
    unsigned weight_L1_sizes;
    unsigned output_L1_sizes;
    bypass_t L1_bypass;
    std::string L1_stationary;
    // X, Y
    unsigned array_size_x;
    unsigned array_size_y;
    parameter_t array_unroll_x;
    parameter_t array_unroll_y;
    // L2
    unsigned L2_size;
    bypass_t L2_bypass;
    std::string L2_stationary;
    // PRECISION
    unsigned input_precision;
    unsigned weight_precision;
    unsigned output_precision;
};

/* Tiles stats */
class tiles_t {
public:
    tiles_t(mapping_table_t *mapping_table_, accelerator_t *accelerator_);
    ~tiles_t();
    void init();
    void print_stats();
    size_t calculate_tile_size(data_type_t type_, component_t U);

    struct tile_t {
        tile_t(std::string name_) : name(name_) {}
        std::string name;
        size_t num_input;
        size_t num_weight;
        size_t num_output;
    };

    std::vector<tile_t> tiles;

private:
    mapping_table_t *mapping_table;
    accelerator_t *accelerator;
};

/* Accesses stats */
class accesses_t {
public:
    accesses_t(mapping_table_t *mapping_table_, accelerator_t *accelerator_, tiles_t *tiles_);
    ~accesses_t();
    void init();
    void print_stats();
    size_t calculate_access_counts(data_type_t type_, component_t U, bool is_optimized);

    struct access_t {
        access_t(std::string name_) : name(name_) {}
        std::string name;
        std::vector<std::pair<size_t, double>> input_cnts_mb;
        std::vector<std::pair<size_t, double>> weight_cnts_mb;
        std::vector<std::pair<size_t, double>> output_cnts_mb;
        std::vector<double> mb_totals;
        void counts_to_mb(accelerator_t *acc_) {
            for(size_t i = 0; i < input_cnts_mb.size(); i++) {
                input_cnts_mb.at(i).second = double(input_cnts_mb.at(i).first * acc_->input_precision) / MB_UNIT; 
                weight_cnts_mb.at(i).second = double(weight_cnts_mb.at(i).first * acc_->weight_precision) / MB_UNIT; 
                output_cnts_mb.at(i).second = double(output_cnts_mb.at(i).first * acc_->output_precision) / MB_UNIT; 
                mb_totals.push_back(input_cnts_mb.at(i).second + weight_cnts_mb.at(i).second + output_cnts_mb.at(i).second);
            }
        }
    };

private:
    mapping_table_t *mapping_table;
    accelerator_t *accelerator;
    tiles_t *tiles;
    std::vector<access_t> accesses;
};

#endif
