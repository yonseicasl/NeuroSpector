# NeuroSpector: Systematic Optimization of Dataflow and Hardware Mapping in Deep Neural Network Accelerators
Developed by Chanho Park, Bogil Kim, Sungmin Ryu, and William J. Song\
Intelligent Computing Systems Lab, Yonsei University

## Notice
NeuroSpector is currently at the final stage of code cleanup, and we expect that the initial release will be ready by mid-August, 2022. The master branch has the latest stable copy of the NeuroSpector framework.

## Table of Contents
1. [Intoduction](#introduction)
2. [Compile](#compile)
3. [Run](#run)
4. [Download](#download)
5. [Reference](#reference)

## Introduction
A number of hardware accelerators have been proposed to speed up deep neural network (DNN) computations and enhance energy efficiency. DNN accelerators offer high-throughput and energy-efficient processing options by deploying many processing elements (PEs) in parallel and constructing multi-level memory hierarchy to exploit data reuse within the accelerators. For diverse DNN layer configurations (e.g., width, height, channel) and various accelerator implementations with distinct hardware specifications (e.g., number of PEs, buffer size), it is a highly challenging task to find proper ways of executing DNN layers on given hardware accelerators to maximize performance and energy efficiency. A broad range of DNN dataflow and hardware mapping options create an enormous problem space to explore. The complexity of problem is compounded by tightly coupled correlations between the hardware design of accelerators and computational ordering of DNN layers.

_NeuroSpector_ is a scheduling optimization framework that systematically analyzes the dataflow and mapping possibilities of DNN workloads in accelerators and rapidly identifies optimal scheduling schemes. NeuroSpector finds efficient scheduling solutions for DNN accelerators by 8,000x faster than prior techniques with only 1.6% energy and cycle differences on average to the optimal schemes obtained from fully comprehensive brute-force search, whereas the previous techniques exhibit 83.1% greater energy and cycle results than the optimal solutions and as much as 14.9x in the worst case. In addition, NeuroSpector supports many essential features of DNN accelerators and workloads including group convolutions, multi-chip accelerators, data bypassing in buffers, unified/separate buffer types, static power modeling, and network-wise scheduling optimization, which were only partly supported or overlooked in the prior work.

NeuroSpector is currently at the final stage of code cleanup, and we expect that the initial release will be ready by mid-August, 2022. The NeuroSpector framework is available at [https://github.com/yonsei-icsl/NeuroSpector](https://github.com/yonsei-icsl/NeuroSpector).

## Compile
NeuroSpector uses g++ to compile C++ codes to execute on CPUs. Compile of NeuroSpector is simply possible as below.

	$ make

## Run
### Optimizer
NeuroSpector supports `optimizer` which finds the optimal scheduling solution among all possible scheduling options for a given accelerator and DNN model configurations.  

A run command to execute NeuroSpector optimizer follows the format shown below.

	$ ./NeuroSpector --run_type=optimizer 
			 --accelerator=<PATH TO ACCELERATOR> 
			 --network=<PATH TO DNN MODEL> 
			 --dataflow=<fixed or flexible> 
			 --optimizer=<OPTIMIZATION METRIC> 
			 --metric=<energy, cycle> 
			 --layer=<SELECT TARGET LAYER>

For example, a user can find  an `energy`-optimal scheduling solution using `bottom-up` search for (`Eyeriss`, `ResNet50`).

	$ ./NeuroSpector --run_type=optimizer\
			 --accelerator=configs/accelerators/eyeriss.cfg\
			 --network=configs/networks/resnet50.cfg\
			 --dataflow=fixed\
			 --optimizer=bottom-up\
			 --metric=energy

### Analyzer
NeuroSpector also supports `analzyer` which evaluate a given scheduling table.
Following command is an example command for evaluating sample scheduling table.

	$ ./NeuroSpector --run_type=analyzer\
			 --accelerator=configs/accelerators/eyeriss.cfg\
			 --network=configs/networks/resnet50.cfg\
			 --scheduling_table=configs/scheduling_table/eyeriss_sample.cfg


## Download
To use the latest development version, clone the git repository.

	$ git clone https://github.com/yonsei-icsl/NeuroSpector

## Reference
Below shows the brief informantion of the related publication which is under preparation.

	@inproceedings{park_icsl2021,
	    author    = {C. Park and and B. Kim and S. Ryu and W. Song},
	    title     = {{NeuroSpector: Systematic Optimization of Dataflow Scheduling in Deep Neural Network Accelerators}},
	    booktitle = {under review},
	    month     = {Aug.},
	    year      = {2022},
	}
