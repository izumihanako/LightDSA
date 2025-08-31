# prepare LightDSA cmake and Makefile
cd ..
rm -rf build
mkdir build
cd build
cmake ..
make 
cd ../AE
echo "Done!"