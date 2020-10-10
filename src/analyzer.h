#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <vector>

#include "configs.h"
#include "mapping_table.h"
#include "stats.h"

class analyzer_t {
public:
    analyzer_t(analyzer_configs_t &analyzer_configs_);
    ~analyzer_t();
    void init_acc();
    void init_layer_and_map();
    void analyze_tiles();
    void analyze_accesses();
    void print_stats();
    void print_accelerator(); 
    void print_mapping_tables(unsigned idx_);
    void print_tiles(unsigned idx_);
    void print_accesses(unsigned idx_);

private:
    analyzer_configs_t &analyzer_configs;
    // Parsed from analyzer configs
    accelerator_t *accelerator;
    std::vector<std::pair<std::string, mapping_table_t*>> mapping_tables;
    // Analyzed stats
    std::vector<tiles_t*> tiles;
};

#endif
