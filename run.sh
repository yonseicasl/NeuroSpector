#!/bin/bash

# Configuration
analyzer_arg=configs/eyeriss_alexnet.csv
optimizer_arg=configs/

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
    ./neurospector $analyzer_arg
fi

# For debugging
if [[ "$#" -eq 1  ]];  then
    action=$1; shift
    if [[ $action == gdb ]]; then
        gdb --args neurospector $analyzer_arg
    else
        ./neurospector $analyzer_arg $action
    fi
fi
