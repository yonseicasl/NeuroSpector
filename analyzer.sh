#!/bin/bash

# Configurations
map_arg=configs/mappings/eyeriss_alexnet.csv

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
    ./neurospector 'A' $map_arg
fi

# For debugging
if [[ "$#" -eq 1  ]];  then
    action=$1; shift
    if [[ $action == gdb ]]; then
        gdb --args neurospector 'A' $map_arg
    else
        ./neurospector 'A' $map_arg $action
    fi
fi
