#ifndef __ACCELERATOR_H__
#define __ACCELERATOR_H__

#include "enums.h"
#include "parser.h"

class accelerator_t {
public:
    struct temporal_component_t {
        reuse_t   type = reuse_t::TEMPORAL;
        std::string        name;
        std::vector<float> size;
        dataflow_t         dataflow = dataflow_t::NONE;
        unsigned           num_components = 1;
        
        std::vector<float> bitwidth;
        unsigned bitwidth_i;
        unsigned bitwidth_w;
        unsigned bitwidth_o;

        std::vector<float> unit_energy;
        std::vector<float> unit_static;
        std::vector<float> unit_cycle;
        float unit_energy_per_dtype[(unsigned)data_t::SIZE] = { };
        float unit_cycle_per_dtype[(unsigned)data_t::SIZE] = { };
        bool  bypass[(unsigned)data_t::SIZE] = { false };
        bool  separated_data[(unsigned)data_t::SIZE] = { false };
        std::string        constraint;
    };

    struct spatial_component_t {
        reuse_t   type = reuse_t::SPATIAL;
        std::string name;
        unsigned    dim_x = 1;
        unsigned    dim_y = 1;
        std::string constraint_x;
        std::string constraint_y;
        
        unsigned    radix_degree = 1;   // NoC radix
        unsigned    radix_degree_i; // Radix for input transfer 
        unsigned    radix_degree_w; // Radix for weight transfer
        unsigned    radix_degree_o; // Radix for output transfer
    };
    // Construct components in accelerator 
    accelerator_t(const std::string &cfg_path_); 
    accelerator_t(const accelerator_t &accelerator_); // Copy constructor
    ~accelerator_t();

    // Generate components which make up the accelerator
    void generate_components(const parser_t parser); 

    // Check if the component exists
    bool check_component_exist(component_t comp_);
    // Get accelerator's specifications
    std::string         get_acc_name();
    float               get_clock_time();
    float               get_clock_frequency();
    float               get_theoretical_peak_power();
    float               get_dram_power();
    float               get_mac_energy();
    float               get_mac_static_power();
    float               get_mul_energy();
    float               get_add_energy();
    unsigned            get_precision();

    // Get dimension size of spatially arranged components 
    unsigned            get_total_num_MACs();
    unsigned            get_total_num_PEs();
    unsigned            get_total_num_chips();
    
    unsigned            get_array_size(component_t comp_, dimension_t dim_); 
    float               get_radix_degree(component_t comp_, data_t dtype_);
    
    // Get variables in temporal component structure
    std::string         get_name(component_t comp_);
    reuse_t             get_type(component_t comp_) const;
    bool*               get_bypass(component_t comp_); 
    bool*               get_separated_data(component_t comp_); 
    std::vector<float>  get_size(component_t comp_);
    dataflow_t          get_dataflow(component_t comp_);
    float*              get_bitwidth(component_t comp_);         
    float               get_bitwidth(component_t comp_, data_t dtype_);
    unsigned            get_num_components(component_t comp_);
    // Returns a list of unit energy, static power, and cycle of a component level
    float*              get_energy(component_t comp_) const;
    float*              get_static(component_t comp_) const;
    float*              get_cycle(component_t comp_) const;
    // Returns unit cost to transfer a specific type of data in a component level
    float               get_data_access_energy(component_t comp_, data_t dtype_) const;
    float               get_data_access_cycle(component_t comp_, data_t dtype_) const;
    // Returns unit static power of a specific component.
    float               get_comp_level_static(component_t comp_) const;


    // Check the component sizes is 1
    bool                is_unit_component(component_t comp_);

    // Get user specified constraints
    std::vector<unsigned> get_memory_constraint(component_t comp_); 
    std::string           get_array_constraint(component_t comp_, 
                                               dimension_t dim_); 
    void                update_array_constraint(component_t comp_, 
                                                std::string cnst_x_,
                                                std::string cnst_y_);
    
    // Print specifiations of accelerator
    void print_spec();
    void print_temporal_component(temporal_component_t* component_); 
    void print_spatial_component(spatial_component_t* component_); 

    // Print specifiations of accelerator to output file
    void print_spec(std::ofstream &output_file_);
    void print_temporal_component(std::ofstream &output_file_,
                                  temporal_component_t* component_); 
    void print_spatial_component(std::ofstream &output_file_,
                                 spatial_component_t* component_); 
    
    // Calculate Accelerator's power consumption
    void  calc_accelerator_max_power();
    void  calc_dram_power();
    float sum_component_static_power();
    float sum_component_energy();
private:
    // Configure temporal components
    std::vector<float>  set_size(std::string value_)    const;
    std::vector<float>  set_size_kB(std::string value_) const;
    std::vector<float>  set_size_MB(std::string value_) const;
    
    void set_cost(std::vector<float> &cost_, std::string value_);
    void set_bypass(temporal_component_t* comp_, std::string value_);
    void set_bitwidth(std::vector<float> &bw_, std::string value_); 
    void set_constraint(temporal_component_t* comp_, std::string value_);
    void set_separated_data(temporal_component_t* comp_, std::string value_);

    void set_bitwidth_per_dtype(temporal_component_t* comp_); 
    void set_energy_per_dtype(temporal_component_t* comp_);
    void set_cycle_per_dtype(temporal_component_t* comp_);
    // Configure spatial components
    void set_radix_degree(spatial_component_t* comp_);
    // Initialize components
    void init_temporal_component(temporal_component_t* component_,
                                 section_config_t section_config_); 
    void init_spatial_component(spatial_component_t* component_,
                                section_config_t section_config_); 

    std::string               acc_cfg_path;
    std::string               name;
    unsigned                  precision;
    unsigned                  clock_frequency      = 0;
    float                     theoretical_peak_power = 0.0;
    float                     dram_power           = 0.0;
    float                     mac_static_power     = 0.0;
    float                     mac_operation_energy = 0.0;
    float                     mul_operation_energy = 0.0;
    float                     add_operation_energy = 0.0;
    bool                      is_flexible = false;

    float                     backup_energy = 0.0;
    float                     backup_power  = 0.0;

    // Evaluation Metric for optimization
    std::vector<float>        energy;
    std::vector<float>        power;
    std::vector<float>        cycle;

    void *component_list[(unsigned)component_t::SIZE] = { nullptr };
    temporal_component_t *register_file;
    spatial_component_t  *mac_array;
    temporal_component_t *local_buffer;
    spatial_component_t  *pe_array;
    temporal_component_t *global_buffer;
    spatial_component_t  *multi_chips;
    temporal_component_t *dram;
};

#endif
