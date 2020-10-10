#ifndef __STATS_H__
#define __STATS_H__

#include <iomanip>
#include <string>
#include <vector>

#include "mapping_table.h"
#include "utils.h"

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
    tiles_t(mapping_table_t *mapping_table_, accelerator_t * accelerator_);
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

private:
    mapping_table_t *mapping_table;
    accelerator_t *accelerator;
    std::vector<tile_t> tiles;
};

/* Accesses stats */
class accesses_t {
public:
    accesses_t(mapping_table_t *mapping_table_, accelerator_t * accelerator_);
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
    };

private:
    mapping_table_t *mapping_table;
    accelerator_t *accelerator;
    std::vector<access_t> accesses;
};

#endif
