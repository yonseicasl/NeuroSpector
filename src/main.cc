#include <ctime>
#include <string>

#include "parser.h"
#include "banner.h"
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
    std::string batch_size       = str_argv.find("batch_size")!=str_argv.end()
                                 ? str_argv.find("batch_size")->second : "1";

    run_t run_type_enum = (run_t)get_enum_type(run_str, run_type);
    if(run_type_enum > run_t::SIZE) {
        std::cerr << "Invalid Run type."
                  << "(Possible run type: optimizer, analyzer)"
                  << std::endl;
        exit(0);
    }
    print_banner();
    if(run_type         != "") { std::clog << "Run_type         = " << run_type       << std::endl; }
    if(accelerator      != "") { std::clog << "Accelerator      = " << accelerator    << std::endl; }
    if(dataflow         != "") { std::clog << "Dataflow         = " << dataflow       << std::endl; }
    if(network          != "") { std::clog << "Network          = " << network        << std::endl; }
    if(batch_size       != "") { std::clog << "Batch size       = " << batch_size     << std::endl; }
    if(optimizer_type   != "") { std::clog << "Optimizer        = " << optimizer_type << std::endl; }
    if(metric           != "") { std::clog << "Target Metric    = " << metric         << std::endl; }
    if(layer            != "") { std::clog << "Target Layer     = " << layer          << std::endl; }
    if(scheduling_table != "") { std::clog << "Scheduling table = " << scheduling_table << std::endl; }
    std::clog << "Num. threads     = " << thread;
    std::clog << "\n=======================================================";
    
    clock_t start, finish;
    // Search start
    start = time(nullptr);
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
            if(dataflow.empty()) {
                std::cerr << "[Error] Specify dataflow (fixed, flexible, or"
                          << " specific dataflow (ws-os, ws-is, or etc))"
                          << std::endl;
                exit(0);
            }
            if(metric.empty()) {
                std::cerr << "[Error] Specify optimization objectives "
                          << "(energy or cycle)"
                          << std::endl;
                exit(0);
            }
            if(optimizer_type.compare("bottom-up") == 0) {
                bottom_up_t *optimizer = new bottom_up_t(accelerator, dataflow, 
                                                    network, layer, batch_size, 
                                                    metric,
                                                    cl_optimization);
                std::cerr << "[message] Run optimizer" << std::endl;
                // If multi-chip partitioning
                if(!list_layer.empty()) {
                    std::cerr << "[message] Run Multichip partitioning"
                              << std::endl;
                    optimizer->run(list_layer);
                }
                else { optimizer->run(); }
                delete optimizer;
            }
            else if(optimizer_type.compare("brute-force") == 0) {
                brute_force_t *optimizer = new brute_force_t(accelerator, 
                                                    dataflow, network, layer, 
                                                    batch_size, metric, thread);
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
                analyzer_t *analyzer = new analyzer_t(accelerator, dataflow, 
                                                      network, batch_size,
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
            break;
    }
    // Search complete
    finish = time(nullptr);
    std::cout << "[message] Runtime: "
         << (finish - start) / 60 << "m "
         << (finish - start) % 60 << "s ("
         << (finish - start) << ")" << std::endl;
    return 0;
}
