#!/bin/bash

# Configurations
acc_arg=configs/accelerators/eyeriss.csv
net_arg=configs/networks/alexnet.csv

# Usage
print_help() {
    echo -e "Usage: $0                   for running"
    echo -e "Usage: $1 [Layer index > 0] for running"
    echo -e "Usage: $0 [gdb]             for debugging"
    exit 0
}
if [[ "$#" -gt 1 || $1 = '-h' || $1 = '--help' ]];  then
    print_help
fi

# For running
if [[ "$#" -eq 0  ]];  then
    ./neurospector 'O' $acc_arg $net_arg
fi

# For debugging
if [[ "$#" -eq 1  ]];  then
    action=$1; shift
    if [[ $action == gdb ]]; then
        gdb --args neurospector 'O' $acc_arg $net_arg
    else
        ./neurospector 'O' $acc_arg $net_arg $action
    fi
fi
