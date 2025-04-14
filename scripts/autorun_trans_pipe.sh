#!/bin/bash
file_path="./expr/pipeline/trans_pipe"
output_path="../output/trans_pipe_16K_512/" 
eval "rm -rf $output_path/*"
groups=1000

for (( method = 0 ; method <= 1 ; method++ )) # 0: dsa_batch, 1: dsa_single
do 
    for (( is_divide = 0 ; is_divide <= 1 ; is_divide++ )) # 0: mixed, 1: divide
    do
        for (( desc_per_group = 2 ; desc_per_group <= 32 ; desc_per_group *= 2 ))
        do 
            step=$( [ $desc_per_group -gt 10 ] && echo 2 || echo 1  )
            for (( small = 0 ; small <= $desc_per_group ; small+=step ))
            do
                method_name=$( [ $method -eq 0 ] && echo "dsa_batch" || echo "dsa_single" )
                method_name=$( [ $is_divide -eq 0 ] && echo "${method_name}_mixed" || echo "${method_name}_divide" )  
                big=$(( $desc_per_group - $small ))
                output_file="${output_path}/${method_name}_descs$( printf "%02d" $desc_per_group ).txt"
                touch $output_file 
                echo "Method: $method_name, Groups: $groups, Small: $small, Big: $big"
                echo "Output file: $output_file" 
                command="python3 perf_PIPE.py \"numactl -C0 --membind=0 $file_path $method $groups $small $big $is_divide\" >> $output_file"
                echo $command
                eval $command
                eval "echo -e \"\n\n\" >> $output_file"
                # read -n 1 -s -r -p $'Press any key to continue...\n'
            done 
        done 
    done
done