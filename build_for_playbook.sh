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


#We set a destination where everything gets installed.

PREFIX=$(pwd)/playbook_prefix

# You might choose to run this if you change something 
# in the build scripts.  We comment it out. 

#NOCONFIGURE=1 ./autogen.sh --quick --skip-gnulib 

# Note that we explicitly put --without-x because the configure 
# system has been known to forget that we are cross compiling 
# and over-zealously look for x11 headers and find them in /usr/include. 

RANLIB="${QNX_HOST}/usr/bin/ntoarmv7-ranlib " \
CPP="${QNX_HOST}/usr/bin/qcc -V4.4.2,gcc_ntoarmv7le_cpp -E " \
CC="${QNX_HOST}/usr/bin/qcc -V4.4.2,gcc_ntoarmv7le_cpp " \
LD="${QNX_HOST}/usr/bin/ntoarmv7-ld " \
CPPFLAGS="-D__PLAYBOOK__ -D__QNXNTO__ -I $(pwd)/../TouchControlOverlay/public " \
CFLAGS=" -g " \
LDFLAGS="-L$(pwd)/../TouchControlOverlay/Device-Debug -lTouchControlOverlay -L${QNX_TARGET}/armle-v7/lib -L${PREFIX}/lib -lscreen -lasound -lpps -lm -lpng14 -lbps -lxml2 -lEGL -lGLESv2" \
PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig \
PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig \
./configure --prefix="${PREFIX}" \
            --build=i686-pc-linux \
            --host=arm-unknown-nto-qnx6.5.0eabi \
            --without-x \
            --enable-pthreads \
            --enable-audio \
            --enable-video-playbook

PREFIX=${PREFIX} make all


if [ $? != 0 ]; then
  echo "Build Failed!"
  exit 1
fi

PREFIX=${PREFIX} make install

cd test

RANLIB="${QNX_HOST}/usr/bin/ntoarmv7-ranlib " \
CPP="${QNX_HOST}/usr/bin/qcc -V4.4.2,gcc_ntoarmv7le_cpp -E " \
CC="${QNX_HOST}/usr/bin/qcc -V4.4.2,gcc_ntoarmv7le_cpp " \
LD="${QNX_HOST}/usr/bin/ntoarmv7-ld " \
CFLAGS="-g -D__PLAYBOOK__ -D__QNXNTO__ -I ${PREFIX}/include " \
LIBS="-L$(pwd)/../../TouchControlOverlay/Device-Debug -lTouchControlOverlay -L${QNX_TARGET}/armle-v7/lib -L${PREFIX}/lib -lscreen -lasound -lpps -lm -lpng14 -lbps -lxml2  -lEGL -lGLESv2 " \
PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig \
PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig \
./configure --prefix="${PREFIX}" \
            --build=i686-pc-linux \
            --host=arm-unknown-nto-qnx6.5.0eabi \
            --without-x

PREFIX=${PREFIX} make all

if [ $? != 0 ]; then
  echo "Build Failed!"
  exit 1
fi

echo "Build Successful!!"
exit 0
