#!/usr/bin/env bash

#
#

# bailout on errors and echo commands.
set -xe
sudo apt-get -qq update

DOCKER_SOCK="unix:///var/run/docker.sock"

echo "DOCKER_OPTS=\"-H tcp://127.0.0.1:2375 -H $DOCKER_SOCK -s devicemapper\"" \
    | sudo tee /etc/default/docker > /dev/null
sudo service docker restart;
sleep 5;

#docker run --rm --privileged multiarch/qemu-user-static:register --reset
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

docker run --privileged -d -ti -e "container=docker" \
      -v ~/source_top:/source_top \
      -v $(pwd):/ci-source:rw \
      $DOCKER_IMAGE /bin/bash
 
sudo docker ps

DOCKER_CONTAINER_ID=$(sudo docker ps | grep 'ubuntu' | awk '{print $1}')


echo $DOCKER_CONTAINER_ID 
sleep 5
sudo docker ps

docker exec -ti $DOCKER_CONTAINER_ID uname -a

docker exec -ti $DOCKER_CONTAINER_ID apt-get update
docker exec -ti $DOCKER_CONTAINER_ID echo "------\nEND apt-get update\n" 

docker exec -ti $DOCKER_CONTAINER_ID apt-get -y install git cmake build-essential cmake gettext wx-common libwxgtk3.0-dev libbz2-dev libcurl4-openssl-dev libexpat1-dev libcairo2-dev libarchive-dev liblzma-dev libexif-dev lsb-release 


#docker exec -ti $DOCKER_CONTAINER_ID echo $OCPN_BRANCH

#docker exec -ti $DOCKER_CONTAINER_ID wget https://github.com/bdbcat/oernc_pi/tarball/$OCPN_BRANCH
#docker exec -ti $DOCKER_CONTAINER_ID tar -xzf $OCPN_BRANCH -C source_top --strip-components=1


#docker exec -ti $DOCKER_CONTAINER_ID /bin/bash -c \
#    'mkdir source_top/build; cd source_top/build; cmake ..; make; make package;'

docker exec -ti $DOCKER_CONTAINER_ID /bin/bash -c \
    'mkdir ci-source/build; cd ci-source/build; cmake -DCMAKE_INSTALL_PREFIX=/usr ..; make; make package;'
 
echo "Stopping"
docker ps -a
docker stop $DOCKER_CONTAINER_ID
docker rm -v $DOCKER_CONTAINER_ID

sudo apt-get install python3-pip python3-setuptools

#  Upload to cloudsmith

UNSTABLE_REPO=${CLOUDSMITH_UNSTABLE_REPO:-'david-register/ocpn-plugins-unstable'}
STABLE_REPO=${CLOUDSMITH_STABLE_REPO:-'david-register/ocpn-plugins-stable'}

echo "Check 0.5"
echo $STABLE_REPO
echo $UNSTABLE_REPO

if [ -z "$CLOUDSMITH_API_KEY" ]; then
    echo 'Cannot deploy to cloudsmith, missing $CLOUDSMITH_API_KEY'
    exit 0
fi

echo "Using \$CLOUDSMITH_API_KEY: ${CLOUDSMITH_API_KEY:0:4}..."

set -xe

#python -m ensurepip

python3 -m pip install -q setuptools
python3 -m pip install -q cloudsmith-cli

BUILD_ID=${APPVEYOR_BUILD_NUMBER:-1}
commit=$(git rev-parse --short=7 HEAD) || commit="unknown"
tag=$(git tag --contains HEAD)

echo "Check 1"
echo $tag
echo $commit
echo $OCPN_BRANCH

#  shift to the build directory linked from docker execution
echo "Check 1.5"
pwd
ls

cd build

xml=$(ls *.xml)
tarball=$(ls *.tar.gz)
tarball_basename=${tarball##*/}

# extract the project name for a filename.  e.g. oernc-pi... sets PROJECT to  "oernc"
PROJECT=$(ls *.xml | awk '{split($0,a,"-"); print a[1]}')

echo "Check 2"
echo $PROJECT
echo $xml
echo $tarball_name
echo $tarball_basename
echo $tarball


source ../build/pkg_version.sh
test -n "$tag" && VERSION="$tag" || VERSION="${VERSION}.${commit}"
test -n "$tag" && REPO="$STABLE_REPO" || REPO="$UNSTABLE_REPO"
tarball_name=${PROJECT}-${PKG_TARGET}-${PKG_TARGET_VERSION}-tarball

echo "Check 3"
echo $tarball_name
# There is no sed available in git bash. This is nasty, but seems
# to work:
touch ~/xml.tmp
while read line; do
    line=${line/@pkg_repo@/$REPO}
    line=${line/@name@/$tarball_name}
    line=${line/@version@/$VERSION}
    line=${line/@filename@/$tarball_basename}
    echo $line
done < $xml > ~/xml.tmp
cp ~/xml.tmp ~/$xml

echo "Check 4"
#echo $PKG_TARGET
#raspbian
#echo $PKG_TARGET_VERSION
#10

cat ~/$xml
#cat ~/xml.tmp

#sudo gunzip $tarball
#tarball_tar=$(ls *.tar)
#sudo cp ~/$xml metadata.xml 
#sudo tar -rf $tarball_tar metadata.xml
#sudo gzip $tarball_tar

sudo tar xf $tarball
tar_dir=${tarball%%.tar.gz}
ls -la
ls -la $tar_dir
sudo cp ~/$xml $tar_dir/metadata.xml
tar_dir_here=${tar_dir##*/}
sudo tar czf $tarball $tar_dir_here

cloudsmith push raw --republish --no-wait-for-sync \
    --name ${PROJECT}-${PKG_TARGET}-${PKG_TARGET_VERSION}-metadata \
    --version ${VERSION} \
    --summary "opencpn plugin metadata for automatic installation" \
    $REPO ~/$xml

cloudsmith push raw --republish --no-wait-for-sync \
    --name $tarball_name  \
    --version ${VERSION} \
    --summary "opencpn plugin tarball for automatic installation" \
    $REPO $tarball

