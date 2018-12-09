#!/bin/bash
VERSION=2018.12.11
QTVERSION=5.12.0
OLDPATH=$PATH
./cleanup.sh
export PATH=../../../staticqt/qt-$QTVERSION-32bit/qtbase/bin:$OLDPATH
./buildredhat32.sh $VERSION
./cleanup.sh
export PATH=../../../staticqt/qt-$QTVERSION-64bit/qtbase/bin:$OLDPATH
./buildredhat64.sh $VERSION
./cleanup.sh
export PATH=$OLDPATH
