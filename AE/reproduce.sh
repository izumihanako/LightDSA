#!/bin/bash
set -e

cd ..
./build.sh 
cd AE
for i in 1 3 4 5 6 7 8 9 11 12
do
  (cd ./figure$i && ./runner.sh)
done
