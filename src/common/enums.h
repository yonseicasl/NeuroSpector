#ifndef __ENUMS_H__
#define __ENUMS_H__

/* Acclerator */
// Supported precision types
enum class precision_t {
    FP8, FP16, FP32, INT8, INT16, INT32, SIZE
};

// Dataflow types of temporal components
enum class dataflow_t {
    IS,     // Input-stationary 
    WS,     // Weight-stationary 
    OS,     // Output-stationary 
    NONE,   // Only for MAC 
    SIZE
};

// Buffer (L1 & L2) types
enum class buffer_type_t {
    NONE,
    SEPARATED, 
    SHARED, 
    SHARED_IF, 
    SHARED_FO,
    SHARED_OI,
    SIZE
};

/* Mapping table */
// Layer parameters: D (mapping table columns)
enum class parameter_t {  
    K, B, P, Q, C, S, R, SIZE
};

// Components: U (mapping table rows) 
enum class component_t { 
    MAC, S0, L1, S1_X, S1_Y, L2, S2, DRAM, SIZE 
};

#endif
