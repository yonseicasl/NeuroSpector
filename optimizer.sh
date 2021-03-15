#!/bin/bash

# Configurations
# acc_arg    : Accelerator specification (*.cfg)
# net_arg    : DNN layer configuration (*.csv)
# opt_type   : "b-f-energy", "b-f-cycle", "b-f-edp", or "systematic"
# num_threads: Multi-threading for brute-force (b-f-xxx)
# dataflows  : "fixed" or "flexible"
acc_arg=configs/accelerators/eyeriss-like.cfg
net_arg=configs/networks/alexnet_b8.csv
opt_type=b-f-energy
num_threads=8
dataflows=fixed

# Usage
print_help() {
    echo -e "Usage: $0                           for running with all layers"
    echo -e "Usage: $0 [Layer index > 0]         for running with a target layer"
    echo -e "Usage: $0 [gdb]                     for debugging with all layers"
    echo -e "Usage: $0 [gdb] [Layer index > 0]   for debugging with a target layer"
    exit 0
}

# Exceptions
if [[ "$#" -gt 2 || $1 = '-h' || $1 = '--help' ]];  then
    print_help
fi

# Num args = 0
if [[ "$#" -eq 0  ]];  then
    ./neurospector 'O' $acc_arg $net_arg $dataflows $num_threads
fi

# Num args = 1
if [[ "$#" -eq 1  ]];  then
    if [[ $1 == gdb ]]; then
        gdb --args neurospector 'O' $acc_arg $net_arg $dataflows $num_threads
    else
        ./neurospector 'O' $acc_arg $net_arg $dataflows $1 $num_threads
    fi
fi

# Num args = 2
if [[ "$#" -eq 2  ]];  then
    if [[ $1 == gdb ]]; then
        gdb --args neurospector 'O' $acc_arg $net_arg $dataflows $2 $num_threads
    else
        print_help
    fi
fi
