#!/bin/sh -e
# print "1" if device $1 is either removable or attached to a bus listed
# in $2 (e.g. 'ieee1394 usb') and "0" otherwise.

# check if the device $1 is on the bus $2
# this is done by checking if any of the devices on the bus is a prefix
# of the device
on_bus() {
  local BUSDEVP="/sys/bus/$2/devices"
  for link in $BUSDEVP/*; do
    [ -L "$link" ] || continue
    if echo "$1" | grep -q "^$(readlink -f $link)/"; then
      return 0
    fi
  done
  return 1
}

# read the first line of the file $1
read_value() {
  local value
  read -r value < $1 || true
  echo $value
}

# strip the partition number, if present
DEV="${1%%[0-9]*}"
SCAN_BUS="$2"

BLOCKPATH="/sys/block/$DEV"

[ -d $BLOCKPATH ] || exit 1

IS_REMOVABLE=$(read_value $BLOCKPATH/removable)

if [ "$IS_REMOVABLE" != 1 -a "$SCAN_BUS" ]; then
  DEVICE="$(readlink -f "${BLOCKPATH}/device")"
  for bus in "$SCAN_BUS"; do
    if on_bus $DEVICE $bus; then
      IS_REMOVABLE=1
      break
    fi
  done
fi

if [ "$IS_REMOVABLE" = "1" ]; then
  echo 1
else
  echo 0
fi

exit 0
