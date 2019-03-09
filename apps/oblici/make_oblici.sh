#!/bin/bash
if gcc -m32 -o oblici.bin trougao.c kvadrat.c krug.c oblici.c -nostdlib -nostdinc -e main -Ttext=100 -static -fno-builtin ../../lib/lib.a -I ../../include; then
cd ../..
mkdir tmp_hd
sudo mount -o loop,offset=10240 hd_oldlinux.img tmp_hd
sudo cp apps/oblici/oblici.bin tmp_hd/root
sleep .5
sudo umount tmp_hd
rmdir tmp_hd
rm apps/oblici/oblici.bin
echo 'Uspesno kompajliranje, mozete pokrenuti QEMU'
echo 'Pavle Prica RN75/18'
else
echo 'GCC Compile failed!'
echo 'Kompajliranje neuspesno'
echo 'Proverite dir pokretanja skripte'
fi
