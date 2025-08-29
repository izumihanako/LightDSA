#!/bin/zsh
set -e

# first setup DSA, init hugepages and disable numa balancing
echo 20480 > /proc/sys/vm/nr_hugepages
echo 0 | sudo tee /proc/sys/kernel/numa_balancing
../../scripts/setup_dsa.sh -d dsa0
../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# Get naive DSA performance, "dsa_conf_naiveDSA.hpp" DISABLE all optimization in LightDSA.
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure1
echo "Running naiveDSA test..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter1_Motivation/motivation_speed_test 1 1 > naiveDSA.txt

# Get LightDSA performance, "dsa_conf_LightDSA.hpp" ENABLE all optimization in LightDSA.
cp dsa_conf_LightDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure1
echo "Running LightDSA test..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter1_Motivation/motivation_speed_test 1 1 > LightDSA.txt

# Finally, we get CPU performance
echo "Running CPU test..."
numactl -C0 --membind=0 ../../build/expr/paper/chapter1_Motivation/motivation_speed_test 1 0 > CPU.txt

# Extract do_speed values and save to output.txt
echo "Extracting do_speed values..."
awk -F'do_speed = ' '/do_speed/ {print $2}' CPU.txt      | awk '{print $1}' > cpu_col.txt
awk -F'do_speed = ' '/do_speed/ {print $2}' naiveDSA.txt | awk '{print $1}' > naive_col.txt
awk -F'do_speed = ' '/do_speed/ {print $2}' LightDSA.txt | awk '{print $1}' > light_col.txt 
paste cpu_col.txt naive_col.txt light_col.txt > data.txt 
rm cpu_col.txt naive_col.txt light_col.txt

# Draw figure1
echo "Drawing figure1..."
python3 figure1.py

echo "Done!"