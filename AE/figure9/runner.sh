#!/bin/zsh

set -e
# first setup DSA, init hugepages and disable numa balancing
echo 20480 > /proc/sys/vm/nr_hugepages
echo 0 | sudo tee /proc/sys/kernel/numa_balancing
echo 3 > /proc/sys/vm/drop_caches
../../scripts/setup_dsa.sh -d dsa0
../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 4 -f 1

# Use the naive configuration  
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure9

numactl -C0 --membind=0 ../../build/expr/paper/chapter3_5_OOO/ooo_test > ooo_test.txt

ops=(MEMMOVE MEMFILL COMPARE COMPVAL)

for op in "${ops[@]}"; do
    awk -v op="$op" '
        $0 ~ "DSA_batch , op_type = "op {flag=1; next}
        flag && $0 ~ /Average out-of-order completed batch:/ {
            print $NF
        }
        flag && $0 ~ /^DSA_batch , op_type =/ {flag=0}
    ' ooo_test.txt > "${op}_batch.txt"
done
paste MEMMOVE_batch.txt MEMFILL_batch.txt COMPARE_batch.txt COMPVAL_batch.txt > data.txt
rm *_batch.txt

# draw figure9
python3 figure9.py
echo "Figure 9 done!"