#!/bin/bash
echo 'Starting..'
echo 'This will delete all compiled files in root directory'
mkdir tmp_hd
sudo mount -o loop,offset=10240 hd_oldlinux.img tmp_hd
cd tmp_hd/root
sudo rm * -f
cd ../..
sudo umount tmp_hd
sudo rmdir tmp_hd
echo 'All files deleted.'
