#!/bin/sh
./configure \
   CC=/usr/local/cross/gcc-3.3.4_glibc-2.3.2/arm-linux/bin/gcc \
   --prefix=/usr/local/cross/gcc-3.3.4_glibc-2.3.2/arm-linux/ \
   --build=i386-linux --host=arm-unknown-linux \
   --enable-tomtomtsial \
   --enable-static \
   --disable-shared 
    
