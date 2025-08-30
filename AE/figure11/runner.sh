#!/bin/zsh

set -e
# Setup DSA
sudo ../../scripts/setup_dsa.sh -d dsa0
sudo ../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# Use the naive configuration to generate naiveDSA performance
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure11
echo "Running naiveDSA ..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter5_1_vector/vector_dsa > naiveDSA.txt

# Use the LightDSA configuration to generate +LightDSA and +Insighted Design performance
cp dsa_conf_LightDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure11
echo "Running +LightDSA and +Insight Design..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter5_1_vector/vector_dsa > LightDSA.txt

# extract "nve_dsa_vector" from naiveDSA.txt 
awk '/nve_dsa_vector/ {print $6}' naiveDSA.txt | sed 's/s,//' > naive_dsa.txt

# extract "std_vector" "nve_dsa_vector" "my_dsa_vector" from LightDSA.txt
awk '/std_vector/ {print $6}' LightDSA.txt | sed 's/s,//' > std_vector.txt
awk '/nve_dsa_vector/ {print $6}' LightDSA.txt | sed 's/s,//' > nve_dsa_vector.txt
awk '/my_dsa_vector/ {print $6}' LightDSA.txt | sed 's/s,//' > my_dsa_vector.txt

paste std_vector.txt naive_dsa.txt nve_dsa_vector.txt my_dsa_vector.txt > data.txt
rm naive_dsa.txt std_vector.txt nve_dsa_vector.txt my_dsa_vector.txt
 
# draw figure11
python3 figure11.py
echo "Done!"