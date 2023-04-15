#!/bin/sh  -xe
cd $(dirname $(readlink -fn $0))

#
# Actually build the mingw artifacts inside the Fedora container
#
set -xe

cd $TOPDIR

sudo dnf install -y cmake
sudo dnf install -y gcc-c++
sudo dnf install -y flatpak-builder
sudo dnf install -y flatpak
sudo dnf install -y make
sudo dnf install -y tar

sudo dnf install -y ntpsec
sudo /usr/sbin/ntpdate se.pool.ntp.org
sudo dnf update -y ca-certificates
flatpak remote-add --user  flathub https://flathub.org/repo/flathub.flatpakrepo
#flatpak install --user  -y \
#        http://opencpn.duckdns.org/opencpn/opencpn.flatpakref  >/dev/null
flatpak install --user -y  flathub org.freedesktop.Sdk//22.08  >/dev/null
flatpak install --user -y  flathub org.freedesktop.Platform/x86_64/22.08  >/dev/null
flatpak install --user -y  https://flathub.org/repo/appstream/org.opencpn.OpenCPN.flatpakref >/dev/null
rm -rf build && mkdir build && cd build
git config --global --add safe.directory /root/project
cmake -DOCPN_FLATPAK=ON ..
make flatpak-build
make package
