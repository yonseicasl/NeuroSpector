#ifndef __ENUMS_H__
#define __ENUMS_H__

#include <algorithm>
#include <vector>
#include <string>

enum class run_t : unsigned {
    OPTIMIZER, ANALYZER, SIZE
};
// Components of an accelerator
enum class component_t : unsigned {
    REG, MAC_X, MAC_Y, LB, PE_X, PE_Y, GB, CHIP_X, CHIP_Y, DRAM, SIZE 
};
// Reuse types for separating the kinds of data-reuse
enum class reuse_t : unsigned {
    TEMPORAL, SPATIAL, SIZE
};
// Eight parameters that comprise DNN data (columns of scheduling table)
enum class parameter_t : unsigned {
    K, B, P, Q, C, R, S, G, SIZE
};
// Three types of DNN data
enum class data_t : unsigned {
    INPUT, WEIGHT, OUTPUT, SIZE
};
// Four correlation types of DNN parameters
enum class correlation_t : unsigned {
    WO, OI, IW, IWO, SIZE
};
// Three types of dataflow
enum class dataflow_t : unsigned {
    NONE, IS, WS, OS, SIZE
};
// Data read and write operations
enum class operation_t :unsigned {
    READ, WRITE, SIZE
};
// Where to send (or receive) tile data
enum class direction_t:unsigned {
    UPPER, LOWER, SIZE
};
enum class metric_t:unsigned {
    ENERGY, CYCLE, SIZE
};
enum class strategy_t:unsigned {
    PM, SP, SIZE
};
// Two dimension of spatial level
enum class dimension_t : unsigned {
    DIM_X, DIM_Y, SIZE
};

static std::vector<std::string> run_str __attribute__((unused)) = {
    "optimizer",
    "analyzer",
    "size",
};
static std::vector<std::string> component_type_str __attribute__((unused)) = {
    "temporal",
    "spatial",
    "size",
};
static std::vector<std::string> dataflow_str __attribute__((unused)) = {
    "none",
    "is",
    "ws",
    "os",
    "size",
};
static std::vector<std::string> metric_str __attribute__((unused)) = {
    "energy",
    "cycle",
    "size",
};
static std::vector<std::string> parameter_str __attribute__((unused)) = {
    "k",
    "b",
    "p",
    "q",
    "c",
    "r",
    "s",
    "g",
};

#define get_enum_type(m_vector, m_string) \
        distance(m_vector.begin(), find(m_vector.begin(), m_vector.end(), m_string.c_str()))

#define convert_to_string(x) #x

#endif
