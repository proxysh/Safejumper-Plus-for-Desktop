#!/bin/sh

VERSION=$1

scripts/pkg-dmg \
    --verbosity 2 \
    --volname "safejumper" \
    --source application/Safejumper.app \
    --sourcefile \
    --format UDRW \
    --target safejumper.dmg \
    --icon application.icns  \
    --mkdir .background \
    --symlink  /Applications:Applications \
    --copy uninstall.sh:uninstall.sh \
    --copy README.txt:README.txt

