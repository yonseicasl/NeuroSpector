#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <vector>

#include "configs.h"
#include "mapping_table.h"

class analyzer_t {
public:
    analyzer_t(analyzer_configs_t &analyzer_configs_);
    ~analyzer_t();
    void init_acc();
    void init_layer_and_map();
    void print_mapping_tables();

    struct acc_t {
        std::string name;
        unsigned array_size_x;
        unsigned array_size_y;
        unsigned input_precision;
        unsigned weight_precision;
        unsigned output_precision;
        // Bypass range: 0 ~ 7
        unsigned bypass_L1;
        unsigned bypass_L2;
    };

private:
    analyzer_configs_t &analyzer_configs;
    acc_t accelerator;
    std::vector<std::pair<std::string, mapping_table_t*>> mapping_tables;
};

#endif
