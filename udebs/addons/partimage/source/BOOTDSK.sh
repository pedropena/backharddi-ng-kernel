#!/bin/sh

mkdir -p /tmp/pi-flop/bin /tmp/pi-cd/bin

make distclean
make -f PACKAGES rootdisk
make
cp src/client/partimage /tmp/pi-flop/bin/partimage-floppy
cp src/server/partimaged /tmp/pi-flop/bin/partimaged-floppy
make distclean
make -f PACKAGES bootcd
make
cp src/client/partimage /tmp/pi-cd/bin/partimage
cp src/server/partimaged /tmp/pi-cd/bin/partimaged

strip --strip-all /tmp/pi-flop/bin/partimage-floppy
strip --strip-all /tmp/pi-flop/bin/partimaged-floppy
strip --strip-all /tmp/pi-cd/bin/partimage
strip --strip-all /tmp/pi-cd/bin/partimaged
