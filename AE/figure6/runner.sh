#!/bin/zsh

# first setup DSA, init hugepages and disable numa balancing
echo 20480 > /proc/sys/vm/nr_hugepages
echo 0 | sudo tee /proc/sys/kernel/numa_balancing
../../scripts/setup_dsa.sh -d dsa0
../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# First, we get the speed of PRS
cp dsa_conf_withPRS.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure6
echo "Running RPS..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_tapping 0 0 > PRS.txt


# Then, disable PRS and do other tests
cp dsa_conf_noPRS.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure6
echo "Running no PRS..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_tapping 0 0 > no_PRS.txt

echo "Running tap all..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_tapping 1 0 > tap_all.txt

echo "Running tap demand..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_tapping 2 0 > tap_demand.txt

echo "Running tap only..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_tapping 3 0 > tap_only.txt

echo "Running tap demand with large page..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_tapping 4 0 > tap_demand_large.txt

echo "Running tap only with large page..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_tapping 5 0 > tap_only_large.txt

for f in PRS.txt no_PRS.txt tap_all.txt tap_only.txt tap_demand.txt tap_only_large.txt tap_demand_large.txt; do
    awk -F'average speed: ' '/average speed:/ {print $2}' "$f" | awk '{print $1}' > "${f%.txt}_col.txt"
done 
paste PRS_col.txt no_PRS_col.txt tap_all_col.txt tap_demand_col.txt tap_only_col.txt tap_demand_large_col.txt tap_only_large_col.txt > data.txt
rm *_col.txt

# draw figure6
python3 figure6.py
echo "Figure 6 done!"