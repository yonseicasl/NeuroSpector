#include <ctime>
#include <iostream>
#include <string>

#include "parser.h"
#include "bottom_up.h"
#include "brute_force.h"

static parser_t parser;

int main(int argc, char **argv) {
    
    // Get user-specified inputs
    std::map<std::string, std::string> str_argv;
    parser.argparse(argc, argv, str_argv);

    std::string run_type         = str_argv.find("run_type")!=str_argv.end() 
                                 ? str_argv.find("run_type")->second : "";
    std::string accelerator      = str_argv.find("accelerator")!=str_argv.end()
                                 ? str_argv.find("accelerator")->second : "";
    std::string dataflow         = str_argv.find("dataflow")!=str_argv.end()
                                 ? str_argv.find("dataflow")->second : "";
    std::string network          = str_argv.find("network")!=str_argv.end()
                                 ? str_argv.find("network")->second : "";
    std::string optimizer_type   = str_argv.find("optimizer")!=str_argv.end()
                                 ? str_argv.find("optimizer")->second : "";
    std::string metric           = str_argv.find("metric")!=str_argv.end()
                                 ? str_argv.find("metric")->second : "";
    std::string layer            = str_argv.find("layer")!=str_argv.end()
                                 ? str_argv.find("layer")->second : "";
    std::string thread           = str_argv.find("thread")!=str_argv.end()
                                 ? str_argv.find("thread")->second : "1";
    std::string scheduling_table = str_argv.find("scheduling_table")!=str_argv.end()
                                 ? str_argv.find("scheduling_table")->second : "";
    std::string mc_partitioning  = str_argv.find("mc_partitioning")!=str_argv.end()
                                 ? str_argv.find("mc_partitioning")->second : "";
    std::string cl_optimization  = str_argv.find("cl_optimization")!=str_argv.end()
                                 ? str_argv.find("cl_optimization")->second : "";

    std::clog << "[message] Run NeuroSpector" 
              << "\nRun_type         = " << run_type
              << "\nAccelerator      = " << accelerator
              << "\nDataflow         = " << dataflow
              << "\nNetwork          = " << network
              << "\nOptimizer        = " << optimizer_type
              << "\nTarget Metric    = " << metric
              << "\nTarget Layer     = " << layer
              << "\nNum. threads     = " << thread;
    
    clock_t start, finish;
    // Search start
    start = time(nullptr);
    run_t run_type_enum = run_type == "optimizer" 
                             ? run_t::OPTIMIZER : run_t::ANALYZER;
    std::vector<unsigned> list_layer;
    if(!mc_partitioning.empty()) {
        std::clog << "\nMulti. Chip. P   = " << mc_partitioning;
        list_layer = comma_to_vector(mc_partitioning);
    }
    if(cl_optimization == "true") {
        std::clog << "\nCross Layer Opt  = " << cl_optimization; 
    }
    std::clog << std::endl;

    switch(run_type_enum) {
        case run_t::OPTIMIZER:
            if(optimizer_type.compare("bottom-up") == 0) {
                bottom_up_t *optimizer = new bottom_up_t(accelerator, dataflow, 
                                                        network, layer, metric,
                                                        cl_optimization);
            
                // Run optimizer
                std::cerr << "[message] Run optimizer" << std::endl;
                // If multi-chip partitioning
                if(!list_layer.empty()) {
                    std::cerr << "[message] Run Multichip partitioning" << std::endl;
                    optimizer->run(list_layer);
                }
                else { optimizer->run(); }
                delete optimizer;
            }
            else if(optimizer_type.compare("brute-force") == 0) {
                brute_force_t *optimizer = new brute_force_t(accelerator, dataflow, 
                                                            network, layer, metric, 
                                                            thread);
                // Run optimizer
                std::cerr << "[message] Run optimizer" << std::endl;
                optimizer->run();
                delete optimizer;
            }
            else { 
                std::cerr << "Invalid Optimizer."
                          << "(Possible optimizer : bottom-up, brute-force)"
                          << std::endl;
            }
            break;
        case run_t::ANALYZER:
            if(!scheduling_table.empty()) {
                std::clog << "Scheduling table = " << scheduling_table << std::endl;
                analyzer_t *analyzer = new analyzer_t(accelerator, network,
                                                      scheduling_table);
                std::cerr << "[message] Run Analyzer" << std::endl;
                analyzer->run();
                delete analyzer;
            }
            else {
                std::cerr << "Invalid Analzyer."
                            << "Type scheduling_table path"
                            << std::endl;
            }
            break;

        default:
            std::cerr << "Invalid Run type."
                      << "(Possible run type: optimizer, analyzer)"
                      << std::endl;

            exit(0);
    }


    // Search complete
    finish = time(nullptr);
    std::cout << "[message] Runtime: "
         << (finish - start) / 60 << "m "
         << (finish - start) % 60 << "s ("
         << (finish - start) << ")" << std::endl;
    return 0;
}
