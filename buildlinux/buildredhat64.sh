#!/bin/bash
# Run this script from the buildlinux folder to build a redhat package
qmake DEFINES+="Q_OS_REDHAT" ../src
make
cp application/safejumperplus linuxfiles
cp service/safejumperplusservice linuxfiles
cp netdown/netdown linuxfiles
cp openvpn64 linuxfiles/openvpn
cp debian/safejumperplus.service linuxfiles

# Then the content of linuxfiles mostly goes into /opt/safejumper/.

# To package redhat do the following:

tar --transform "s/^linuxfiles/safejumperplus-$1/" -zcpvf ~/rpmbuild/SOURCES/safejumperplus-$1.tar.gz linuxfiles
rpmbuild --define "debug_package %{nil}" -ba --sign -v ./safejumperplus.spec

