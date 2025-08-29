#!/bin/zsh
set -e

# first setup DSA, init hugepages and disable numa balancing
echo 20480 > /proc/sys/vm/nr_hugepages
echo 0 | sudo tee /proc/sys/kernel/numa_balancing
echo 3 > /proc/sys/vm/drop_caches
../../scripts/setup_dsa.sh -d dsa0
../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# "dsa_conf_naiveDSA.hpp" DISABLE all optimization in LightDSA.
# To achieve ATS miss and no fault, Cache Control should be enabled. 
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure5
echo "Running page fault speed..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_fault_memmove_speed > all_results.txt
 
awk '/method = major PF/ {getline; getline; print $(NF-4)}' all_results.txt > majorPF.txt
awk '/method = minor PF/ {getline; getline; print $(NF-4)}' all_results.txt > minorPF.txt
awk '/method = ATS miss/ {getline; getline; print $(NF-4)}' all_results.txt > ATSmiss.txt
awk '/method = no PF/ {getline; getline; print $(NF-4)}' all_results.txt > noPF.txt
paste majorPF.txt minorPF.txt ATSmiss.txt noPF.txt > pf_type_data.txt

rm majorPF.txt minorPF.txt ATSmiss.txt noPF.txt

# draw figure5
python3 figure5.py
echo "Figure 5 done!"