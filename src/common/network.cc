#include "network.h"

network_t::network_t(const std::string &cfg_path_) { 
    net_cfg_path = cfg_path_; 
}
network_t::~network_t() {};
void network_t::init_network() {
    // Parse the configuration file
    parser_t parser;
    parser.cfgparse(net_cfg_path);

    unsigned num_components = parser.sections.size() - 1;
    for(unsigned i = 0; i <= num_components; i++) {
        section_config_t section_config = parser.sections[i];
        if(section_config.name == "NETWORK") { 
            // Get network name
            section_config.get_setting("name", &name); 
        }
        else if(section_config.name == "LAYERS") {
            // Get layer configurations
            get_layers(section_config);
        }
    }
}
void network_t::get_layers(section_config_t section_config_) {
    std::vector<unsigned> parameters;
    std::string name;
    unsigned stride;
    
    std::vector<std::string> tmp_param;

    for(unsigned i = 0; i < section_config_.get_num_settings(); i++) {
        // Get name, parameters, and stride from layer configuration
        name = section_config_.get_value(i);
        // Separate config values by comma
        tmp_param = split(section_config_.get_value(name), ',');
        // Convert string to unsigned
        for(unsigned i = 0; i < tmp_param.size(); i++) {
            parameters.push_back(stoi(tmp_param.at(i)));
        }
        stride = parameters.back();
        // Remove stride value in parameters
        parameters.pop_back();
        // Put them in layer object
        layer_t layer = layer_t(name, parameters, stride);
        layers.push_back(layer);
        // Clear parameters
        tmp_param.clear();
        parameters.clear();
        
    }
    return;
}
void network_t::update_batch_size(const std::string &batch_size_) {
    unsigned batch_size = stoi(batch_size_);
    // Traverse all layer configurations
    for(unsigned i = 0;  i < layers.size(); i++) {
        // Update B parameter to the batch_size_
        layer_t& layer = layers.at(i);
        layer.update_parameter(parameter_t::B, batch_size);
    }
    return;
}
std::string network_t::get_network_name() {
    return name;
}
std::string network_t::get_layer_name(unsigned idx_) {
    layer_t layer = layers.at(idx_); 
    return layer.get_name();
}
std::vector<unsigned> network_t::get_layer_parameters(unsigned idx_) {
    layer_t layer = layers.at(idx_); 
    return layer.get_parameters();
}
unsigned  network_t::get_layer_index(std::string name_) {
    unsigned index = 0;
    for(unsigned i = 0; i < layers.size();) {
        if(layers.at(i).get_name() == name_) { break;}
        i++;
        index = i; 
    }
    if(index == layers.size()) { 
        std::cerr << "Error: invalid layer name in scheduling table `" 
                  << name_ 
                  << "`" << std::endl;
        exit(0);
    }
    return index;
}
unsigned network_t::get_num_layers() {
    return layers.size();
}
unsigned network_t::get_stride(unsigned idx_) {
    return layers.at(idx_).get_stride();
}
void network_t::print_stats() {
    std::vector<unsigned> parameters;
    std::cout << "Network Name :" << name << std::endl;
    for(unsigned i = 0;  i < layers.size(); i++) {
        layer_t layer = layers.at(i);
        std::cout << "Layer Name :" << layer.get_name() << "\t";
        std::cout << "Parameters :";
        parameters = layer.get_parameters();
        for(unsigned j = 0; j < parameters.size(); j++) {
            std::cout << parameters.at(j) << " ";
        }
        std::cout << "\tStride :" << layer.get_stride();
        std::cout << std::endl;
    }
    return;
}
void network_t::print_stats(unsigned layer_idx_) {
    layer_t layer = layers.at(layer_idx_);
    layer.print_stats();
    return;
}
void network_t::print_stats(unsigned layer_idx_, std::ofstream &output_file_) {
    layer_t layer = layers.at(layer_idx_);
    layer.print_stats(output_file_);
    return;
}