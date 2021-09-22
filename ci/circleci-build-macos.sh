#!/usr/bin/env bash

#
# Build the OSX artifacts
#

# bailout on errors and echo commands
set -xe
set -o pipefail

#git -C /usr/local/Homebrew/Library/Taps/homebrew/homebrew-core fetch --unshallow
#git -C /usr/local/Homebrew/Library/Taps/homebrew/homebrew-cask fetch --unshallow
#brew update-reset

#
# Check if the cache is with us. If not, re-install brew.
brew list --versions libexif || brew update-reset

for pkg in cmake libarchive libexif wget;  do
    brew list --versions $pkg || brew install $pkg || brew install $pkg || :
    brew link --overwrite $pkg || brew install $pkg
done


# Install the pre-built wxWidgets package
#wget -q https://download.opencpn.org/s/rwoCNGzx6G34tbC/download \
#    -O /tmp/wx312B_opencpn50_macos109.tar.xz
#tar -C /tmp -xJf /tmp/wx312B_opencpn50_macos109.tar.xz

wget -q https://download.opencpn.org/s/MCiRiq4fJcKD56r/download \
    -O /tmp/wx315_opencpn50_macos1010.tar.xz
tar -C /tmp -xJf /tmp/wx315_opencpn50_macos1010.tar.xz

export PATH="/usr/local/opt/gettext/bin:$PATH"
echo 'export PATH="/usr/local/opt/gettext/bin:$PATH"' >> ~/.bash_profile

rm -rf build && mkdir build && cd build
CI_BUILD=ON
cmake -DOCPN_CI_BUILD=$CI_BUILD \
  -DOCPN_USE_LIBCPP=ON \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx315_opencpn50_macos1010/bin/wx-config \
  -DwxWidgets_CONFIG_OPTIONS="--prefix=/tmp/wx315_opencpn50_macos1010" \
  -DCMAKE_INSTALL_PREFIX= "/" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  ..
make -sj2
make package
