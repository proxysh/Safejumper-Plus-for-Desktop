#!/bin/bash
# Run this script from the buildlinux folder to build a debian/ubuntu package
qmake ../src
make
cp application/safejumperplus linuxfiles
cp service/safejumperplusservice linuxfiles
cp netdown/netdown linuxfiles
cp openvpn64 linuxfiles/openvpn

# Then the content of linuxfiles mostly goes into /opt/safejumper/.

# To package debian/ubuntu do the following:

tar -zcpvf ../safejumper_$1_orig.tar.gz linuxfiles
cd ../
tar -zxpvf safejumper_$1_orig.tar.gz
cp -r buildlinux/debian linuxfiles/
cd linuxfiles
dpkg-buildpackage -b -uc -us

