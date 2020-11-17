#ifndef __OPTIMIZER_H__
#define __OPTIMIZER_H__

#include <iomanip>
#include <vector>

#include "configs.h"
#include "mapping_table.h"
#include "stats.h"

class optimizer_t {
public:
    optimizer_t(accelerator_configs_t &accelerator_configs_, network_configs_t &network_configs_);
    ~optimizer_t();
    void init();
    void print_stats(unsigned idx_);
    void print_accelerator(); 
    void print_mapping_tables(unsigned idx_);
    void optimize();

private:
    std::string network_name;
    accelerator_configs_t &accelerator_configs;
    network_configs_t &network_configs;
    accelerator_t *accelerator;
    std::vector<std::pair<std::string, mapping_table_t*>> mapping_tables;
};

#endif
