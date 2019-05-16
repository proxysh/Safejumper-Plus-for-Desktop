#!/bin/bash
# Before executing this script make sure you have the 64 bit debian package installed.
cat files.txt | zip safejumperplus-linux-64.zip -@
zip -r safejumperplus-linux-64.zip /opt/safejumperplus
