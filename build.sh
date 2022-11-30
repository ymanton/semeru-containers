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
#  Usage : build.sh <Image name> <Dockerfile location>                              #
#                                                                                   #
#####################################################################################

image=$1
dloc=$2
dockerFile="Dockerfile.open.releases.full"
dfile=$dloc$dockerFile
containerEngine=docker
artifactoryToken=$3

tag=`echo $image | cut -d ":" -f2`

if [ $# -lt 3 ]
then
   if [ $# == 1 ]
   then
      echo "Dockerfile location not provided, using \".\". No artifactory token provided, using \"\""
      dloc="."
      artifactoryToken=""
   else
      if [ $# == 2 ]
      then
         echo "No artifactory token provided, using \"\""
         artifactoryToken=""
      else
         echo "Usage : build.sh <Image name> <Dockerfile location> <artifactory token>"
         exit 1
      fi
   fi
fi

echo "******************************************************************************"
echo "           Starting docker build for $image                                   "
echo "******************************************************************************"

if [[ $VAR == *"17-ea"* ]]; then
  containerEngine=podman
fi

$containerEngine build --no-cache --pull -t $image -f $dfile $dloc --build-arg ARTIFACTORY_TOKEN=$artifactoryToken 2>&1 | tee build_$tag.log

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
