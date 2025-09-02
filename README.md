# LightDSA: Enabling Efficient DSA Through Hardware-Aware Transparent Optimization

LightDSA is a user-friendly, asynchronous DSA library featuring low-overhead, run-to-completion optimizations. It is designed to eliminate the negative performance impact of DSA features, enabling DSA to run efficiently under any workload.

More information about LightDSA can be found in our EuroSys '26 paper.

## Overview
The key directories of this project and their contents are as follows:
```bash
LightDSA 
├── AE              # Scripts for artifact evaluation of the paper
│   ├── figure1         # Scripts for reproduce Figure 1
│   ├── figure3         # Scripts for reproduce Figure 3
│   ├── ...
│   └── ATCexplore      # Scripts for reproduce ATC structure exploration
├── example         # Simple example programs using LightDSA
├── expr            # Source code for the paper's experiments
├── Makefile        # Builds both static and shared libraries
├── scripts         # Utility scripts
├── src             # LightDSA source code
├── build.sh        # Script for building LightDSA
└── prerequisite.sh # Script for installing dependencies
```

## Dependencies 

To reproduce the experiments on a custom machine, ensure the following requirements are met:

### Hardware Dependencies

- Intel Xeon CPU, 4th gen or higher (only these CPUs integrate the DSA accelerator).

### Software Dependencies

- Linux Kernel version ≥ 5.19 (Ubuntu 20.04.6 LTS with Linux 6.6.58 on the provided server)
- Python 3.10.12 (most of the python libraries are for AE scripts)
  - Required Python libraries: brokenaxes, datasets, huggingface_hub, matplotlib, numpy, pandas, redis, tqdm
- `idxd-config` from [Intel repository](https://github.com/intel/idxd-config)

- libnuma, libpmem (available from package manager)

- CMake version ≥ 3.16 (CMake 3.16.3 on the provided server)
- Any C++ compiler supporting CXX14 (g++ 11.4.0 on the provided server)


## Build & Reproduce

First, install the required dependencies by running:
```bash
./prerequisite.sh
```
This script installs `python3-pip`, `libnuma`, `libpmem`, and `CMake` via the package manager, installs the required Python libraries via `pip`, and clones and installs the **idxd-config** from Intel's official repository.

To build LightDSA, run: 
```bash
./build.sh
```
We also provide a end-to-end script to reproduce all experiments:
```bash
cd AE && ./reproduce.sh 
```
The figures for each experiment will be generated in their corresponding figureX directories (X = 1, 3, 4, 5, 6, 7, 8, 9, 11, 12).
If you prefer to run a specific experiment, each directory contains a `runner.sh` script to reproduce the experiment and generate the figure. For example, to reproduce Figure 1:
```bash
cd AE/figure1 && ./runner.sh
```
This will generate `figure1.pdf` in the same directory, corresponding to Figure 1 in the paper.

For more details on reproduction, see the [README.md](https://github.com/izumihanako/LightDSA-AE/blob/master/README.md) in the LightDSA-AE repository.

## As a Library

After building, a shared library `liblightDSA.so` will be available in the `build` directory. 

If you prefer to use the Makefile directly, we also provide a `Makefile` in the project root directory. Simply run:
```bash
make
```
and you will find both `liblightDSA.so` and `liblightDSA.a` in the `lib` directory.

## Configuration of LightDSA

The configuration of LightDSA is located in the header file src/details/dsa_conf.hpp. 
All configurable options are defined as macros, with names and comments consistent with those in the paper. 
You can disable an optimization or option by commenting the corresponding line. 
For example, consider the following two lines in the configuration:
```c++
#define DESCS_OUT_OF_ORDER_RECYCLE_ENABLE        /*** use Out-of-Order recycle ***/  
constexpr int OUT_OF_ORDER_RECYCLE_T_INIT = 25 ; /*** T_init value ***/
```
If the first line is commented, the out-of-order recycle optimization is disabled.
The second line sets $t_{init}$ to 25. However, if the first line is commented, this definition has no effect.
For details on out-of-order recycle and the meaning of $t_{init}$, please refer to Section 4.5 of the LightDSA paper.

### Why Determine All Configurations at Compile Time?
LightDSA determines all configurations at compile time rather than at run time. This maximizes performance and simplifies the API. Each descriptor contains a `flags` field whose value depends on the configuration. If configurations are determined at run time, the API would need extra knobs, and every descriptor initialization would perform extra computations to determine the `flags` value. This extra computation overhead is especially noticeable for short-running operations. By computing the `flags` values at compile time, LightDSA reduces the initialization overhead of the `flags` field to a single constant assignment, thereby ensuring minimal overhead and simplifying the API.


## API and Code Examples


<!-- Figure1 109.60s user 14.68s system 153% cpu 1:20.86 total  -->
<!-- Figure3 2814.38s user 46.42s system 100% cpu 47:26.92 total -->
<!-- Figure4 58.98s user 14.48s system 248% cpu 29.620 total  -->
<!-- Figure5 99.32s user 81.20s system 30% cpu 9:59.10 total -->
<!-- Figure6 303.13s user 204.53s system 106% cpu 7:57.70 total -->
<!-- Figure7 57.28s user 23.54s system 219% cpu 36.769 total -->
<!-- Figure8 109.64s user 11.92s system 123% cpu 1:38.46 total -->
<!-- Figure9 70.14s user 17.33s system 140% cpu 1:02.24 total -->
<!-- Fugure11 488.82s user 253.78s system 103% cpu 11:54.96 total -->
<!-- Figure12 7m48.626s user 4m8.417s sys 11m30.628s real -->
<!-- Figure13 ./env_init.sh  294.61s user 54.40s system 73% cpu 7:55.88 total
              ./runner.sh  387.71s user 146.58s system 172% cpu 5:09.75 total -->