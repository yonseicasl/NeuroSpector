#include "layer.h"

layer_t::layer_t(const std::string name_, 
                 const std::vector<unsigned> parameters_,
                 const unsigned stride_)
    :name(name_),
     parameters(parameters_),
     stride(stride_) { 
          // Check layer validity
          if(parameters.at((unsigned)parameter_t::C) 
          % parameters.at((unsigned)parameter_t::G) != 0 ||
          parameters.at((unsigned)parameter_t::K) 
          % parameters.at((unsigned)parameter_t::G) != 0) {
               std::cerr << "[Error] in Layer `" << name
                         << "`, the remainder of input channel (C) (or weight count (K)) / group (G) should be `0`; "
                         << "C=" << parameters.at((unsigned)parameter_t::C) << ", "
                         << "K=" << parameters.at((unsigned)parameter_t::K) << ", "
                         << "G=" << parameters.at((unsigned)parameter_t::G) << " "
                         << std::endl;
               exit(0);
          }
}
layer_t::~layer_t() {}
// Get layer name
std::string layer_t::get_name() { return name; }
// Get layer parameters
std::vector<unsigned> layer_t::get_parameters () { return parameters; }
// Get stride
unsigned layer_t::get_stride() { return stride; }
