#!/bin/zsh

# first setup DSA, init hugepages and disable numa balancing
echo 20480 > /proc/sys/vm/nr_hugepages
echo 0 | sudo tee /proc/sys/kernel/numa_balancing
../../scripts/setup_dsa.sh -d dsa0
../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 1 -f 1

# Use the configuration with DRDBK
cp dsa_conf_DRDBK.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure7

# Then for mix granularity from 1 to 16, run the test
echo "Running tests with DRDBK ..."
rm -f with_DRDBK.txt
for i in {1..16}
do
    echo "Running mix granularity $i ..." 
    numactl -C0 --membind=0 ../../build/expr/paper/chapter3_3_PIPE/pipeline_mc 0 100000 1 1 $((2 * i)) >> with_DRDBK.txt
done

# Use the configuration without DRDBK
cp dsa_conf_noDRDBK.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure7

# Then for mix granularity from 1 to 16, run the test
echo "Running tests without DRDBK ..."
rm -f no_DRDBK.txt
for i in {1..16}
do
    echo "Running mix granularity $i ..." 
    numactl -C0 --membind=0 ../../build/expr/paper/chapter3_3_PIPE/pipeline_mc 0 100000 1 1 $((2 * i)) >> no_DRDBK.txt
done

awk -F'speed ' '/; DSA_batch/{print $2}' with_DRDBK.txt | awk -F'MB/s' '{print $1}' | awk '{$1=$1};1' > with_DRDBK_data.txt
awk -F'speed ' '/; DSA_batch/{print $2}' no_DRDBK.txt | awk -F'MB/s' '{print $1}' | awk '{$1=$1};1' > no_DRDBK_data.txt

# draw figure7
python3 figure7.py
echo "Figure 7 done!"