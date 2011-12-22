#!/bin/bash 

# Copyright (c) 2011 Research In Motion Limited.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script builds the SDL library.  It is useful as 
# an example of how to build software for the playbook 
# platform where the build system for that software is 
# configured with autoconf tools. 

#since we use relative paths, please make sure you run this 
#script in the directory where you found it. 

if [ $# -ne 4 ]
then
  echo "Usage: `basename $0` <testapp> <playbook_ipaddr> <playbook_password> [8bpp | 32bpp]"
  exit -1
fi

cp ../../TouchControlOverlay/Device-Debug/libTouchControlOverlay.so ../playbook_prefix/lib/
echo "<qnx>" > blackberry-tablet.xml
echo "   <id>org.libsdl.$1</id>" >> blackberry-tablet.xml
echo "   <name>$1</name>" >> blackberry-tablet.xml
echo "   <category>core.all</category>" >> blackberry-tablet.xml
echo "   <versionNumber>1.0.0</versionNumber>" >> blackberry-tablet.xml
echo "   <buildId>1</buildId>" >> blackberry-tablet.xml
echo "   <description>Simple DirectMedia $1 App</description>" >> blackberry-tablet.xml
echo "   <filename>$1</filename>" >> blackberry-tablet.xml
echo "   <initialWindow>" >> blackberry-tablet.xml
echo "      <systemChrome>none</systemChrome>" >> blackberry-tablet.xml
echo "      <transparent>false</transparent>" >> blackberry-tablet.xml
echo "   </initialWindow>" >> blackberry-tablet.xml

echo "   <permission system=\"true\">run_native</permission>" >> blackberry-tablet.xml
echo "   <asset path=\"../playbook_prefix/lib/libSDL-1.2.so.11\" type=\"Qnx/Elf\">libSDL-1.2.so.11</asset>" >> blackberry-tablet.xml
echo "   <asset path=\"../playbook_prefix/lib/libTouchControlOverlay.so\" type=\"Qnx/Elf\">libTouchControlOverlay.so</asset>" >> blackberry-tablet.xml
echo "   <asset path=\"icon.bmp\">icon.bmp</asset>" >> blackberry-tablet.xml
echo "   <asset path=\"sail.bmp\">sail.bmp</asset>" >> blackberry-tablet.xml
echo "   <asset path=\"sample.bmp\">sample.bmp</asset>" >> blackberry-tablet.xml
echo "   <asset path=\"sample.wav\">sample.wav</asset>" >> blackberry-tablet.xml
echo "   <asset path=\"moose.dat\">moose.dat</asset>" >> blackberry-tablet.xml
echo "   <asset path=\"utf8.txt\">utf8.txt</asset>" >> blackberry-tablet.xml

if [ "$4" = "8bpp" ]
then
  echo "   <env var=\"SDL_VIDEODRIVER\" value=\"pb-8bit\"/>" >> blackberry-tablet.xml
fi
echo "   <env var=\"LD_LIBRARY_PATH\" value=\"app/native/lib\"/>" >> blackberry-tablet.xml
echo "</qnx>" >> blackberry-tablet.xml

blackberry-nativepackager -package -target bar $1.bar blackberry-tablet.xml $1 icon.bmp sail.bmp sample.bmp sample.wav moose.dat utf8.txt -C ../playbook_prefix ../playbook_prefix/lib/libSDL-1.2.so.11 ../playbook_prefix/lib/libTouchControlOverlay.so
blackberry-deploy -installApp -device $2 -password $3 -package $1.bar

rm $1.bar blackberry-tablet.xml


