#!/bin/bash
# This script installs all the prerequisites for running the experiments in this repository.
set -e 

# Install g++
if command -v g++ >/dev/null 2>&1; then
    echo "g++ is installed"
else
    echo "g++ is NOT installed, now installing ..."
    sudo apt install g++ -y
fi

# check if accel-config exists
if command -v accel-config >/dev/null 2>&1; then
    echo "accel-config is installed"
else # Install idxd-config
    echo "accel-config is NOT installed, now installing ..."
    sleep 5
    sudo apt install build-essential -y
    sudo apt install autoconf automake autotools-dev libtool pkgconf asciidoc xmlto -y
    sudo apt install uuid-dev libjson-c-dev libkeyutils-dev libz-dev libssl-dev -y
    sudo apt install debhelper devscripts debmake quilt fakeroot lintian asciidoctor -y
    sudo apt install file gnupg patch patchutils -y

    git clone git@github.com:intel/idxd-config.git
    cd idxd-config
    ./autogen.sh
    ./configure CFLAGS='-g -O2' --prefix=/usr --sysconfdir=/etc \
        --libdir=/usr/lib64 --enable-test=yes --enable-asciidoctor
    sudo make
    # This step is sometimes unreliable due to bugs in the check scripts.
    # In addition, running it partially may write incorrect configurations  
    # to the DSA device, causing it to malfunction (a system reboot can  
    # resolve the issue).  
    # Therefore, we skipped this step by default. You may try it at your  
    # own discretion if needed. 
    # ------------------------
    # sudo make check 
    # ------------------------
    sudo make install
fi

# Install pip
if command -v python3 >/dev/null 2>&1; then
    echo "Python 3 is installed"
else
    echo "Python 3 is NOT installed, now installing ..."
    sudo apt install python3 -y
fi 

# Install python libraries
pkgs=(matplotlib numpy pandas scipy brokenaxes datasets huggingface_hub redis tqdm)
missing=()
for p in "${pkgs[@]}"; do
  if python3 -m pip show "$p" >/dev/null 2>&1; then
    echo "OK   : $p"
  else
    echo "MISS : $p"
    missing+=("$p")
  fi
done

if ((${#missing[@]})); then
  echo "Installing missing: ${missing[*]}"
  python3 -m pip install --upgrade "${missing[@]}"
else
  echo "All required packages are installed."
fi

# Install libnuma and libpmem
need_pkgs=(libnuma-dev libpmem-dev)
missing=()
for p in "${need_pkgs[@]}"; do
  dpkg -s "$p" >/dev/null 2>&1 || missing+=("$p")
done
if ((${#missing[@]})); then
  echo "Installing: ${missing[*]}"
  sudo apt-get update -y
  sudo apt-get install -y "${missing[@]}"
else
  echo "All required dev packages are installed."
fi

# Install CMake
if command -v cmake >/dev/null 2>&1; then
    echo "CMake is installed"
else
    echo "CMake is NOT installed, now installing ..."
    sudo apt install cmake -y
fi