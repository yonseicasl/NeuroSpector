#!/bin/bash

output=x.csv

./optimizer.sh 1 >> $output; 
./optimizer.sh 2 >> $output; 
./optimizer.sh 3 >> $output; 
./optimizer.sh 4 >> $output; 
./optimizer.sh 5 >> $output; 
