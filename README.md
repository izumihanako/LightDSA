# LightDSA: Enabling Efficient DSA Through Hardware-Aware Transparent Optimization

LightDSA is a user-friendly, asynchronous DSA library featuring low-overhead, run-to-completion optimizations. It is designed to eliminate the negative performance impact of DSA features, enabling DSA to run efficiently under any workload.

## Overview
The key directories of this project and their contents are as follows:
```
LightDSA 
├── AE              # Scripts for artifact evaluation of the paper
│   ├── figure1         # Scripts for reproduce Figure 1
│   ├── figure2         # Scripts for reproduce Figure 2
│   ├── ...
├── example         # Simple example programs using LightDSA
├── expr            # Source code for the paper's experiments
├── Makefile        # Builds both static and shared libraries
├── scripts         # Utility scripts
├── src             # LightDSA source code
├── build.sh        # Script for building LightDSA
└── prerequisite.sh # Script for installing dependencies
```

## Dependencies 

All hardware and software dependencies are pre-configured on the provided server. To reproduce the experiments on a custom machine, ensure the following requirements are met:

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


## Build

First, install the required dependencies by running:
```
./prerequisite.sh
```
This script installs `python3-pip`, `libnuma`, `libpmem`, and `CMake` via the package manager, installs the required Python libraries via `pip`, and clones and installs the **idxd-config** from Intel's official repository.

To build LightDSA, run: 
```
./build.sh
```
This will create a `build` directory, from which all experiments can be executed.
For example, to reproduce an experiment such as Figure 1, go to the corresponding directory and run `runner.sh`:
```
cd AE/figure1 && ./runner.sh
```
This will generate `figure1.pdf` in the same directory, corresponding to Figure 1 in the paper.

After building, a shared library `liblightDSA.so` will be available in the `build` directory. 
If you prefer to use the Makefile directly, we also provide one in the project root. Simply run:
```
make
```
and you will find both `liblightDSA.so` and `liblightDSA.a` in the `lib` directory.

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