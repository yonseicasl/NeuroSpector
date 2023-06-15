# NeuroSpector: Dataflow and Mapping Optimization of Deep Neural Network Accelerators
Developed by Chanho Park, Bogil Kim, Sungmin Ryu, and William J. Song\
Intelligent Computing Systems Lab, Yonsei University\
Current release: v1.4.1 (Aug. 2023)

## Table of Contents
1. [Intoduction](#introduction)
2. [Compile](#compile)
3. [Run](#run)
4. [Download](#download)
5. [Reference](#reference)

## Introduction
A number of hardware accelerators have been proposed to speed up deep neural network (DNN) computations and enhance energy efficiency. DNN accelerators offer high-throughput and energy-efficient computing solutions by deploying many processing elements (PEs) and chips in parallel and exploiting data reuse across multiple levels of the memory hierarchy in the accelerators. The vertical and spatial arrangements of buffers and PEs construct a multi-level hierarchy of accelerator components from multiply-accumulate (MAC) units to global buffers and off-chip DRAM. For diverse DNN workload configurations and accelerator implementations, it is a highly challenging problem to find the proper ways of executing neural layers on accelerators to maximize energy efficiency and performance. The challenge lies in that hardware specifications (e.g., number of PEs and chips, buffer sizes and types) associated with workload configurations (e.g., width, height, channel, batch size) produce a huge number of possible dataflow and mapping options that can be exercised in an accelerator.

_NeuroSpector_ is a scheduling optimization framework that systematically analyzes the dataflow and mapping possibilities of DNN workloads in accelerators and rapidly identifies optimal execution schemes. NeuroSpector finds scheduling solutions for a variety of DNN accelerators and workloads 7,958x faster than previous work with only 1.5% energy and cycle differences on average to the optimal schemes, whereas the prior techniques produce hit-or-miss results with 100.1% greater energy and cycle results than the optimal solutions and as much as 14.9x in the worst case. In addition, NeuroSpector supports many essential features of DNN accelerators and workloads, including group convolutions, multi-chip accelerators, data bypassing in buffers, unified/separate buffer types, static power modeling, and network-wise scheduling optimization, which were overlooked or only partly supported in related work.

## Compile
NeuroSpector is implemented in C++. It only requires a g++ compiler to build and does not depend on any other libraries or external tools to run. To build NeuroSpector, type `make` in a terminal.

	$ make

If the gcc compiler is not installed, type the following command in a terminal.

    $ sudo apt install build-essential

## Run
### Optimizer
NeuroSpector has two different execution modes, `optimizer` and `analyzer`. The `optimizer` mode triggers a scheduling optimization algorithm to find an optimal scheduling scheme (i.e., dataflow and mapping) for the given accelerator and neural layer configurations. The following run command executes NeuroSpector in the `optimizer` mode.

	$ ./neurospector --run_type=optimizer 
			 --accelerator=<accelerator input> 
			 --network=<DNN input> 
			 --dataflow=<'fixed' or 'flexible'> 
			 --optimizer=<'bottom-up' or 'brute-force'>
			 --metric=<'energy' or 'cycle'> 
			 --layer=<targeted DNN layer>

For example, the following command triggers NeuroSpector to find the most energy-efficient scheduling schemes for ResNet-50 layers on Eyeriss with fixed dataflows specified in the accelerator configuration file (i.e., `eyeriss.cfg`), using the NeuroSpector's signature optimization strategy named `bottom-up`.

	$ ./neurospector --run_type=optimizer\
			 --accelerator=configs/accelerators/eyeriss.cfg\
			 --network=configs/networks/resnet50.cfg\
			 --dataflow=fixed\
			 --optimizer=bottom-up\
			 --metric=energy

### Analyzer
The `analyzer` mode of NeuroSpector simply evaluates the energy and cycle costs of an accelerator using a scheduling scheme provided in the `--scheduling_table` option of the run command. A combinatorial case of dataflow and mapping in an accelerator is represented via a scheduling table in NeuroSpector. The following command invokes NeuroSpector to calculate the cost metrics of Eyeriss for ResNet-50 layers based on the given scheduling option.

	$ ./neurospector --run_type=analyzer \
			 --accelerator=configs/accelerators/eyeriss.cfg \
			 --network=configs/networks/resnet50.cfg \
			 --scheduling_table=configs/scheduling_tables/eyeriss_sample.cfg


## Download
The latest release of the NeuroSpector framework is v1.4.1. The NeuroSpector framework will continually be updated for better code structuring, readability, usability, and bug fixes. To obtain the latest version of NeuroSpector, use the following git command in a terminal. 

	$ git clone --branch v1.4.1 https://github.com/yonsei-icsl/NeuroSpector

Or, if you wish to use the latest development version, simply clone the git repository as is.

	$ git clone https://github.com/yonsei-icsl/NeuroSpector

## Reference
To reference NeuroSpector, please use our TPDS paper.

	@article{park_tpds2023,
	    author  = {C. Park and and B. Kim and S. Ryu and W. Song},
	    title   = {{NeuroSpector: Systematic Optimization of Dataflow Scheduling in Deep Neural Network Accelerators}},
	    journal = {IEEE Transactions on Parallel and Distributed Systems},
	    volume  = {34},
	    number  = {8},
	    month   = {Aug.},
	    year    = {2023},
	}

For troubleshooting, bug reports, or any questions regarding the use of NeuroSpector, please contact Chanho Park via email: ch.park {\at} yonsei {\dot} ac {\dot} kr, or visit our lab webpage: https://icsl.yonsei.ac.kr.
