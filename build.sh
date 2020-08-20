#!/bin/bash

echo
echo "Clean Build Directory"
echo 

#make clean && make mrproper
#rm -rf ./out

echo
echo "Issue Build Commands"
echo

mkdir -p out
export ARCH=arm64
export SUBARCH=arm64
BASE_PATH=/home/android/pixel
EXT_UTILS=$BASE_PATH/wahoo-kernel-tools/bin
export CLANG_PATH=$BASE_PATH/clang-10/bin
export PATH=${CLANG_PATH}:${EXT_UTILS}:${PATH}
export DTC_EXT=$EXT_UTILS/dtc
export CLANG_TRIPLE=aarch64-linux-gnu-
export CROSS_COMPILE=$BASE_PATH/aarch64-linux-android-4.9/bin/aarch64-linux-android-
export CROSS_COMPILE_ARM32=$BASE_PATH/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-
export LD_LIBRARY_PATH=$BASE_PATH/clang-10/lib:$LD_LIBRARY_PATH

echo
echo "Set DEFCONFIG"
echo 
make CC=clang AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip O=out vendor/cleanslate_obiwan_defconfig

echo
echo "Build The Good Stuff"
echo 

make CC=clang AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip O=out -j4
