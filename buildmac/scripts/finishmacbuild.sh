#!/bin/bash
hdiutil detach /Volumes/safejumper/

sh scripts/package.sh 
hdiutil mount safejumper.dmg
scripts/ddstoregen.sh safejumper
hdiutil detach /Volumes/safejumper/

mv safejumper.dmg tmp.dmg
hdiutil convert -format UDRO -o safejumper.dmg tmp.dmg
rm tmp.dmg
hdiutil convert safejumper.dmg -format UDZO -imagekey -zlib-level=9 -o safejumper-compressed.dmg
rm safejumper.dmg
mv safejumper-compressed.dmg safejumper.dmg
