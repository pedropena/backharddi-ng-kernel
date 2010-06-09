#!/bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

if [ -z "$1" ]; then
	PROGRESSBAR=hw-detect/detect_progress_step
else
	PROGRESSBAR=$1
fi

NEWLINE="
"
MISSING_MODULES_LIST=""
SUBARCH="$(archdetect)"

finish_install=/usr/lib/finish-install.d/30hw-detect

if [ -x /sbin/depmod ]; then
	depmod -a > /dev/null 2>&1 || true
fi

# Which discover version to use.  Updated by discover_version()
DISCOVER_VERSION=1

# Is hotplug available?  Updated by hotplug_type()
HOTPLUG_TYPE=fake

log () {
	logger -t hw-detect "$@"
}

is_not_loaded() {
	! ((cut -d" " -f1 /proc/modules | grep -q "^$1\$") || \
	   (cut -d" " -f1 /proc/modules | sed -e 's/_/-/g' | grep -q "^$1\$"))
}

is_available () {
	find /lib/modules/$(uname -r)/ | sed 's!.*/!!' | cut -d . -f 1 | \
	grep -q "^$1$"
}

# Module as first parameter, description of device the second.
missing_module () {
	if ! in_list "$1" "$MISSING_MODULES_LIST"; then
		if [ -n "$MISSING_MODULES_LIST" ]; then
			MISSING_MODULES_LIST="$MISSING_MODULES_LIST, "
		fi
		MISSING_MODULES_LIST="$MISSING_MODULES_LIST$1 ($2)"
	fi
}

# The list can be delimited with spaces or spaces and commas.
in_list() {
	echo "$2" | grep -q "\(^\| \)$1\(,\| \|$\)"
}

snapshot_devs() {
	echo -n `grep : /proc/net/dev | sort | cut -d':' -f1`
}

compare_devs() {
	local olddevs="$1"
	local devs="$2"
	echo "${devs#$olddevs}" | sed -e 's/^ //'
}

load_module() {
	local module="$1"
	local cardname="$2"
	local devs=""
	local olddevs=""
	local newdev=""

	old=`cat /proc/sys/kernel/printk`
	echo 0 > /proc/sys/kernel/printk

	devs="$(snapshot_devs)"
	if log-output -t hw-detect modprobe -v "$module"; then
		olddevs="$devs"
		devs="$(snapshot_devs)"
		newdevs="$(compare_devs "$olddevs" "$devs")"

		# Make sure space is used as a delimiter.
		IFS_SAVE="$IFS"
		IFS=" "
		if [ -n "$newdevs" -a -n "$cardname" ]; then
			mkdir -p /etc/network
			for dev in $newdevs; do
				echo "${dev}:${cardname}" >> /etc/network/devnames
			done
		fi
		IFS="$IFS_SAVE"
	else   
		log "Error loading '$module'"
		if [ "$module" != floppy ] && [ "$module" != ide-floppy ] && [ "$module" != ide-cd ]; then
			db_subst hw-detect/modprobe_error CMD_LINE_PARAM "modprobe -v $module"
			db_input medium hw-detect/modprobe_error || [ $? -eq 30 ]
			db_go
		fi
	fi

	echo $old > /proc/sys/kernel/printk
}

load_sr_mod () {
	case "$(uname -r)" in
	2.4*)
		if is_not_loaded "sr_mod"; then
			if is_available "sr_mod"; then
				db_subst hw-detect/load_progress_step CARDNAME "SCSI CDROM support"
				db_subst hw-detect/load_progress_step MODULE "sr_mod"
				db_progress INFO hw-detect/load_progress_step
				load_module sr_mod
				register-module sr_mod
			else
				missing_module sr_mod "SCSI CDROM"
			fi
		fi
		;;
	esac
}

blacklist_de4x5 () {
	cat << EOF >> $finish_install
if [ -e /target/etc/discover.conf ]; then
	touch /target/etc/discover-autoskip.conf
	(echo "# blacklisted since tulip is used instead"; echo skip de4x5 ) >> /target/etc/discover-autoskip.conf
fi
EOF
}

discover_version () {
	if type discover >/dev/null 2>&1; then
		# Ugh, Discover 1.x didn't exit with nonzero status if given an
		# unrecognized option!
		DISCOVER_TEST=$(discover --version 2> /dev/null) || true
		if expr "$DISCOVER_TEST" : 'discover 2.*' > /dev/null 2>&1; then
			log "Testing experimental discover version 2."
			DISCOVER_VERSION=2
		else
			log "Using discover version 1."
			DISCOVER_VERSION=1
		fi
	else
		DISCOVER_VERSION=
	fi
}

# join hack for discover 2
dumb_join_discover (){
	IFS_SAVE="$IFS"
	IFS="$NEWLINE"
	for i in $MODEL_INFOS; do
		echo $1:$i;
		shift
	done
	IFS="$IFS_SAVE"
}

# wrapper for discover command that can distinguish Discover 1.x and 2.x
discover_hw () {
	case "$DISCOVER_VERSION" in
	2)
		dpath=linux/module/name
		dver=`uname -r|cut -d. -f1,2` # Kernel version (e.g. 2.4)
		dflags="-d all -e ata -e pci -e pcmcia -e \
			scsi bridge broadband fixeddisk humaninput modem \
			network optical removabledisk"

		MODEL_INFOS=$(discover -t $dflags)
		MODULES=$(discover --data-path=$dpath --data-version=$dver $dflags)
		dumb_join_discover $MODULES
		;;
	1)
		case "$SUBARCH" in
		  sparc/*) sbus=",sbus" ;;
		esac
		discover --format="%m:%V %M\n" --disable-all \
		          --enable=pci,ide,scsi${sbus},pcmcia ide scsi cdrom ethernet bridge |
			sed 's/ $//'
		;;
	esac
}

hotplug_type () {
	if [ -f /proc/sys/kernel/hotplug ]; then
		HOTPLUG_HANDLER="$(cat /proc/sys/kernel/hotplug)"
		case $HOTPLUG_HANDLER in
			''|/sbin/udevsend)
				HOTPLUG_TYPE=udev
				;;
			*)
				if [ -d /etc/hotplug ]; then
					HOTPLUG_TYPE=real
				else
					HOTPLUG_TYPE=fake
				fi
				;;
		esac
	else
		HOTPLUG_TYPE=
	fi
}

# Some pci chipsets are needed or there can be DMA or other problems.
get_ide_chipset_info() {
	for ide_module in $(find /lib/modules/*/kernel/drivers/ide/pci/ -type f 2>/dev/null); do
		if [ -e $ide_module ]; then
			baseidemod=$(echo $ide_module | sed 's/\.o$//' | sed 's/\.ko$//' | sed 's/.*\///')
			# hpt366 is in the discover database, and causes
			# problems with some other hardware (bug #269823)
			if [ "$baseidemod" != hpt366 ]; then
				echo "$baseidemod:IDE chipset support"
			fi
		fi
	done
}

# Return list of lines formatted "module:Description"
get_detected_hw_info() {
	if [ "${SUBARCH%%/*}" = powerpc ]; then
		discover-mac-io
		if [ "$SUBARCH" = powerpc/chrp_rs6k ] || [ "$SUBARCH" = powerpc/chrp_ibm ]; then
			discover-ibm
		fi
	fi
	if [ "${SUBARCH%%/*}" = sparc ]; then
		discover-sbus
	fi
	discover_hw
	if [ -d /proc/bus/usb ]; then
		echo "usb-storage:USB storage"
	fi
}

# NewWorld PowerMacs don't want floppy or ide-floppy, and on some models
# (e.g. G5s) the kernel hangs when loading the module.
get_floppy_info() {
	case $SUBARCH in
		powerpc/powermac_newworld) ;;
		*) echo "floppy:Linux Floppy" ;;
	esac
}

get_ide_floppy_info() {
	case $SUBARCH in
		powerpc/powermac_newworld) ;;
		*) echo "ide-floppy:Linux IDE floppy" ;;
	esac
}

get_input_info() {
	case "$(uname -r)" in
	2.4*)
		case "$(udpkg --print-architecture)" in
			i386|ia64|amd64)
				register-module psmouse
			;;
		esac
	
		case $SUBARCH in
			powerpc/chrp*|powerpc/prep)
				register-module psmouse
			;;
		esac
		;;
	esac
}

# Modules that should load before autodetection.
get_early_manual_hw_info() {
	# Load explicitly rather than implicitly to allow the user to
	# specify parameters when the module is loaded.
	echo "ide-core:Linux IDE support"
}
	
# Manually load modules to enable things we can't detect.
# XXX: This isn't the best way to do this; we should autodetect.
# The order of these modules are important.
get_manual_hw_info() {
	get_floppy_info
	# ide-mod and ide-probe-mod are needed for older (2.4.20) kernels
	echo "ide-mod:Linux IDE driver"
	echo "ide-probe-mod:Linux IDE probe driver"
	get_ide_chipset_info
	echo "ide-detect:Linux IDE detection" # 2.4.x > 20
	echo "ide-generic:Linux IDE support" # 2.6
	get_ide_floppy_info
	echo "ide-disk:Linux ATA DISK"
	echo "ide-cd:Linux ATAPI CD-ROM"
	echo "isofs:Linux ISO 9660 filesystem"
	get_input_info

	# on some hppa systems, nic and scsi won't be found because they're
	# not on a bus that discover understands ... 
	if [ "`udpkg --print-architecture`" = hppa ]; then
		echo "lasi_82596:LASI Ethernet"
		register-module lasi_82596
		echo "lasi700:LASI SCSI"
		register-module -i lasi700
		echo "zalon7xx:Zalon SCSI"
		register-module -i zalon7xx
	fi
}

# Detect discover version
discover_version

# Detect hotplug type
hotplug_type

# Should be greater than the number of kernel modules we can reasonably
# expect it will ever need to load.
MAX_STEPS=1000
OTHER_STEPS=7
# Use 1/10th of the progress bar for the non-module-load steps.
OTHER_STEPSIZE=$(expr $MAX_STEPS / 10 / $OTHER_STEPS)
db_progress START 0 $MAX_STEPS $PROGRESSBAR

# Load queued Cardbus modules, if any, and catch hotplug events.
# We need to do this before the regular PCI detection so that we can
# determine which network cards are Cardbus.
if [ -f /etc/pcmcia/cb_mod_queue ]; then
	if [ "$HOTPLUG_TYPE" = fake ]; then
		saved_hotplug=`cat /proc/sys/kernel/hotplug`
		echo /bin/hotplug-pcmcia >/proc/sys/kernel/hotplug
	fi
	for module in $(cat /etc/pcmcia/cb_mod_queue); do
		log "Loading queued Cardbus module $module"
		modprobe -v $module | logger -t hw-detect
	done
	if [ "$HOTPLUG_TYPE" = fake ]; then
		echo $saved_hotplug >/proc/sys/kernel/hotplug
	fi
fi

db_progress INFO hw-detect/detect_progress_step

# Load yenta_socket on 2.6 kernels, if hardware is available, so that
# discover will see Cardbus cards.
if [ -d /sys/bus/pci/devices ] && grep -q 0x060700 \
	/sys/bus/pci/devices/*/class && \
	! grep -q ^yenta_socket /proc/modules; then
	db_subst hw-detect/load_progress_step CARDNAME "Cardbus bridge"
	db_subst hw-detect/load_progress_step MODULE "yenta_socket"
	db_progress INFO hw-detect/load_progress_step
	
	log "Detected Cardbus bridge, loading yenta_socket"
	modprobe -v yenta_socket | logger -t hw-detect
	# Ugly hack, but what's the alternative?
	sleep 3 || true
fi

# If using real hotplug, re-run the rc scripts to pick up new modules.
# TODO: this just loads modules itself, rather than handing back a list
case $HOTPLUG_TYPE in
	real)
		/lib/debian-installer/coldplug
		;;
	udev)
		update-dev
		;;
esac

ALL_HW_INFO=$(get_early_manual_hw_info; get_detected_hw_info; get_manual_hw_info)
db_progress STEP $OTHER_STEPSIZE

# Remove modules that are already loaded or not available, and construct
# the list for the question.
LIST=""
PROCESSED=""
AVAIL_MODULES="$(find /lib/modules/$(uname -r)/ | sed 's!.*/!!' | cut -d . -f 1)"
LOADED_MODULES="$(cut -d " " -f 1 /proc/modules) $(cut -d " " -f 1 /proc/modules | sed -e 's/_/-/g')"
IFS_SAVE="$IFS"
IFS="$NEWLINE"
for device in $ALL_HW_INFO; do
	module="${device%%:*}"
	cardname="${device##*:}"
	if [ "$module" != "ignore" -a "$module" != "" ] &&
	   ! in_list "$module" "$LOADED_MODULES" &&
	   ! in_list "$module" "$PROCESSED"
	then
		if [ -z "$cardname" ]; then
			cardname="[Unknown]"
		fi
		
		if [ "$module" = de4x5 ] && ! in_list "$module" "$AVAIL_MODULES"; then
			log "Using tulip rather than unavailable de4x5"
			blacklist_de4x5
			module=tulip
			tulip_de4x5_hack=1
		fi
		
		if in_list "$module" "$AVAIL_MODULES"; then
			if [ -n "$LIST" ]; then
				LIST="$LIST, "
			fi
			LIST="$LIST$module ($(echo "$cardname" | sed 's/,/ /g'))"
			PROCESSED="$PROCESSED $module"
		else
			missing_module "$module" "$cardname"
		fi
	fi
done
IFS="$IFS_SAVE"
db_progress STEP $OTHER_STEPSIZE

if [ "$LIST" ]; then
	# Ask which modules to install.
	db_subst hw-detect/select_modules list "$LIST"
	db_set hw-detect/select_modules "$LIST"
	db_input medium hw-detect/select_modules || true
	db_go || exit 10 # back up
	db_get hw-detect/select_modules
	LIST="$RET"
fi

list_to_lines() {
	echo "$LIST" | sed 's/, /\n/g'
}

# Work out amount to step per module load. expr rounds down, so 
# it may not get quite to 100%, but will at least never exceed it.
MODULE_STEPS=$(expr \( $MAX_STEPS - \( $OTHER_STEPS \* $OTHER_STEPSIZE \) \))
if [ "$LIST" ]; then
	MODULE_STEPSIZE=$(expr $MODULE_STEPS / $(list_to_lines | wc -l))
fi

IFS="$NEWLINE"

for device in $(list_to_lines); do
	module="${device%% *}"
	cardname="`echo $device | cut -d'(' -f2 | sed 's/)$//'`"
	# Restore IFS after extracting the fields.
	IFS="$IFS_SAVE"

	if [ -z "$module" ] ; then module="[Unknown]" ; fi
	if [ -z "$cardname" ] ; then cardname="[Unknown]" ; fi

	log "Detected module '$module' for '$cardname'"

	if is_not_loaded "$module"; then
		db_subst hw-detect/load_progress_step CARDNAME "$cardname"
		db_subst hw-detect/load_progress_step MODULE "$module"
		db_progress INFO hw-detect/load_progress_step
		if [ "$cardname" = "[Unknown]" ]; then
			load_module "$module"
		else
			load_module "$module" "$cardname"
		fi

		if [ "$module" = tulip ] && [ "$tulip_de4x5_hack" = 1 ]; then
			log "Forcing use of tulip in installed system (de4x hack)"
			register-module tulip
		fi
	fi

	db_progress STEP $MODULE_STEPSIZE
	IFS="$NEWLINE"
done
IFS="$IFS_SAVE"

if [ -z "$LIST" ]; then
	db_progress STEP $MODULE_STEPS
fi

# If there is an ide bus, then register the ide CD modules so they'll be
# available on the target system for base-config. Disk too, in case root is
# not ide but ide is still used. udev should handle this for 2.6.
if [ -e /proc/ide/ -a "`find /proc/ide/* -type d 2>/dev/null`" != "" ]; then
	case "$(uname -r)" in
	2.4*)
		register-module ide-detect
		register-module ide-cd
		register-module ide-disk
	;;
	esac
fi

case "$(uname -r)" in
2.4*)
	# always load sd_mod and sr_mod if a scsi controller module was loaded.
	# sd_mod to find the disks, and sr_mod to find the CD-ROMs
	if [ -e /proc/scsi/scsi ] && ! grep -q "Attached devices: none" /proc/scsi/scsi; then
		if grep -q 'Type:[ ]\+Direct-Access' /proc/scsi/scsi && \
		   is_not_loaded "sd_mod" && \
		   ! grep -q '^[^[:alpha:]]\+sd$' /proc/devices; then
		   	if is_available "sd_mod"; then
				db_subst hw-detect/load_progress_step CARDNAME "SCSI disk support"
				db_subst hw-detect/load_progress_step MODULE "sd_mod"
				db_progress INFO hw-detect/load_progress_step
				load_module sd_mod
				register-module sd_mod
			else
				missing_module sd_mod "SCSI disk"
			fi
		fi
		db_progress STEP $OTHER_STEPSIZE
		if grep -q 'Type:[ ]\+CD-ROM' /proc/scsi/scsi && \
		   ! grep -q '^[^[:alpha:]]\+sr$' /proc/devices; then
			load_sr_mod
		fi
		db_progress STEP $OTHER_STEPSIZE
	fi
	;;
esac
	
if ! is_not_loaded ohci1394; then
	# if firewire was found, try to enable firewire cd support
	if is_not_loaded sbp2 && is_available scsi_mod; then
		if is_available sbp2; then
			db_subst hw-detect/load_progress_step CARDNAME "FireWire CDROM support"
			db_subst hw-detect/load_progress_step MODULE "sbp2"
			db_progress INFO hw-detect/load_progress_step
			load_module sbp2
		else
			missing_module sbp2 "FireWire CDROM"
		fi
	fi
	register-module sbp2
	db_progress STEP $OTHER_STEPSIZE
	load_sr_mod
	db_progress STEP $OTHER_STEPSIZE
	case "$(uname -r)" in
	2.4*)
		# rescan bus for firewire CD after loading sr_mod
		# (Sometimes this echo fails.)
		echo "scsi add-single-device 0 0 0 0" > /proc/scsi/scsi || true
	;;
	esac

	# also try to enable firewire ethernet (The right way to do this is
	# really to catch the hotplug events from the kernel.)
	if is_not_loaded eth1394; then
		case "$(uname -r)" in
		2.4*)
			:
		;;
		*)
			if is_available eth1394; then
				db_subst hw-detect/load_progress_step CARDNAME "FireWire ethernet support"
				db_subst hw-detect/load_progress_step MODULE "eth1394"
				db_progress INFO hw-detect/load_progress_step
				load_module eth1394 "FireWire ethernet"
				# do not call register-module; udev/hotplug will load it
				# on the installed system
			else
				missing_module eth1394 "FireWire ethernet"
			fi
		;;
		esac
	fi
fi

apply_pcmcia_resource_opts() {
	local config_opts=/etc/pcmcia/config.opts
	
	# Idempotency
	if ! [ -f ${config_opts}.orig ]; then
		cp $config_opts ${config_opts}.orig
	fi
	cp ${config_opts}.orig $config_opts

	local mode=""
	local rmode=""
	local type=""
	local value=""
	while [ -n "$1" ] && [ -n "$2" ] && [ -n "$3" ]; do
		if [ "$1" = exclude ]; then
			mode=exclude
			rmode=include
			shift
		elif [ "$1" = include ]; then
			mode=include
			rmode=exclude
			shift
		fi
		type="$1"
		shift
		value="$1"
		shift
		
		if grep -q "^$rmode $type $value\$" $config_opts; then
			sed "s/^$rmode $type $value\$/$mode $type $value/" \
				$config_opts >${config_opts}.new
			mv ${config_opts}.new $config_opts
		else
			echo "$mode $type $value" >>$config_opts
		fi
	done
}

# get pcmcia running if possible
PCMCIA_INIT=
if [ -x /etc/init.d/pcmciautils ]; then
	PCMCIA_INIT=/etc/init.d/pcmciautils
elif [ -x /etc/init.d/pcmcia ]; then
	PCMCIA_INIT=/etc/init.d/pcmcia
fi
if [ "$PCMCIA_INIT" ]; then
	if ! [ -e /var/run/cardmgr.pid ]; then
		db_input medium hw-detect/start_pcmcia || true
	fi
	if db_go && db_get hw-detect/start_pcmcia && [ "$RET" = true ]; then
		if ! [ -e /var/run/cardmgr.pid ]; then
			db_input medium hw-detect/pcmcia_resources || true
			db_go || true
			db_get hw-detect/pcmcia_resources || true
			apply_pcmcia_resource_opts $RET
		fi
		
		db_progress INFO hw-detect/pcmcia_step
		
		if [ -e /var/run/cardmgr.pid ]; then
			# Not using $PCMCIA_INIT stop as it
			# uses sleep which is not available and is racey.
			kill -9 $(cat /var/run/cardmgr.pid) 2>/dev/null || true
			rm -f /var/run/cardmgr.pid
		fi

		# If hotplugging is available in the kernel, we can use it to
		# load modules for Cardbus cards and tell which network
		# interfaces belong to PCMCIA devices. The former is only
		# necessary on 2.4 kernels, though.
		if [ "$HOTPLUG_TYPE" = fake ]; then
			# Snapshot discover information so we can detect
			# modules for Cardbus cards by later comparison in
			# the hotplug handler. (Only on 2.4 kernels.)
			if expr `uname -r` : "2.4.*" >/dev/null 2>&1; then
				case "$DISCOVER_VERSION" in
				2)
					dpath=linux/module/name
					dver=`uname -r|cut -d. -f1,2` # Kernel version (e.g. 2.4)
					dflags="-d all -e pci scsi fixeddisk modem network removabledisk bridge"
			
					echo `discover --data-path=$dpath --data-version=$dver $dflags` \
						| sed 's/ $//' >/tmp/pcmcia-discover-snapshot
					;;
				1)
					discover --format="%m " --disable-all --enable=pci \
						scsi ide ethernet bridge \
						| sed 's/ $//' >/tmp/pcmcia-discover-snapshot
					;;
				esac
			fi
		
			# Simple handling of hotplug events during PCMCIA
			# detection
			saved_hotplug=`cat /proc/sys/kernel/hotplug`
			echo /bin/hotplug-pcmcia >/proc/sys/kernel/hotplug
		fi
	    
		CARDMGR_OPTS="-f" $PCMCIA_INIT start </dev/null 3<&0 2>&1 \
			| logger -t hw-detect
	    
		if [ "$HOTPLUG_TYPE" = fake ]; then
			echo $saved_hotplug >/proc/sys/kernel/hotplug
			rm -f /tmp/pcmcia-discover-snapshot
		fi

		db_progress STEP $OTHER_STEPSIZE
	fi
	db_fset hw-detect/start_pcmcia seen true || true
fi

gen_pcmcia_devnames() {
	while read line; do
		log "Reading line: $line"
		line="$(echo $line | tr '\t' ' ')"

		case "$line" in
			Socket*)
			devname="$(echo $line | cut -d' ' -f3-)"
		;;
		[0-9]*)
			class="$(echo $line | cut -d' ' -f2)"
			dev="$(echo $line | cut -d' ' -f5)"

			if [ "$class" != "network" ]; then
				devname=""
				return
			else
				echo "$dev:$devname" >> /etc/network/devnames
				echo "$dev" >> /etc/network/devhotplug
			fi
		;;
		esac
	done
}

have_pcmcia=0
case "$(uname -r)" in
	2.4*)
		if [ -e "/proc/bus/pccard/drivers" ]; then
			have_pcmcia=1
		fi
	;;
	2.6*)
		if ls /sys/class/pcmcia_socket/* >/dev/null 2>&1; then
			have_pcmcia=1
		fi
	;;
esac

# find Cardbus network cards on 2.6 kernels
cardbus_check_netdev()
{
	local socket="$1"
	local netdev="$2"
	if [ -L $netdev/device ] && \
		[ -d $socket/device/$(basename $(readlink $netdev/device)) ]; then
		echo $(basename $netdev) >> /etc/network/devhotplug
	fi
}
if ls /sys/class/pcmcia_socket/* >/dev/null 2>&1; then
	for socket in /sys/class/pcmcia_socket/*; do
		for netdev in /sys/class/net/*; do
			cardbus_check_netdev $socket $netdev
		done
	done
fi

if db_get hw-detect/start_pcmcia && [ "$RET" = false ]; then
	have_pcmcia=0
fi

# Try to do this only once..
if [ "$have_pcmcia" -eq 1 ] && ! grep -q pcmcia-cs /var/lib/apt-install/queue 2>/dev/null; then
	log "Detected PCMCIA, installing pcmcia-cs."
	apt-install pcmcia-cs || true
	if expr "$(uname -r)" : 2.6 >/dev/null; then
		log "Detected PCMCIA, installing pcmciautils."
		apt-install pcmciautils || true
	fi

	echo "mkdir /target/etc/pcmcia 2>/dev/null || true" \
		>>$finish_install
	echo "cp /etc/pcmcia/config.opts /target/etc/pcmcia/config.opts" \
		>>$finish_install

	# Determine devnames.
	if [ -f /var/run/stab ]; then
		mkdir -p /etc/network
		gen_pcmcia_devnames < /var/run/stab
	fi
fi

# Install appropriate hardware detection tool into target.
if type udevd >/dev/null 2>&1; then
	apt-install udev || true
else
	case "$DISCOVER_VERSION" in
		2)
			apt-install discover || true
			;;
		1|'')
			apt-install discover1 || true
			;;
	esac
	if [ -f /proc/sys/kernel/hotplug ]; then
		apt-install hotplug || true
	fi
fi

# TODO: should this really be conditional on hotplug support?
if [ -f /proc/sys/kernel/hotplug ]; then
	apt-install usbutils || true
fi

# Install acpi (works only for 2.6 kernels)
if [ -d /proc/acpi ]; then
	apt-install acpi || true
	apt-install acpid || true
fi

# If hardware has support for pmu, install pbbuttonsd
if [ -d /sys/class/misc/pmu/ ]; then
	apt-install pbbuttonsd || true
fi

# Install optimised libc based on CPU type.
case "$(udpkg --print-architecture)" in
	i386)
		case "$(grep '^cpu family' /proc/cpuinfo | cut -d: -f2)" in
			" 6"|" 15")
				# intel 686 or Amd k6.
				apt-install libc6-i686 || true
	                ;;
		esac
	;;
	sparc)
		if grep -q '^type.*: sun4u' /proc/cpuinfo ; then
			# sparc v9 or v9b
			if grep -q '^cpu.*: .*UltraSparc III' /proc/cpuinfo; then
				apt-install libc6-sparcv9b || true
			else
				apt-install libc6-sparcv9 || true
			fi
		fi
	;;
esac

db_progress SET $MAX_STEPS
db_progress STOP

if [ -n "$MISSING_MODULES_LIST" ]; then
	log "Missing modules '$MISSING_MODULES_LIST"
fi

sysfs-update-devnames

# Let userspace /dev tools rescan the devices.
if type update-dev >/dev/null 2>&1; then
	update-dev
fi

exit 0
