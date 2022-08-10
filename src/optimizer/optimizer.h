#ifndef __OPTIMIZER_H__
#define __OPTIMIZER_H__

#include <cassert>
#include <string>
#include <iostream>

#include "accelerator.h"
#include "analyzer.h"
#include "mapping_space.h"
#include "network.h"
#include "scheduling_table.h"

class optimizer_t {
    public:
        optimizer_t(const std::string& accelerator_pth_,
                    const std::string& dataflow_,
                    const std::string& network_pth_,
                    const std::string& layer_);
        virtual ~optimizer_t();
        virtual void run() = 0;                         // Run optimizer for all layers
        virtual void run(const unsigned idx_) = 0;      // Run optimizer for target layer
        virtual void print_stats() = 0;                 // Print stats

    protected:
        std::vector<std::vector<dataflow_t>> generate_dataflow_combinations();

        bool     is_fixed = true;                       // Fix accelerator's dataflow
        unsigned layer_idx = 0;                         // Layer index
        accelerator_t      *accelerator;                // Target accelerator
        network_t          *network;                    // Target network
        scheduling_table_t *scheduling_table;           // Scheduling table
        std::vector<scheduling_table_t> list_of_scheduling_table;
        std::vector<std::vector<dataflow_t>> dataflow_combinations;
};

#endif
