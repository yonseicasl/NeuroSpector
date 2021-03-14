#include <ctime>
#include <iostream>
#include <string>

#include "analyzer.h"
#include "configs.h"
#include "optimizer.h"
#include "utils.h"

static handler_t handler;

using namespace std;

void print_usage(char **argv) {
    cerr << "# Analyzer" << endl;
    cerr << "  Usage: " << argv[0] << "A [accelerator.cfg] [mapping.csv]" << endl;
    cerr << "  Usage: " << argv[0] << "A [accelerator.cfg] [mapping.csv] [Layer index > 0]" << endl;
    cerr << "# Optimizer" << endl;
    cerr << "  Dataflows: fixed or flexible" << endl;
    cerr << "  Usage: " << argv[0] << "O [accelerator.cfg] [network.csv] [Dataflows]" << endl;
    cerr << "  Usage: " << argv[0] << "O [accelerator.cfg] [network.csv] [Dataflows] [Layer index > 0]" << endl;
    exit(1);
}

int main(int argc, char **argv) {
    clock_t start;
    clock_t finish;
    // Start time
    start = time(nullptr);
    // Analyzer
    if(*argv[1] == 'A') {
        // Analyzer initialization
        analyzer_t *analyzer = new analyzer_t(argv[2], argv[3]);
        // Analyzing
        if(argc == 5) {
            unsigned layer_idx = stoi(argv[4]);
            if(layer_idx == 0)
                handler.print_err(err_type_t::INVAILD, "Layer index must be more than 0");
            analyzer->print_stats(layer_idx);
        }
        else
            analyzer->print_stats();
        delete analyzer;
    }
    // Optimizer
    else if(*argv[1] == 'O') {
        // Dataflow: fiexed (true) or flexible (false)
        bool is_fixed = true;
        string is_fixed_str(argv[4]);
        if(is_fixed_str.compare("fixed") == 0) 
            is_fixed = true;
        else if(is_fixed_str.compare("flexible") == 0) 
            is_fixed = false;
        else
            handler.print_err(err_type_t::INVAILD, "Dataflow policy: fixed or flexible");
        // Optimizer initialization
        unsigned num_threads = stoi(argv[6]);
        optimizer_t *optimizer = new optimizer_t(argv[2], argv[3], is_fixed, num_threads);
        // Optimizing
        if(argc == 7) {
            unsigned layer_idx = stoi(argv[5]);
            if(layer_idx == 0)
                handler.print_err(err_type_t::INVAILD, "Layer index must be more than 0");
            //optimizer->run_brute_force(layer_idx);
            optimizer->run_two_lv_by_two_lv(layer_idx);
        }
        else
            optimizer->run_brute_force();
        delete optimizer;
    }
    else 
        print_usage(argv);
    // Finish time
    finish = time(nullptr);
    cout << "\n# TIME: " 
         << (finish - start) << endl;
    cout << "\n# TIME: " 
         << (finish - start) / 60 << " min "
         << (finish - start) % 60 << " sec " << endl;
    return 0;
}
