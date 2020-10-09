#include "network.h"
#include "utils.h"

static handler_t handler;

/* Network */
network_t::network_t() 
    : num_layers(0) {
}

network_t::~network_t() {
    for(unsigned i = 0; i < num_layers; i++)
        delete layers.at(i);
}

void network_t::init() {
    if(num_layers == 0)
        handler.print_err(err_type_t::GENERAL, "No layer in network_t::init()");
    else {
        for(unsigned i = 0; i < num_layers; i++) {
            layer_t *layer = new layer_t(i + 1);
            layers.push_back(layer);
        }
    } 
}

void network_t::print_stats() {
    std::cout << "# Network: " << name << std::endl;
    for(size_t i = 0; i < layers.size(); i++) {
        layers.at(i)->print_stats();
    }
}

/* Layer */
layer_t::layer_t(unsigned id_) 
    : B(1), G(1), H(1), W(1), C(1), 
    R(1), S(1), K(1), P(1), Q(1),
    stride(1), padding(0), id(id_) {
}

layer_t::~layer_t() {
}

void layer_t::init() {
    P = (H + 2 * padding - R) / stride + 1;
    Q = (W + 2 * padding - S) / stride + 1;
    // Layer type
    if(H == 1 && W == 1 && R == 1 && S == 1 && padding == 0 && stride == 1) {
        layer_type = layer_type_t::FULLYCONNECTED;
        return;
    }
    else if(R == 1 && S == 1 && stride == 1) {
        layer_type = layer_type_t::POINTWISE;
        return; 
    }
    else if(G == C && G == K) {
        layer_type = layer_type_t::DEPTHWISE;
        return; 
    }
    else {
        layer_type = layer_type_t::STANDARD;
        return;
    }
}

void layer_t::print_stats() {
    std::cout << "# Layer " << id << " Stats" << std::endl;
    switch(layer_type) {
        case layer_type_t::STANDARD:
            std::cout << "  TYPE    : STANDARD CONV" << std::endl; break; 
        case layer_type_t::DEPTHWISE:
            std::cout << "  TYPE    : DEPTH-WISE CONV" << std::endl; break; 
        case layer_type_t::POINTWISE:
            std::cout << "  TYPE    : POINT-WISE CONV" << std::endl; break; 
        case layer_type_t::FULLYCONNECTED:
            std::cout << "  TYPE    : FULLY-CONNECTED" << std::endl; break; 
        default:
            handler.print_err(err_type_t::INVAILD, "Invalid layer type in layer_t::print_stats()");
    }
    if(layer_type == layer_type_t::FULLYCONNECTED) {
        std::cout << "  INPUT   : " << "B(" << B << ") C(" << C << ")\n";
        std::cout << "  WEIGHT  : " << "K(" << K << ") C(" << C << ")\n";
        std::cout << "  OUTPUT  : " << "B(" << B << ") K(" << K << ")\n";
    }
    else {
        std::cout << "  INPUT   : " << "B(" << B << ") C(" << C << ") H(" << H << ") W(" << W << ")\n";
        std::cout << "  WEIGHT  : " << "K(" << K << ") C(" << C << ") R(" << R << ") S(" << S << ")\n";
        std::cout << "  OUTPUT  : " << "B(" << B << ") K(" << K << ") P(" << P << ") Q(" << Q << ")\n";
        std::cout << "  GROUP(G): " << G << "\n";
        std::cout << "  STRIDE  : " << stride << "\n";
        std::cout << "  PADDING : " << padding << std::endl;
    }
}
