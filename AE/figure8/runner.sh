#!/bin/zsh

set -e
# first setup DSA, init hugepages and disable numa balancing
echo 20480 > /proc/sys/vm/nr_hugepages
echo 0 | sudo tee /proc/sys/kernel/numa_balancing
echo 3 > /proc/sys/vm/drop_caches
../../scripts/setup_dsa.sh -d dsa0
../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 1 -f 1

# Use the naive configuration  
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure8

numactl -C0 --membind=0 ../../build/expr/paper/chapter3_4_ALIGN/alignment_speed 0 0 50000 > batch_memcpy.txt # 0: DSA_batch, 0: memcpy
sleep 2
numactl -C0 --membind=0 ../../build/expr/paper/chapter3_4_ALIGN/alignment_speed 1 0 50000 > sg64_memcpy.txt # 1: DSA_memcpy*32, 0: memcpy

# extract the do_speed values for different alignments and submitting methods
aligns=(16 32 64)
files=(sg64_memcpy.txt batch_memcpy.txt)
for f in "${files[@]}"; do
    for a in "${aligns[@]}"; do 
        # extract the do_speed values for alignment = a
        awk -v a="$a" '
            $0 ~ "alignment = "a" bytes" {flag=1; next}
            flag && $0 ~ /transfer_size/ {print $0}
            flag && $0 ~ /^method =/ {flag=0}
        ' "$f" | awk -F'do_speed = ' '{print $2}' | awk '{print $1}' > "${f%.txt}_${a}B.txt"
    done
done

# merge ( 16B/32B/64B non-batch, 16B/32B/64B batch )
paste sg64_memcpy_16B.txt sg64_memcpy_32B.txt sg64_memcpy_64B.txt \
      batch_memcpy_16B.txt batch_memcpy_32B.txt batch_memcpy_64B.txt > data.txt
rm sg64_memcpy_*B.txt batch_memcpy_*B.txt

# draw figure8
python3 figure8.py
echo "Figure 8 done!"