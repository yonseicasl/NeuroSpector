#ifndef __LAYERS_H__
#define __LAYERS_H__

#include <iostream>
#include <vector>

enum class layer_type_t {
    STANDARD, DEPTHWISE, POINTWISE, FULLYCONNECTED, SIZE
};

class layer_t;

class network_t {
public:
    network_t();
    ~network_t();
    void init();
    void print_stats();
    layer_t* layer(unsigned num_) { return layers.at(num_); }

    unsigned num_layers;
    std::string name;

private:
    std::vector<layer_t*> layers;
};

class layer_t {
public:
    layer_t(unsigned id_);
    ~layer_t();
    void init();
    void print_stats();

    unsigned B;		// Batch
    unsigned G;		// Group
    unsigned H;		// Input Height
    unsigned W;		// Input Width
    unsigned C;		// Input(Weight) Channel
    unsigned R;		// Weight Height
    unsigned S;		// Weigth Width
    unsigned K;		// # of Weights(Output Channel)
    unsigned P;		// Output Height
    unsigned Q;		// Output Width
    unsigned stride;
    unsigned padding;
    layer_type_t layer_type;
 
private:
    unsigned id;
};

#endif 
