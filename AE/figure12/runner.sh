#!/bin/zsh

# first setup DSA, init hugepages and disable numa balancing
echo 20480 > /proc/sys/vm/nr_hugepages
echo 0 | sudo tee /proc/sys/kernel/numa_balancing
echo 3 > /proc/sys/vm/drop_caches
../../scripts/setup_dsa.sh -d dsa0
../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# Use the naive configuration to generate naiveDSA performance
# It set numa_alloc in code, so we do not use numactl membind here
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure12

# running CPU shuffle
echo "Running CPU shuffle ..."
sleep 3
echo 3 > /proc/sys/vm/drop_caches
numactl -C0-47 ../../build/expr/paper/chapter5_1_shuffle/shuffle_dsa 0 > CPU.txt
 
echo "Running naiveDSA ..."
numactl -C0-47 ../../build/expr/paper/chapter5_1_shuffle/shuffle_dsa 1 > naiveDSA.txt

# Use the LightDSA configuration to generate LightDSA performance
cp dsa_conf_LightDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure12
echo "Running LightDSA ..."
numactl -C0-47 ../../build/expr/paper/chapter5_1_shuffle/shuffle_dsa 1 > LightDSA.txt

# extract data
awk -F'speed = ' '/shuffle_data/ {split($2, a, "GB/s"); print a[1]}' CPU.txt > cpu.txt 
awk -F'speed = ' '/shuffle_data/ {split($2, a, "GB/s"); print a[1]}' LightDSA.txt > our.txt 
awk -F'speed = ' '/shuffle_data/ {split($2, a, "GB/s"); print a[1]}' naiveDSA.txt > naive.txt
paste cpu.txt naive.txt our.txt > data.txt
rm cpu.txt naive.txt our.txt
 
# draw figure12
python3 figure12.py
echo "Figure 12 done!"