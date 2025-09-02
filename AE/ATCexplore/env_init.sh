#!/bin/zsh
set -e

# Setup DSA with only one engine to explore the ATC structure
sudo ../../scripts/setup_dsa.sh -d dsa0
sudo ../../scripts/setup_dsa.sh -d dsa0 -w 1 -m s -e 1 -f 1

# Get naive DSA performance, "dsa_conf_naiveDSA.hpp" DISABLE all optimization in LightDSA.
cp dsa_conf_naiveDSA.hpp ../../src/details/dsa_conf.hpp
cd ../..
rm -rf build
mkdir build
cd build
cmake ..
make -j8
echo "Done!"