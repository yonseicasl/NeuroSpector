#include <iomanip>

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
          parameters.at((unsigned)parameter_t::K)/=parameters.at((unsigned)parameter_t::G);
          parameters.at((unsigned)parameter_t::C)/=parameters.at((unsigned)parameter_t::G);
}
layer_t::~layer_t() {}
// Get layer name
std::string layer_t::get_name() { return name; }
// Get layer parameters
std::vector<unsigned> layer_t::get_parameters () { return parameters; }
// Update specific layer parameter
void layer_t::update_parameter(parameter_t param_, unsigned num) {
    parameters.at((unsigned)param_) = num;
}
// Print stats
void layer_t::print_stats() {
     std::cout << "|";
     std::cout.width(14);
     std::cout << std::left << name; 
     std::cout << "|";
     for(unsigned col = 0; col < parameters.size(); col++) {
          std::cout.width(6);
          std::cout << std::right 
                    << parameters.at(col) 
                    << " |";
     }
     std::cout << std::setw(5) << stride << " |" << std::endl; 
     std::cout << "|------------------------------------------------------------------------------|------|" << std::endl;

     return;
}
void layer_t::print_stats(std::ofstream &output_file_) {
     output_file_ << "|";
     output_file_.width(14);
     output_file_ << std::left << name; 
     output_file_ << "|";
     for(unsigned col = 0; col < parameters.size(); col++) {
          output_file_.width(6);
          output_file_ << std::right 
                    << parameters.at(col) 
                    << " |";
     }
     output_file_ << std::setw(5) << stride << " |\n"; 
     output_file_ << "|------------------------------------------------------------------------------|------|" << std::endl;

     return;
}
// Get stride
unsigned layer_t::get_stride() { return stride; }
