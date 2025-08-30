#!/bin/zsh
set -e

# Setup DSA
sudo ../../scripts/setup_dsa.sh -d dsa0
sudo ../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# "dsa_conf_naiveDSA.hpp" DISABLE all optimization in LightDSA.
# To achieve ATS miss and no fault, Cache Control should be enabled. 
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure5
echo "Running page fault speed..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter3_2_PF/page_fault_memmove_speed > all_results.txt
 
awk '/method = major PF/ {getline; getline; print $(NF-4)}' all_results.txt > majorPF.txt
awk '/method = minor PF/ {getline; getline; print $(NF-4)}' all_results.txt > minorPF.txt
awk '/method = ATS miss/ {getline; getline; print $(NF-4)}' all_results.txt > ATSmiss.txt
awk '/method = no PF/ {getline; getline; print $(NF-4)}' all_results.txt > noPF.txt
paste majorPF.txt minorPF.txt ATSmiss.txt noPF.txt > pf_type_data.txt

rm majorPF.txt minorPF.txt ATSmiss.txt noPF.txt
# recover hugepages setting
echo 20480 | sudo tee /proc/sys/vm/nr_hugepages >/dev/null


# draw figure5
python3 figure5.py
echo "Done!"