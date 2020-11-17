#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <iomanip>
#include <vector>

#include "configs.h"
#include "mapping_table.h"
#include "stats.h"

class analyzer_t {
public:
    analyzer_t(mapping_configs_t &mapping_configs_);
    ~analyzer_t();
    void init();
    void print_stats(unsigned idx_);
    void print_mapping_tables(unsigned idx_);

private:
    std::string network_name;
    mapping_configs_t &mapping_configs;
    std::vector<std::pair<std::string, mapping_table_t*>> mapping_tables;
};

#endif
