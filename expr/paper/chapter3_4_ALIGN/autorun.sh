#!/bin/bash
project_path="/root/DSA/dsa_memcpy"

file_path="${project_path}/build/expr/paper/chapter3_4_ALIGN/alignment_speed"
output_path="${project_path}/expr/paper/chapter3_4_ALIGN/data" 
# eval "rm -rf $output_path/*" 


for (( method = 0 ; method <= 2 ; method++ )) # 0 is DSA_batch, 1 is DSA_memcpy*32, 2 is DSA_single
do 
    for (( op = 0 ; op <= 3 ; op ++ )) # 0 is memcpy, 1 is memfill, 2 is compare, 3 is compval
    do
        desc_cnt=50000 
        method_name=$( ( [ $method -eq 0 ] && echo "batch" ) || ( [ $method -eq 1 ] && echo "sg64" ) || echo "single" )
        op_name=$( ( [ $op -eq 0 ] && echo "memcpy" ) || ( [ $op -eq 1 ] && echo "memfill" ) || ( [ $op -eq 2 ] && echo "compare" ) || echo "compval" )
        echo "Method: $method_name,  Operation: $op_name"
  
        output_file="${output_path}/${method_name}_${op_name}.data"
        touch $output_file  
        echo "Output file: $output_file" 
        command="numactl -C0 --membind=0 $file_path $method $op $desc_cnt >> $output_file"
        echo $command
        eval $command 
        # read -n 1 -s -r -p $'Press any key to continue...\n' 
    done
done