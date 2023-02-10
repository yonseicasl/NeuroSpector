#ifndef __UTILS_H__
#define __UTILS_H__

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

// Convert string to lowercase
std::string& lowercase(std::string &str_); 
// Convert string to uppercase
std::string& uppercase(std::string &str_); 
// Split comma-separated string
std::vector<std::string> split(std::string str_, char divisor_); 
// Convert Comma separated string to vector
std::vector<unsigned> comma_to_vector(std::string value_); 


#endif
