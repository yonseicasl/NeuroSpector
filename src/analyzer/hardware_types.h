#ifndef __ANALYZER_STRUCTURE_H__
#define __ANALYZER_STRUCTURE_H__

// Memory-related structures
struct tile_size_t {
    uint64_t input  = 1;
    uint64_t weight = 1;
    uint64_t output = 1;
};
struct access_count_t {
    uint64_t input_rd  = 1;
    uint64_t weight_rd = 1;
    uint64_t output_rd = 0;
    uint64_t output_wt = 1;
};
struct bitwidth_t {
    unsigned input  = 1;
    unsigned weight = 1;
    unsigned output = 1;
};
struct buffer_size_t {
    float input  = 1;
    float weight = 1;
    float output = 1;
    float shared = 1;
};
struct bypass_t {
    bool input  = false;
    bool weight = false;
    bool output = false;
};

// Spatial-related structures
struct arr_size_t {
    unsigned dim_x = 1;
    unsigned dim_y = 1;
};
struct arr_cnst_t {
    std::vector<std::string> dim_x;
    std::vector<std::string> dim_y;
    bool empty = true;
};
struct rdx_degree_t {
    float input  = 1;
    float weight = 1;
    float output = 1;
};

// Cost-related structures
struct unit_cost_t {
    float input  = 0;
    float weight = 0;
    float output = 0;
};
#endif
