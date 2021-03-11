#include "mapping_space.h"

mapping_space_t::mapping_space_t(const unsigned num_levels_, 
                                 const std::vector<unsigned> &layer_values_) 
    : num_levels(num_levels_), 
      num_permutations(1) {
    for(unsigned i = 0; i < layer_values_.size(); i++) {
        std::vector<std::vector<unsigned>> permutations;
        layer_permutations.push_back(permutations);
        std::vector<unsigned> permutation;
        get_permutations(i, layer_values_.at(i), permutation);
        num_permutations *= layer_permutations.at(i).size();
    }
}

mapping_space_t::~mapping_space_t() {

}

void mapping_space_t::print_permutations() const {
    std::cout << "**** PERMUTATIONS ****" << std::endl;
    std::string str = "KBPQCSR";
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

uint64_t mapping_space_t::get_num_permutations() const {
    return num_permutations;
}

std::vector<std::vector<unsigned>> mapping_space_t::get_permutations(const unsigned idx_) const {
    return layer_permutations.at(idx_); 
}

std::vector<std::vector<std::vector<unsigned>>> mapping_space_t::get_layer_permutations() const {
    return layer_permutations; 
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

void mapping_space_t::get_permutations(const unsigned idx_, const unsigned val_, std::vector<unsigned> &permutation_) {
    if(permutation_.size() == num_levels - 1) {
        permutation_.push_back(val_);
        layer_permutations.at(idx_).push_back(permutation_);
        return;
    }
    std::vector<unsigned> factors = get_factors(val_);
    for(auto it = factors.begin(); it != factors.end(); ++it) {
        std::vector<unsigned> new_permutation;
        new_permutation.assign(permutation_.begin(), permutation_.end());
        new_permutation.push_back(*it);
        get_permutations(idx_, val_ / *it, new_permutation);
    }
}

/* Mapping space range per thread */
range_t::range_t(const unsigned tid_, 
                 const unsigned num_threads_,
                 const std::vector<std::vector<std::vector<unsigned>>>& layer_permutations_,
                 std::mutex& m_) 
    : start_k(0),
      end_k(layer_permutations_.at(0).size()),
      start_b(0),
      end_b(layer_permutations_.at(1).size()),
      start_p(0),
      end_p(layer_permutations_.at(2).size()),
      start_q(0),
      end_q(layer_permutations_.at(3).size()),
      start_c(0),
      end_c(layer_permutations_.at(4).size()),
      start_s(0),
      end_s(layer_permutations_.at(5).size()),
      start_r(0),
      end_r(layer_permutations_.at(6).size()) {

    unsigned depth = 0;
    unsigned num_works = 1;

    for(unsigned d = 0; d < layer_permutations_.size(); d++) {
        // KBPQCSR
        num_works = layer_permutations_.at(d).size(); 
        if(num_threads_ <= num_works) 
            break;
        depth++;
    }

    if(num_threads_ > num_works) {
        std::cout << "# NUM OF THREADS MUST BE SMALLER THAN " << num_works << std::endl;
        exit(1);
    }

    bool change = false;
    unsigned div_first = floor((double(num_works) / num_threads_) + 0.5);
    unsigned div_second = (double(num_works) / num_threads_) - (num_works / num_threads_) >= 0.5 ? div_first - 1 : div_first + 1;
    unsigned start_idx = 0;
    unsigned end_idx = 0;

    for(unsigned tid = 0; tid < tid_ + 1; tid++) {
        if(num_works % num_threads_ == 0) {
            start_idx = tid * (num_works / num_threads_);
            end_idx = (tid + 1) * (num_works / num_threads_);
        }
        else {
            if(!change 
               && (num_works - tid * div_first) > (num_threads_ - tid) 
               && (num_works - tid * div_first) % (num_threads_ - tid) != 0) {
                start_idx = tid * div_first; 
                end_idx = (tid + 1) * div_first;
            }
            else {
                change = true;
                start_idx = num_works - (num_threads_ - tid) * div_second; 
                end_idx = num_works - (num_threads_ - tid - 1) * div_second; 
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
        start_s = start_idx;
        end_s = end_idx;
    }
    else if(depth == 6) {
        start_r = start_idx;
        end_r = end_idx;
    }
    else {
        std::cout << "# DEPTH ERROR" << std::endl;
        exit(1);
    }
    std::string str = "KBPQCSR";
    m_.lock();
    std::cout << "# TID " << std::setw(2) << tid_ << ": " << str.at(depth) << "(" << num_works << ") FROM " 
              << std::setw(5) << start_idx << " TO " << std::setw(5) << end_idx << std::endl;
    m_.unlock();
}

range_t::~range_t() {

}
