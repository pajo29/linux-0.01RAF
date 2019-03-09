#!/bin/bash
echo Starting...
echo 'Pavle Prica RN75/18'
qemu-system-i386 -hdb hd_oldlinux.img -fda Image -boot a
