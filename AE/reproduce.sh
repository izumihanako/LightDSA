#!/bin/bash
set -e

cd ..
./build.sh 
cd AE
for i in 1 3 4 5 6 7 8 9 12 13 15
do
  (cd ./figure$i && ./runner.sh)
done
