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
#include "utils.h"

class optimizer_t {
    public:
        optimizer_t(const std::string& accelerator_pth_,
                    const std::string& dataflow_,
                    const std::string& network_pth_,
                    const std::string& layer_,
                    const std::string& batch_size_);
        virtual ~optimizer_t();
        // Run optimizer for all layers
        virtual void run() = 0;                         
        // Run optimizer for target layer
        virtual void run(const unsigned idx_) = 0;      
        // Print results on termianl
        virtual void print_results() = 0;               
        // Count total number of component level to fill out
        unsigned get_num_targeted_levels(unsigned begin_pos_, unsigned end_pos_);
        unsigned get_num_spatial_levels(unsigned begin_pos_, unsigned end_pos_);
        unsigned get_num_temporal_levels(unsigned begin_pos_, unsigned end_pos_);

        unsigned is_dulicated(scheduling_table_t* scheduling_table_);
        void     record_layer_configuration(scheduling_table_t* scheduling_table_);
        unsigned search_history(std::vector<unsigned> layer_params_);

    protected:
        // Generate all possible dataflow combination for the target accelerator
        std::vector<std::vector<dataflow_t>> generate_dataflow_combinations();

        bool     is_fixed = true;                       // Fix accelerator's dataflow
        unsigned layer_idx = 0;                         // Layer index
        accelerator_t      *accelerator;                // Target accelerator
        network_t          *network;                    // Target network
        scheduling_table_t *scheduling_table;           // Scheduling table

        std::string batch_size;
        // list of optimal scheduling tables for all layers of target network
        std::vector<scheduling_table_t> list_of_scheduling_table; 
        // possible datalfow combination for the target accelerator
        std::vector<std::vector<dataflow_t>> dataflow_combinations;

        std::map<unsigned, std::vector<unsigned>> optimization_history;
};

#endif
