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
    unsigned array_size_x;
    unsigned array_size_y;
    unsigned input_precision;
    unsigned weight_precision;
    unsigned output_precision;
    bypass_t bypass_L1;
    bypass_t bypass_L2;
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
    size_t calculate_access_counts(data_type_t type_, component_t U);

    struct access_t {
        access_t(std::string name_) : name(name_) {}
        std::string name;
        size_t cnts_input;
        size_t cnts_weight;
        size_t cnts_output;
        double mb_input;
        double mb_weight;
        double mb_output;
        double mb_total;
        void counts_to_mb(accelerator_t *acc_) {
            mb_input = double(cnts_input * acc_->input_precision) / MB_UNIT; 
            mb_weight = double(cnts_weight) * acc_->weight_precision / MB_UNIT;
            mb_output = double(cnts_output) * acc_->output_precision / MB_UNIT;
            mb_total = mb_input + mb_weight + mb_output;
        }
    };

private:
    mapping_table_t *mapping_table;
    accelerator_t *accelerator;
    tiles_t *tiles;
    std::vector<access_t> accesses;
};

#endif
