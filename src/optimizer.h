#ifndef __OPTIMIZER_H__
#define __OPTIMIZER_H__

#include <iomanip>
#include <vector>

#include "configs.h"
#include "mapping_table.h"
#include "stats.h"

class optimizer_t {
public:
    optimizer_t(analyzer_configs_t &analyzer_configs_);
    ~optimizer_t();
    void init_acc();
    void init_layer_and_map();
    void print_stats(unsigned idx_);
    void print_accelerator(); 
    void print_mapping_tables(unsigned idx_);

    struct data_size_t {
        size_t input_size;
        size_t weight_size;
        size_t output_size;
    };

private:
    analyzer_configs_t &analyzer_configs;
    // Parsed from optimizer configs
    accelerator_t *accelerator;
    std::vector<std::pair<std::string, mapping_table_t*>> mapping_tables;
    // Optimizer stats
    std::vector<data_size_t*> data_sizes;
};

#endif
