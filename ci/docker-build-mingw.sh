#!/bin/sh  -xe

#
# Actually build the Travis mingw artifacts inside teh Fedora container
#
set -xe

if [-n "$TRAVIS" ]; then
    cd /opencpn-ci
elif [ -n "$CIRCLECI" ]; then
    cd /root/project
fi

su -c "dnf install -y sudo dnf-plugins-core"
sudo dnf copr enable -y --nogpgcheck leamas/opencpn-mingw 
sudo dnf builddep  -y /opencpn-ci/ci/opencpn-deps.spec
rm -rf build; mkdir build; cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/opencpn-ci/ci/toolchain.cmake ..
make -j2
make package
