#ifndef __LAYER_H__
#define __LAYER_H__

#include <iostream>
#include <string>
#include <cassert>

#include "enums.h"
#include "parser.h"

class layer_t {
public:
    layer_t(const std::string name_, 
            const std::vector<unsigned> parameters_,
            const unsigned stride_);
    ~layer_t();
    std::string           get_name();             // Get layer name
    std::vector<unsigned> get_parameters();       // Get layer parameters
    unsigned              get_stride();           // Get stride
    void print_stats();
    void print_stats(std::ofstream &output_file_);
private:
    std::string name;                             // Layer name
    std::vector<unsigned> parameters;             // Layer parameters
    unsigned stride;                              // Stride
};

#endif
