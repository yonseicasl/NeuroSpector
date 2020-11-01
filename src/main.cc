#include <iostream>
#include <string>

#include "analyzer.h"
#include "configs.h"
#include "optimizer.h"
#include "utils.h"

#define ANAL_OPTI 1

static handler_t handler;

using namespace std;

int main(int argc, char **argv) {
    if(argc != 2 && argc != 3) {
        cerr << "Usage: " << argv[0] << "[config.csv]" << endl;
        cerr << "Usage: " << argv[0] << "[config.csv] [Layer index > 0]" << endl;
        exit(1);
    }
    // Parse configurations
    if(!ANAL_OPTI) {
        analyzer_configs_t analyzer_cfg(argv[1]);
        analyzer_t *analyzer = new analyzer_t(analyzer_cfg);
        if(argc == 3) {
            unsigned layer_idx = std::stoi(argv[2]);
            if(layer_idx == 0)
                handler.print_err(err_type_t::INVAILD, "Layer index must be more than 0");
            analyzer->print_stats(layer_idx);
        }
        else
            analyzer->print_stats(0);
        delete analyzer;
    }
    else {
        optimizer_configs_t optimizer_cfg(argv[1]);
        optimizer_t *optimizer = new optimizer_t(optimizer_cfg);
        optimizer->optimize();
        if(argc == 3) {
            unsigned layer_idx = std::stoi(argv[2]);
            if(layer_idx == 0)
                handler.print_err(err_type_t::INVAILD, "Layer index must be more than 0");
            //optimizer->print_stats(layer_idx);
        }
        else
            //optimizer->print_stats(0);
        delete optimizer;
    }
    return 0;
}
