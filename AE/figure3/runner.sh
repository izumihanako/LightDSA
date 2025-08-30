#!/bin/zsh
set -e

# Setup DSA
sudo ../../scripts/setup_dsa.sh -d dsa0
sudo ../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# "dsa_conf_naiveDSA.hpp" DISABLE all optimization in LightDSA.
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure3

# Figure3 (a) access memory in sequential order
sleep 2
echo "Figure 3 (a) Sequential Access Pattern"
echo "running memmove sequential..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 0 0 > memmove_sequential.txt # 0: memmove, 0: sequential

sleep 2
echo "running memfill sequential..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 1 0 > memfill_sequential.txt # 1: memfill, 0: sequential

sleep 2
echo "running compare sequential..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 2 0 > compare_sequential.txt # 2: compare, 0: sequential

sleep 2
echo "running compval sequential..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 3 0 > compval_sequential.txt # 3: compval, 0: sequential

# extract data
for f in memmove_sequential.txt memfill_sequential.txt compare_sequential.txt compval_sequential.txt; do
    awk -F'do_speed = ' '/^transfer_size/ {print $2}' "$f" | awk '{print $1}' > "${f%.txt}_dsa.txt"
    awk -F'do_speed = ' '/^CPU: transfer_size/ {print $2}' "$f" | awk '{print $1}' > "${f%.txt}_cpu.txt"
done
paste memmove_sequential_dsa.txt memfill_sequential_dsa.txt compare_sequential_dsa.txt compval_sequential_dsa.txt > dsa_data.txt
paste memmove_sequential_cpu.txt memfill_sequential_cpu.txt compare_sequential_cpu.txt compval_sequential_cpu.txt > cpu_data.txt
rm *_dsa.txt *_cpu.txt
# draw figure3 (a)
python3 figure3.py figure3a
echo "Figure 3 (a) done!"


# Figure3 (b) access memory in random order
echo "Figure 3 (b) Random Access Pattern"

sleep 2
echo "running memmove random..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 0 1 > memmove_random.txt # 0: memmove, 1: random

sleep 2
echo "running memfill random..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 1 1 > memfill_random.txt # 1: memfill, 1: random

sleep 2
echo "running compare random..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 2 1 > compare_random.txt # 2: compare, 1: random

sleep 2
echo "running compval random..."
sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter2_3_DSAperf/DSA_speed 3 1 > compval_random.txt # 3: compval, 1: random
# extract data
for f in memmove_random.txt memfill_random.txt compare_random.txt compval_random.txt; do
    awk -F'do_speed = ' '/^transfer_size/ {print $2}' "$f" | awk '{print $1}' > "${f%.txt}_dsa.txt"
    awk -F'do_speed = ' '/^CPU: transfer_size/ {print $2}' "$f" | awk '{print $1}' > "${f%.txt}_cpu.txt"
done
paste memmove_random_dsa.txt memfill_random_dsa.txt compare_random_dsa.txt compval_random_dsa.txt > dsa_data.txt
paste memmove_random_cpu.txt memfill_random_cpu.txt compare_random_cpu.txt compval_random_cpu.txt > cpu_data.txt
rm *_dsa.txt *_cpu.txt
# draw figure3 (b)
python3 figure3.py figure3b
echo "Figure 3 (b) done!"

echo "Done!"