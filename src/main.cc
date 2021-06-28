#include <ctime>
#include <iostream>
#include <string>

#include "analyzer.h"
#include "configs.h"
#include "enums.h"
#include "optimizer.h"
#include "utils.h"

static handler_t handler;

using namespace std;

void print_usage(char **argv) {
    cerr << "# Analyzer" << endl;
    cerr << "  Usage: " << argv[0] << "A [acc.cfg] [map.csv]" << endl;
    cerr << "  Usage: " << argv[0] << "A [acc.cfg] [map.csv] [layer_idx > 0]" << endl;
    cerr << "# Optimizer" << endl;
    cerr << "  Dataflows: fixed or flexible" << endl;
    cerr << "  Usage: " << argv[0] << "O [acc.cfg] [net.csv] [opt_type] [num_threads] [dataflows]" << endl;
    cerr << "  Usage: " << argv[0] << "O [acc.cfg] [net.csv] [opt_type] [num_threads] [dataflows] [layer_idx > 0]" << endl;
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
        // Start analyzing
        if(argc == 5) {
            unsigned layer_idx = stoi(argv[4]);
            if(layer_idx == 0)
                handler.print_err(err_type_t::INVAILD, "layer_idx must be more than 0");
            analyzer->print_stats(layer_idx);
        }
        else
            analyzer->print_stats();
        delete analyzer;
    }
    // Optimizer
    else if(*argv[1] == 'O') {
        // opt_type: "b-f-energy", "b-f-cycle", "b-f-edp", or "hierarchical"
        string opt_type_str(argv[4]);
        opt_type_t opt_type = opt_type_t::SIZE;
        if(opt_type_str.compare("b-f-energy") == 0) opt_type = opt_type_t::B_F_ENERGY;
        else if(opt_type_str.compare("b-f-cycle") == 0) opt_type = opt_type_t::B_F_CYCLE;
        else if(opt_type_str.compare("b-f-edp") == 0) opt_type = opt_type_t::B_F_EDP;
        else if(opt_type_str.compare("bottom-up") == 0) opt_type = opt_type_t::BOTTOM_UP;
        else handler.print_err(err_type_t::INVAILD, "opt_type: b-f-energy, b-f-cycle, b-f-edp, or bottom-up");
        // num_threads: Multi-threading for brute-force (b-f-xxx)
        int num_threads = stoi(argv[5]);
        if(num_threads <= 0)
            handler.print_err(err_type_t::INVAILD, "num_threads must be more than 0");
        // dataflows (is_fixed): fixed (true) or flexible (false)
        bool is_fixed = true;
        string is_fixed_str(argv[6]);
        if(is_fixed_str.compare("fixed") == 0) is_fixed = true;
        else if(is_fixed_str.compare("flexible") == 0) is_fixed = false;
        else handler.print_err(err_type_t::INVAILD, "dataflows: fixed or flexible");
        // Optimizer initialization
        if(opt_type == opt_type_t::BOTTOM_UP) {
            bottom_up_t * optimizer = new bottom_up_t(argv[2], argv[3], is_fixed);
            // Start hierarchical optimizing
            if(argc == 8) {
                unsigned layer_idx = stoi(argv[7]);
                if(layer_idx == 0)
                    handler.print_err(err_type_t::INVAILD, "layer_idx must be more than 0");
                optimizer->run(layer_idx);
            }
            else
                optimizer->run();
            delete optimizer;
        }
        else {
            brute_force_t *optimizer = new brute_force_t(argv[2], argv[3], opt_type, num_threads, is_fixed);
            // Start brute-force optimizing
            if(argc == 8) {
                unsigned layer_idx = stoi(argv[7]);
                if(layer_idx == 0)
                    handler.print_err(err_type_t::INVAILD, "layer_idx must be more than 0");
                optimizer->run(layer_idx);
            }
            else
                optimizer->run();
            delete optimizer;
        }
    }
    else 
        print_usage(argv);
    // Finish time
    finish = time(nullptr);
    handler.print_line(50, "*");
    cout << "# TIME: " 
         << (finish - start) / 60 << " min "
         << (finish - start) % 60 << " sec ("
         << (finish - start) << ")" << endl;
    handler.print_line(50, "*");
    return 0;
}
