#!/usr/bin/bash

# Fetch class-memory-disclaim builds from ymanton1
# Build semeru and liberty images with class-memory-disclaim build

set -euxo pipefail

debug=class-memory-disclaim
prod=class-memory-disclaim-prod
BUILD=${1:-debug}
BUILD=${!BUILD}

cd ./17/jdk/ubi/ubi8

rsync -avp --del ymanton1.fyre.ibm.com:openj9-openjdk-jdk17/build/$BUILD/images/jdk ./
if [ $BUILD = $debug ]
then
	rsync -avp --del --exclude '.git' --exclude 'TKG' ymanton1.fyre.ibm.com:openj9 ./
	rsync -avp --del --exclude '.git' --exclude 'TKG' ymanton1.fyre.ibm.com:omr ./
fi

podman build -t semeru-17-$BUILD -t icr.io/appcafe/ibm-semeru-runtimes:open-17-jdk-ubi -f Dockerfile.open.dev.full --secret id=criu_secrets,src=$HOME/criu_secrets,type=file .

cd ~/OpenLiberty-ci.docker/releases/latest/kernel-slim

podman build -t liberty-$BUILD -f Dockerfile.ubi.openjdk17 .

