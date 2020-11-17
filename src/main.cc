#include <iostream>
#include <string>

#include "analyzer.h"
#include "configs.h"
#include "optimizer.h"
#include "utils.h"

static handler_t handler;

using namespace std;

int main(int argc, char **argv) {
    // Analyzer
    if(*argv[1] == 'A') {
        mapping_configs_t mapping_configs(argv[2]);
        analyzer_t *analyzer = new analyzer_t(mapping_configs);
        if(argc == 4) {
            unsigned layer_idx = std::stoi(argv[3]);
            if(layer_idx == 0)
                handler.print_err(err_type_t::INVAILD, "Layer index must be more than 0");
            analyzer->print_stats(layer_idx);
        }
        else
            analyzer->print_stats(0);
        delete analyzer;
    }
    // Optimizer
    else if(*argv[1] == 'O') {
        accelerator_configs_t accelerator_configs(argv[2]);
        network_configs_t network_configs(argv[3]);
        optimizer_t *optimizer = new optimizer_t(accelerator_configs, network_configs);
        optimizer->optimize();
        if(argc == 5) {
            unsigned layer_idx = std::stoi(argv[4]);
            if(layer_idx == 0)
                handler.print_err(err_type_t::INVAILD, "Layer index must be more than 0");
            optimizer->print_stats(layer_idx);
        }
        else
            optimizer->print_stats(0);
        delete optimizer;
    }
    else {
        cerr << "Usage: " << argv[0] << "A [mapping.csv]" << endl;
        cerr << "Usage: " << argv[0] << "A [mapping.csv] [Layer index > 0]" << endl;
        cerr << "Usage: " << argv[0] << "O [accelerator.csv] [network.csv]" << endl;
        cerr << "Usage: " << argv[0] << "O [accelerator.csv] [network.csv] [Layer index > 0]" << endl;
        exit(1);
    }
    return 0;
}
