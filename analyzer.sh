#!/bin/bash

# Configurations
acc_arg=configs/accelerators/eyeriss.cfg
map_arg=configs/mappings/eyeriss_alexnet.csv

# Usage
print_help() {
    echo -e "Usage: $0                        for running with all layers"
    echo -e "Usage: $0 [Layer idx > 0]        for running with a target layer"
    echo -e "Usage: $0 [gdb]                  for debugging with all layers"
    echo -e "Usage: $0 [gdb] [Layer idx > 0]  for debugging with a target layer"
    exit 0
}

# Exceptions
if [[ "$#" -gt 2 || $1 = '-h' || $1 = '--help' ]];  then
    print_help
fi

# Num args = 0
if [[ "$#" -eq 0  ]];  then
    ./neurospector 'A' $acc_arg $map_arg
fi

# Num args = 1
if [[ "$#" -eq 1  ]];  then
    if [[ $1 == gdb ]]; then
        gdb --args neurospector 'A' $acc_arg $map_arg
    else
        ./neurospector 'A' $acc_arg $map_arg $1
    fi
fi

# Num args = 2
if [[ "$#" -eq 2  ]];  then
    if [[ $1 == gdb ]]; then
        gdb --args neurospector 'A' $acc_arg $map_arg $2
    else
        print_help
    fi
fi
