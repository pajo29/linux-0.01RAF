#!/bin/bash
echo Compiling..
make clean && make
qemu-system-i386 -hdb hd_oldlinux.img -fda Image -boot a
echo Completed. I hope everything was ok :/
