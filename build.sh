#!/bin/bash
#
# (C) Copyright IBM Corporation 2023
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#####################################################################################
#                                                                                   #
#  Script to build a docker image                                                   #
#                                                                                   #
#                                                                                   #
#  Usage : build.sh <Image name> <Dockerfile location> <edition> <criu secrets file>#
#                                                                                   #
#####################################################################################
set -o pipefail

image=$1
dloc=$2
edition=$3
dockerFile="Dockerfile.$edition.releases.full"
dfile=$dloc/$dockerFile
containerEngine=docker
criu_secrets=$4

tag=`echo $image | cut -d ":" -f2`

if [ $# -lt 3 ]
then
   if [ $# == 1 ]
   then
      echo "Dockerfile location not provided, using \".\". No artifactory token provided, using \"\""
      dloc="."
      criu_secrets=""
   else
      if [ $# == 2 ]
      then
         echo "No CRIU secrets file provided using \"\""
         criu_secrets=""
      else
         echo "Usage : build.sh <Image name> <Dockerfile location> <edition> <criu_secrets_file>"
         exit 1
      fi
   fi
fi

echo "******************************************************************************"
echo "           Starting build for $image                                          "
echo "******************************************************************************"

if [[ $VAR == *"17-ea"* ]]; then
  containerEngine=podman
fi

# Check if the Dockerfile is actually a shell script (for buildah-based builds)
if [ -f "$dfile.sh" ]; then
    dfile="$dfile.sh"
    echo "Detected buildah script: $dfile"
    echo "Executing buildah build..."
    
    # Export variables needed by the buildah script
    export FINAL_IMAGE_NAME="$image"
    export CRIU_SECRETS_FILE="$criu_secrets"
    
    # Execute the buildah script in a user namespace (via `buildah unshare`) so that we can mount
    # the container filesystem to the host without needing to be root.
    buildah unshare bash "$dfile" 2>&1 | tee build_$tag.log
else
    echo "Using container engine: $containerEngine"
    $containerEngine build --no-cache --pull -t $image -f $dfile $dloc --secret id=criu_secrets,src=$criu_secrets 2>&1 | tee build_$tag.log
fi

if [ $? = 0 ]
then
    echo "******************************************************************************"
    echo " SUCCESS:     $image built successfully                                       "
    echo "******************************************************************************"
else
    echo "******************************************************************************"
    echo " FAILURE:     $image build failed                                             "
    echo " LOGS: "
    echo
    tail -50 build_$tag.log | sed -e 's,^, ,'
    echo "******************************************************************************"
    exit 1
fi
