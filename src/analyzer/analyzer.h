#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <iomanip>
#include <vector>

#include "accelerator.h"
#include "configs.h"
#include "enums.h"
#include "mapping_table.h"
#include "stats.h"

/* Analyzer */
class analyzer_t {
public:
    analyzer_t(const std::string& acc_cfg_path_,
               const std::string& map_cfg_path_);
    ~analyzer_t();
    // Analyzer APIs
    void print_stats() const;                       // Print stats of all layers
    void print_stats(const unsigned idx_) const;    // Print stats of the target layer

private:
    // Check each mapping table with the accelerator
    void check_validity(const unsigned idx_) const;      
    bool mac_validity(const unsigned idx_) const;      
    bool s0_validity(const unsigned idx_) const;      
    bool l1_validity(const unsigned idx_) const;      
    bool s1_x_validity(const unsigned idx_) const;      
    bool s1_y_validity(const unsigned idx_) const;      
    bool l2_validity(const unsigned idx_) const;      
    bool s2_validity(const unsigned idx_) const;      
    // Update stats
    void update_stats(unsigned idx_);        
    // Variables & containers
    unsigned D_size;                                // Mapping table column size
    //unsigned U_size;                                // Mapping table row size
    accelerator_t *accelerator;                     // Target accelerator   
    std::string network_name;                       // DNN name
    std::vector<bool> exists;                       // Component exist bits from MAC to DRAM
    std::vector<mapping_table_t*> mapping_tables;   // Mapping tables from the mapping configuration
    std::vector<stats_t*> all_stats;                // All stats of the mapping tables
};

#endif
