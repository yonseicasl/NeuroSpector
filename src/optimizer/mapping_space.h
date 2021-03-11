#ifndef __MAPPING_SPACE_H__
#define __MAPPING_SPACE_H__

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

/* Mapping space */
class mapping_space_t {
public:
    mapping_space_t(const unsigned num_levels_, 
                    const std::vector<unsigned> &layer_values_);
    ~mapping_space_t();

    void print_permutations() const;
    uint64_t get_num_permutations() const;
    std::vector<std::vector<unsigned>> get_permutations(const unsigned idx_) const;
    std::vector<std::vector<std::vector<unsigned>>> get_layer_permutations() const;

private:
    std::vector<unsigned> get_factors(const unsigned val_);
    void get_permutations(const unsigned idx_, const unsigned val_, std::vector<unsigned> &permutation_);
    // Variables & containers
    unsigned num_levels;
    uint64_t num_permutations;
    std::vector<std::vector<std::vector<unsigned>>> layer_permutations;
};

/* Mapping space range per thread */
class range_t {
public:
    range_t(const unsigned tid_, 
            const unsigned num_threads_,
            const std::vector<std::vector<std::vector<unsigned>>>& layer_permutations_,
            std::mutex& m_);
            
    ~range_t();
    
    size_t start_k;
    size_t end_k;
    size_t start_b;
    size_t end_b;
    size_t start_p;
    size_t end_p;
    size_t start_q;
    size_t end_q;
    size_t start_c;
    size_t end_c;
    size_t start_s;
    size_t end_s;
    size_t start_r;
    size_t end_r;

private:
};

#endif
