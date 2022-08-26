#include <cassert>

#include "mapping_space.h"

// Constructor
mapping_space_t::mapping_space_t() 
    : num_levels(0),
      num_permutations(0),
      permutation_index(1),
      parameter_index((unsigned) parameter_t::SIZE, 0) {
}
// Constructor
mapping_space_t::mapping_space_t(unsigned num_levels_,
                                 uint64_t num_permutations_,
                                 std::vector<std::vector<std::vector<unsigned>>> layer_permutations_)
    : num_levels(num_levels_),
      num_permutations(num_permutations_),
      layer_permutations(layer_permutations_),
      permutation_index(1),
      parameter_index((unsigned) parameter_t::SIZE, 0) {
}
// Copy constructor
mapping_space_t::mapping_space_t(mapping_space_t& mapping_space_)
    : num_levels(mapping_space_.num_levels),
      num_permutations(mapping_space_.num_permutations),
      layer_permutations(mapping_space_.layer_permutations),
      permutation_index (1),
      parameter_index(mapping_space_.parameter_index) {

}
mapping_space_t::~mapping_space_t() {

}


void mapping_space_t::clear() {
    num_levels = 0;
    num_permutations = 0;
    permutation_index= 0;
    // clear var:layer_permutations
    layer_permutations.clear();
    // clear var:permutation
}

void mapping_space_t::print_permutations() const {
    std::cout << "**** PERMUTATIONS ****" << std::endl;
    std::string str = "KBPQCSRG";
    for(unsigned i = 0; i < layer_permutations.size(); i++) {
        std::cout << str.at(i) << ": "<< layer_permutations.at(i).size() << " PERMUTATIONS\n"; 
        for(unsigned j = 0; j < layer_permutations.at(i).size(); j++) {
            std::cout << " | ";
            for(unsigned k = 0; k < layer_permutations.at(i).at(j).size(); k++) {
                std::cout << std::setw(3) << layer_permutations.at(i).at(j).at(k);
                if(k < layer_permutations.at(i).at(j).size() - 1)
                    std::cout << ", ";
                else
                    std::cout << " |\n";
            }
        }
        std::cout << std::endl;
    }
    return;
}

void mapping_space_t::generate(const unsigned num_levels_, 
                               const std::vector<unsigned> &layer_values_) {
    num_levels = num_levels_;
    num_permutations = 1;    
    // the number of layer parameters equals to 8 (KBPQCRSG)
    assert(layer_values_.size() == (unsigned)parameter_t::SIZE);
    for(unsigned i = 0; i < (unsigned)parameter_t::SIZE; i++) {
        std::vector<std::vector<unsigned>> permutations;
        layer_permutations.push_back(permutations);
        std::vector<unsigned> permutation;
        // Generate target parameter's possible mapping candidates
        get_permutations(i, layer_values_.at(i), permutation);
        num_permutations *= layer_permutations.at(i).size();
    }
}

uint64_t mapping_space_t::get_num_permutations() const {
    return num_permutations;
}

std::vector<std::vector<std::vector<unsigned>>> mapping_space_t::get_layer_permutations() const {
    return layer_permutations; 
}
mapping_space_t mapping_space_t::partition_off(unsigned tid_, 
                                               unsigned num_threads_) const {
    std::vector<std::vector<std::vector<unsigned>>>  partitioned_permutations;
    std::vector<std::vector<unsigned>> permutations;
    // Set range based on tid_ and num_threads_
    range_t range(tid_, num_threads_, layer_permutations);

    for(unsigned i = 0; i < (unsigned)parameter_t::SIZE; i++) {
        partitioned_permutations.push_back(permutations);
    }
    // Push back factors to return vector
    std::vector<unsigned> permutation;
    for(unsigned k = range.start_k; k < range.end_k; k++) {
        // Initial size partitioned permutations
        permutation = get_permutations(unsigned(parameter_t::K)).at(k);
        partitioned_permutations.at((unsigned)parameter_t::K).push_back(permutation);
        permutation.clear();
    }
    for(unsigned b = range.start_b; b < range.end_b; b++) {
        permutation = get_permutations(unsigned(parameter_t::B)).at(b);
        partitioned_permutations.at((unsigned)parameter_t::B).push_back(permutation);
        permutation.clear();
    }
    for(unsigned p = range.start_p; p < range.end_p; p++) {
        permutation = get_permutations(unsigned(parameter_t::P)).at(p);
        partitioned_permutations.at((unsigned)parameter_t::P).push_back(permutation);
        permutation.clear();
    }
    for(unsigned q = range.start_q; q < range.end_q; q++) {
        permutation = get_permutations(unsigned(parameter_t::Q)).at(q);
        partitioned_permutations.at((unsigned)parameter_t::Q).push_back(permutation);
        permutation.clear();
    }
    for(unsigned c = range.start_c; c < range.end_c; c++) {
        permutation = get_permutations(unsigned(parameter_t::C)).at(c);
        partitioned_permutations.at((unsigned)parameter_t::C).push_back(permutation);
        permutation.clear();
    }
    for(unsigned r = range.start_r; r < range.end_r; r++) {
        permutation = get_permutations(unsigned(parameter_t::R)).at(r);
        partitioned_permutations.at((unsigned)parameter_t::R).push_back(permutation);
        permutation.clear();
    }
    for(unsigned s = range.start_s; s < range.end_s; s++) {
        permutation = get_permutations(unsigned(parameter_t::S)).at(s);
        partitioned_permutations.at((unsigned)parameter_t::S).push_back(permutation);
        permutation.clear();
    }
    for(unsigned g = range.start_g; g < range.end_g; g++) {
        permutation = get_permutations(unsigned(parameter_t::G)).at(g);
        partitioned_permutations.at((unsigned)parameter_t::G).push_back(permutation);
        permutation.clear();
    }
    // Compute num total mapping candidates
    unsigned num_total_mapping_candidates = 1;
    assert(partitioned_permutations.size() == (unsigned)parameter_t::SIZE);
    for(unsigned i = 0; i < partitioned_permutations.size(); i++) {
        num_total_mapping_candidates *= partitioned_permutations.at(i).size();
    }
    // Generate partitioned mapping space
    mapping_space_t rtn_mapping_space(num_levels, num_total_mapping_candidates, partitioned_permutations);
    return rtn_mapping_space; 
}

std::vector<unsigned> mapping_space_t::get_factors(const unsigned val_) {
    std::vector<unsigned> rtn;
    for(unsigned i = 1; i * i <= val_; i++) {
        if(val_ % i == 0) {
            rtn.push_back(i);
            if(i != val_ / i)
                rtn.push_back(val_ / i);
        }
    }
    std::sort(rtn.begin(), rtn.end());
    return rtn;
} 

std::vector<std::vector<unsigned>> mapping_space_t::get_permutations(const unsigned idx_) const {
    return layer_permutations.at(idx_); 
}

void mapping_space_t::get_permutations(const unsigned idx_, const unsigned val_, 
                                       std::vector<unsigned> &permutation_) {
    // If there's no combinations of distributing parameters (targeted component level is 1)
    if(permutation_.size() == num_levels - 1) {
        permutation_.push_back(val_);
        layer_permutations.at(idx_).push_back(permutation_);
        return;
    }
    // Factorize DNN parameters
    std::vector<unsigned> factors = get_factors(val_);
    for(auto it = factors.begin(); it != factors.end(); ++it) {
        std::vector<unsigned> new_permutation;
        new_permutation.assign(permutation_.begin(), permutation_.end());
        new_permutation.push_back(*it);
        get_permutations(idx_, val_ / *it, new_permutation);
    }
}
// Check traverse all possible mapping candidates
bool mapping_space_t::is_last() const {
    return permutation_index > num_permutations;
}
std::vector<std::vector<unsigned>> mapping_space_t::get_mapping_set() {
    std::vector<std::vector<unsigned>> tmp_mapping_set;
    std::vector<std::vector<unsigned>> mapping_set;
    std::vector<unsigned> tmp_row_mapping;
    
    // Change parameter index
    parameter_index.at((unsigned) parameter_t::G)++;
    // From S to K parameter in scheduling table, increase parameter indices sequencially.
    for(int i = (int) parameter_t::SIZE - 1; i >= 0; i--) {
        if(parameter_index.at(i) > get_permutations(i).size() - 1) {
            parameter_index.at(i) = 0;
            if(i != 0) { parameter_index.at(i - 1)++; }
        }
    }
    // Update parameter indices 
    for(unsigned i = 0; i < (unsigned) parameter_t::SIZE; i++) {
        tmp_mapping_set.push_back(get_permutations(i).at(parameter_index.at(i)));
    }

    // Transpose mapping_values
    for(unsigned i = 0; i < num_levels; i++) {
        for(unsigned j = 0; j < tmp_mapping_set.size(); j++) {
            tmp_row_mapping.push_back(tmp_mapping_set.at(j).at(i));
        }
        mapping_set.push_back(tmp_row_mapping);
        tmp_row_mapping.clear();
    }
    // Increase permutation idx
    permutation_index++; 
    return mapping_set;
}

/* Mapping space range per thread */
range_t::range_t(const unsigned tid_, 
                 const unsigned num_threads_,
                 const std::vector<std::vector<std::vector<unsigned>>>& layer_permutations_) 
    : start_k(0),
      end_k(layer_permutations_.at((unsigned) parameter_t::K).size()),
      start_b(0),
      end_b(layer_permutations_.at((unsigned) parameter_t::B).size()),
      start_p(0),
      end_p(layer_permutations_.at((unsigned) parameter_t::P).size()),
      start_q(0),
      end_q(layer_permutations_.at((unsigned) parameter_t::Q).size()),
      start_c(0),
      end_c(layer_permutations_.at((unsigned) parameter_t::C).size()),
      start_r(0),
      end_r(layer_permutations_.at((unsigned) parameter_t::S).size()),
      start_s(0),
      end_s(layer_permutations_.at((unsigned) parameter_t::R).size()),
      start_g(0),
      end_g(layer_permutations_.at((unsigned) parameter_t::G).size()) {

    bool is_assigned = true;
    unsigned depth = 0;
    unsigned max_num_works = 0;
    unsigned local_num_threads = num_threads_;

    for(unsigned d = 0; d < layer_permutations_.size(); d++) {
        // KBPQCSR
        if(max_num_works < layer_permutations_.at(d).size()) {
            max_num_works = layer_permutations_.at(d).size(); 
            depth = d;
        }
    }

    if(num_threads_ > max_num_works) {
        local_num_threads = max_num_works;
        if(!(tid_ < max_num_works)) {
            is_assigned = false;
            end_k = 0; end_b = 0; end_p = 0; end_q = 0; end_c = 0; end_r = 0; end_s = 0; end_g = 0;
        }
    }

    if(is_assigned) {
        bool change = false;
        unsigned div_first = floor((double(max_num_works) / local_num_threads) + 0.5);
        unsigned div_second = (double(max_num_works) / local_num_threads) - (max_num_works / local_num_threads) >= 0.5 ? div_first - 1 : div_first + 1;
        unsigned start_idx = 0;
        unsigned end_idx = 0;

        for(unsigned tid = 0; tid < tid_ + 1; tid++) {
            if(max_num_works % local_num_threads == 0) {
                start_idx = tid * (max_num_works / local_num_threads);
                end_idx = (tid + 1) * (max_num_works / local_num_threads);
            }
            else {
                if(!change 
                   && (max_num_works - tid * div_first) > (local_num_threads - tid) 
                   && (max_num_works - tid * div_first) % (local_num_threads - tid) != 0) {
                    start_idx = tid * div_first; 
                    end_idx = (tid + 1) * div_first;
                }
                else {
                    change = true;
                    start_idx = max_num_works - (local_num_threads - tid) * div_second; 
                    end_idx = max_num_works - (local_num_threads - tid - 1) * div_second; 
                }
            }
        }
        // Index adjustment
        if(depth == 0) {
            start_k = start_idx;
            end_k = end_idx;
        }
        else if(depth == 1) {
            start_b = start_idx;
            end_b = end_idx;
        }
        else if(depth == 2) {
            start_p = start_idx;
            end_p = end_idx;
        }
        else if(depth == 3) {
            start_q = start_idx;
            end_q = end_idx;
        }
        else if(depth == 4) {
            start_c = start_idx;
            end_c = end_idx;
        }
        else if(depth == 5) {
            start_r = start_idx;
            end_r = end_idx;
        }
        else if(depth == 6) {
            start_s = start_idx;
            end_s = end_idx;
        }
        else if(depth == 7) {
            start_g = start_idx;
            end_g = end_idx;
        }
        else {
            std::cout << "# DEPTH ERROR" << std::endl;
            exit(1);
        }
    }
}

range_t::~range_t() {

}
