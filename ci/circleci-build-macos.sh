#!/usr/bin/env bash

#
# Build the Travis OSX artifacts 
#

# bailout on errors and echo commands
set -xe

# As of travis-ci-macos-10.13-xcode9.4.1-1529955246, the travis osx image
# contains a broken homebrew. Walk-around by reinstalling:
#curl -fsSL \
#     https://raw.githubusercontent.com/Homebrew/install/master/uninstall.sh \
#    > uninstall.sh
#chmod 755 uninstall.sh
#./uninstall.sh -q -f

#inst="https://raw.githubusercontent.com/Homebrew/install/master/install"
#/usr/bin/ruby -e "$(curl -fsSL $inst)"

#/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
#brew update-reset

set -o pipefail
#for pkg in cairo cmake libarchive libexif  wget; do
#    brew list $pkg 2>&1 >/dev/null || brew install $pkg
#done

git -C /usr/local/Homebrew/Library/Taps/homebrew/homebrew-core fetch --unshallow
git -C /usr/local/Homebrew/Library/Taps/homebrew/homebrew-cask fetch --unshallow
brew update


#brew install cairo

#wget http://opencpn.navnux.org/build_deps/wx312_opencpn50_macos109.tar.xz
#tar xJf wx312_opencpn50_macos109.tar.xz -C /tmp

# Install the pre-built wxWidgets package
wget -q https://download.opencpn.org/s/rwoCNGzx6G34tbC/download \
    -O /tmp/wx312B_opencpn50_macos109.tar.xz
tar -C /tmp -xJf /tmp/wx312B_opencpn50_macos109.tar.xz 


export PATH="/usr/local/opt/gettext/bin:$PATH"
echo 'export PATH="/usr/local/opt/gettext/bin:$PATH"' >> ~/.bash_profile
 
rm -rf build && mkdir build && cd build
CI_BUILD=ON
cmake -DOCPN_CI_BUILD=$CI_BUILD \
  -DOCPN_USE_LIBCPP=ON \
  -DwxWidgets_CONFIG_EXECUTABLE=/tmp/wx312_opencpn50_macos109/bin/wx-config \
  -DwxWidgets_CONFIG_OPTIONS="--prefix=/tmp/wx312_opencpn50_macos109" \
  -DCMAKE_INSTALL_PREFIX= "/" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  ..
make -sj2
make package
