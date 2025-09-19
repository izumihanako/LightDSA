#!/bin/zsh
set -e

# Setup DSA
sudo ../../scripts/setup_dsa.sh -d dsa0
sudo ../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 1 -f 1

# Use the configuration with DRDBK
cp dsa_conf_DRDBK.hpp ../../src/details/dsa_conf.hpp
cd ../../build
make -j8 > /dev/null
cd ../AE/figure15

# Then for mix granularity from 1 to 16, run the test
echo "Running tests with DRDBK ..."
rm -f with_DRDBK.txt
downlim=0
uplim=16
for i in {$downlim..$uplim}
do
    for j in {$downlim..$uplim}
    do
        echo "Running mix $i small with $j big ..." 
        sudo numactl -C0 --membind=0 ../../build/expr/paper/chapter3_3_PIPE/pipeline_mc_plus_inbatch 0 100000 1 1 $i $j >> with_DRDBK.txt
    done
done

# extract speeds in with_DRDBK.txt 
grep -o "speed [0-9.]*MB/s" with_DRDBK.txt | cut -d' ' -f2 | cut -d'M' -f1 > temp.txt
awk -v uplim=$((uplim-downlim+1)) '{
  printf "%s", $0
  if (NR % uplim == 0) 
    printf "\n"
  else 
    printf ","
}' temp.txt > with_DRDBK_data.txt
rm temp.txt
 
# draw figure15
python3 figure15.py
echo "Done!"