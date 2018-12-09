#!/bin/bash
# Run this script from the buildlinux folder to build a 32 bit redhat package
qmake DEFINES+="Q_OS_REDHAT" ../src
make
cp application/safejumper linuxfiles
cp service/safejumperservice linuxfiles
cp netdown/netdown linuxfiles
cp openvpn32 linuxfiles/openvpn
cp debian/safejumper.service linuxfiles

# Then the content of linuxfiles mostly goes into /opt/safejumper/.

# To package redhat do the following:

tar --transform "s/^linuxfiles/safejumper-$1/" -zcpvf ~/rpmbuild/SOURCES/safejumper-$1.tar.gz linuxfiles
rpmbuild --define "debug_package %{nil}" -ba --sign -v --target=i686 ./safejumper.spec

