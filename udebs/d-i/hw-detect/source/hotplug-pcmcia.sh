#!/bin/sh
#
# hotplug-pcmcia.sh - Handle hotplug events for PCMCIA devices during detection
#

log () {
	logger -t hotplug-pcmcia "$@"
}

TYPE="$1"

case $TYPE in
	net)
		if [ "$INTERFACE" = "" ]; then
			log "Got net event without interface"
			exit 1
		fi

		log "Detected PCMCIA network interface $INTERFACE"
		echo $INTERFACE >>/etc/network/devhotplug
	;;

	# PCI hotplugging for Cardbus cards on 2.4 kernels only
	pci)
		if [ "$PCI_SLOT_NAME" = "" ]; then
			log "Got pci event without slot name"
			exit 1
		fi

		# Sanity check
		if ! [ -f /tmp/pcmcia-discover-snapshot ]; then
			log "Got PCI event but have no discover snapshot! 2.6 kernel?"
			exit 1
		fi

		log "Detected Cardbus device at $PCI_SLOT_NAME"

		# Take another snapshot of discover information and compare it
		# with the old one to find out the module for the new device

		log "Searching for module..."

		modules_before=`cat /tmp/pcmcia-discover-snapshot`

		DISCOVER_TEST=$(discover --version 2> /dev/null) || true
		if expr "$DISCOVER_TEST" : 'discover 2.*' > /dev/null 2>&1; then
			dpath=linux/module/name
			dver=`uname -r|cut -d. -f1,2` # Kernel version (e.g. 2.4)
			dflags="-d all -e pci scsi fixeddisk modem network removabledisk"
	    
			echo `discover --data-path=$dpath --data-version=$dver $dflags` \
				| sed 's/ $//' >/tmp/pcmcia-discover-snapshot
		else
			discover --format="%m " --disable-all --enable=pci \
				scsi ide ethernet \
				| sed 's/ $//' >/tmp/pcmcia-discover-snapshot
		fi
	    
		modules_after=`cat /tmp/pcmcia-discover-snapshot`
		module=`echo "${modules_after#$modules_before}" | sed 's/^ //'`

		if [ -n "$module" ]; then
			log "Found module $module, loading"
			if modprobe $module >/dev/null 2>&1; then
				log "Module $module loaded successfully"
			else
				log "Failed loading $module (queuing)"
				echo "$module" >>/etc/pcmcia/cb_mod_queue
			fi
		else
			log "No module found for Cardbus device at $PCI_SLOT_NAME"
		fi
	;;

	pcmcia_socket)
		log "Got pcmcia_socket event"
	;;
	
	*)
		log "Got unsupported event type \"$TYPE\""
	;;
esac
