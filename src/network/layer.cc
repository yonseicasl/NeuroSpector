#include "layer.h"

layer_t::layer_t(const std::string name_, 
                 const std::vector<unsigned> parameters_,
                 const unsigned stride_)
    :name(name_),
     parameters(parameters_),
     stride(stride_) { 
}
layer_t::~layer_t() {}
// Get layer name
std::string layer_t::get_name() { return name; }
// Get layer parameters
std::vector<unsigned> layer_t::get_parameters () { return parameters; }
// Get stride
unsigned layer_t::get_stride() { return stride; }
