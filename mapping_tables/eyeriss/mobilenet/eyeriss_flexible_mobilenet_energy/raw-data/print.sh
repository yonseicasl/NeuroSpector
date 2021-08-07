#!/bin/bash
MAX_NUM=$2

for ((var=1; var <= $MAX_NUM ; var++))
do
    sed 114p -n eyeriss_$1_mobilenet_energy_$var.txt
done
