#include <iostream>

#include "analyzer.h"
#include "configs.h"
#include "utils.h"

static handler_t handler;

using namespace std;

int main(int argc, char **argv) {
    if(argc != 2) {
        cerr << "Usage: " << argv[0] << "[config.csv]" << endl;
        exit(1);
    }
    // Parse configurations
    analyzer_configs_t analyzer_cfg(argv[1]);
    analyzer_t *analyzer = new analyzer_t(analyzer_cfg);
    analyzer->print_mapping_tables();

    delete analyzer;
    return 0;
}
