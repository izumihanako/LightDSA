#!/bin/zsh
set -e

# Setup DSA
sudo ../../scripts/setup_dsa.sh -d dsa0
sudo ../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 1 -f 1

# "dsa_conf_naiveDSA.hpp" DISABLE all optimization in LightDSA, same as Figure 1
# Get naive DSA performance
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure4
echo "Running naiveDSA test..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter1_Motivation/motivation 1 > naiveDSA.txt # 1: DSA, 0: CPU

# Get LightDSA performance
cp dsa_conf_LightDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure4
echo "Running LightDSA test..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter1_Motivation/motivation 1 > LightDSA.txt # 1: DSA, 0: CPU

# Finally, we get CPU performance
echo "Running CPU test..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter1_Motivation/motivation 0 > CPU.txt # 1: DSA, 0: CPU

# Extract do_speed values and save to output.txt
echo "Extracting do_speed values..."
awk -F'do_speed = ' '/do_speed/ {print $2}' CPU.txt      | awk '{print $1}' > cpu_col.txt
awk -F'do_speed = ' '/do_speed/ {print $2}' naiveDSA.txt | awk '{print $1}' > naive_col.txt
awk -F'do_speed = ' '/do_speed/ {print $2}' LightDSA.txt | awk '{print $1}' > light_col.txt 
paste cpu_col.txt naive_col.txt light_col.txt > data.txt 
rm cpu_col.txt naive_col.txt light_col.txt

# Draw figure4
echo "Drawing figure4..."
python3 figure4.py

echo "Done!"