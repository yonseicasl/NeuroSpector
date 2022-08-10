#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <iostream>
#include <string>
#include <cassert>

#include "layer.h"

class network_t {
public:
    network_t(const std::string &cfg_path_);
    ~network_t();
    void init_network();
    void get_layers(section_config_t section_config_);
    std::string           get_layer_name(unsigned idx_);        // Get target layer's name
    std::vector<unsigned> get_layer_parameters(unsigned idx_);  // Get target layer's parameters
    unsigned              get_layer_index(std::string name_);   // Get index of target layer
    unsigned              get_num_layers();                     // Get total # layers in network
    unsigned              get_stride(unsigned idx_);            // Get target layer's stride
    void print_stats();
private:
    std::string net_cfg_path;                                   // Path to Network config.
    std::string name;                                           // Network name
    std::vector<layer_t> layers;                                // List of pairs (layer name, values)
};

#endif
