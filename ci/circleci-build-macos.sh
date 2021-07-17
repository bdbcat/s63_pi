#!/usr/bin/env bash

#
# Build the Travis OSX artifacts 
#

# bailout on errors and echo commands
set -xe
set -o pipefail

git -C /usr/local/Homebrew/Library/Taps/homebrew/homebrew-core fetch --unshallow
git -C /usr/local/Homebrew/Library/Taps/homebrew/homebrew-cask fetch --unshallow
brew update-reset

#for pkg in cmake libarchive libexif  wget; do
#    brew list $pkg 2>&1 >/dev/null || brew install $pkg
#done

for pkg in cmake libarchive libexif wget;  do
    brew list --versions $pkg || brew install $pkg || brew install $pkg || :
    brew link --overwrite $pkg || brew install $pkg
done

#brew install cairo

#wget http://opencpn.navnux.org/build_deps/wx312_opencpn50_macos109.tar.xz
#tar xJf wx312_opencpn50_macos109.tar.xz -C /tmp

#brew list --versions libexif
#libexif 0.6.22
#+++ dirname ci/circleci-build-macos.sh
#++ cd ci
#++ pwd
#+ here=/Users/distiller/project/ci
#++ sed /#/d
#+ for pkg in '$(sed '\''/#/d'\'' < $here/../build-deps/macos-deps)'
#brew list --versions cmake
#cmake 3.20.5
##brew link --overwrite cmake
#Linking /usr/local/Cellar/cmake/3.20.5... 33 symlinks created.
#+ for pkg in '$(sed '\''/#/d'\'' < $here/../build-deps/macos-deps)'
#brew list --versions gettext
#gettext 0.20.1 0.21
##brew link --overwrite gettext
#Linking /usr/local/Cellar/gettext/0.21... 159 symlinks created.
#+ for pkg in '$(sed '\''/#/d'\'' < $here/../build-deps/macos-deps)'
#brew list --versions libexif
#libexif 0.6.22
##brew link --overwrite libexif
#Linking /usr/local/Cellar/libexif/0.6.22... 42 symlinks created.
#+ for pkg in '$(sed '\''/#/d'\'' < $here/../build-deps/macos-deps)'
#brew list --versions python
#python 3.7.4_1
##brew link --overwrite python
#Warning: Already linked: /usr/local/Cellar/python/3.7.4_1
#To relink, run:
#  brew unlink python && brew link python
#+ for pkg in '$(sed '\''/#/d'\'' < $here/../build-deps/macos-deps)'
#brew list --versions wget
#wget 1.21.1
##brew link --overwrite wget
#Linking /usr/local/Cellar/wget/1.21.1... 80 symlinks created.







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
