#!/bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

if [ "$(uname)" != Linux ]; then
	exit 0
fi

# This is a hack, but we don't have a better idea right now.
# See Debian bug #136743
if [ -x /sbin/depmod ]; then
	depmod -a > /dev/null 2>&1 || true
fi

is_not_loaded() {
	! ((cut -d" " -f1 /proc/modules | grep -q "^$1\$") || \
	   (cut -d" " -f1 /proc/modules | sed -e 's/_/-/g' | grep -q "^$1\$"))
}

load_module() {
	local module="$1"
	local priority=low

	case "$module" in
	"plip")
		module_probe parport_pc high
		priority=high
		;;
	esac

	module_probe "$module" "$priority"
}

list_modules_dir() {
	find $1 -type f | sed -e 's/\.k\?o$//' -e 's/.*\///'
}

list_nic_modules() {
	list_modules_dir /lib/modules/*/kernel/drivers/net
	if [ -d /lib/modules/$(uname -r)/kernel/drivers/usb/net ]; then
		list_modules_dir /lib/modules/$(uname -r)/kernel/drivers/usb/net
	else
		# usb nic modules not separated from non-nic. List all
		# usb modules that have an entry in devnames.
		for module in $(list_modules_dir /lib/modules/*/kernel/drivers/usb); do
			if [ -n "$(get_static_modinfo $module)" ]; then
				echo $module
			fi
		done
	fi
}

snapshot_devs() {
	echo -n `grep : /proc/net/dev | sort | cut -d':' -f1`
}

compare_devs() {
	local olddevs="$1"
	local devs="$2"
	echo ${devs#$olddevs} | sed -e 's/^ //'
}

DEVNAMES_STATIC=/etc/network/devnames-static.gz
TEMP_EXTRACT=/tmp/devnames-static.txt
get_static_modinfo() {
	local module="$(echo $1 | sed 's/\.k\?o//')"
	local modinfo=""

	if [ ! -f "$TEMP_EXTRACT" ]; then
		zcat $DEVNAMES_STATIC > $TEMP_EXTRACT
	fi

	if grep -q "^${module}:" $TEMP_EXTRACT; then
		modinfo=$(zcat $DEVNAMES_STATIC | grep "^${module}:" | head -n 1 | cut -d':' -f2-)
	fi
	echo "$modinfo"
}

cleanup () {
	rm -f $TEMP_EXTRACT
}

ethernet_found() {
	local ifaces=0
	local firewire=0

	for iface in $(sed -e "s/lo://" < /proc/net/dev | grep "[a-z0-9]*:[ ]*[0-9]*" | sed "s/:.*//"| sed "s/^ *//"); do
		ifaces=$(expr $ifaces + 1)
		if [ -f /etc/network/devnames ]; then
			if grep "^$iface:" /etc/network/devnames | grep -q -i firewire; then
				firewire=$(expr $firewire + 1)
			fi
		fi
	done

	if [ "$ifaces" = 0 ]; then
		return 1
	elif [ "$ifaces" = "$firewire" ]; then
		db_input high ethdetect/use_firewire_ethernet || true
		db_go || true
		db_get ethdetect/use_firewire_ethernet
		if [ "$RET" = true ]; then
			return 0
		else
			return 1
		fi
	else
		# At least one regular ethernet interface
		return 0
	fi
}

module_probe() {
	local module="$1"
	local priority="$2"
	local question="$template/$module"
	local modinfo=""
	local devs=""
	local olddevs=""
	local newdev=""

	devs="$(snapshot_devs)"

	if ! log-output -t ethdetect modprobe -v "$module"; then
		# Prompt the user for parameters for the module.
		local template="hw-detect/retry_params"
		db_unregister "$question"
		db_register "$template" "$question"
		db_subst "$question" MODULE "$module"
		db_input critical "$question" || [ $? -eq 30 ]
		db_go
		db_get "$question"
		local params="$RET"

		if [ -n "$params" ]; then
			if ! log-output -t ethdetect modprobe -v "$module" $params; then
				db_unregister "$question"
				db_subst hw-detect/modprobe_error CMD_LINE_PARAM "modprobe -v $module $params"
				db_input critical hw-detect/modprobe_error || [ $? -eq 30 ]
				db_go
				false
			else
				# Module loaded successfully
				if [ "$params" != "" ]; then
					register-module "$module" "$params"
				fi
			fi
		fi
	fi

	olddevs="$devs"
	devs="$(snapshot_devs)"
	newdevs="$(compare_devs "$olddevs" "$devs")"

	# Pick up multiple cards that were loaded by a single module
	# hence they'll have same description

	modinfo=$(get_static_modinfo $module)

	if [ -n "$newdevs" -a -n "$modinfo" ]; then
		for ndev in $newdevs; do
			echo "${ndev}:${modinfo}" >> /etc/network/devnames
		done
	fi
}

hw-detect ethdetect/detect_progress_title || true

while ! ethernet_found; do
	CHOICES=""
	for mod in $(list_nic_modules | sort); do
		modinfo=$(get_static_modinfo $mod)
		if [ -n "$modinfo" ]; then
			if [ "$modinfo" = BLACKLIST ]; then
				continue
			fi
			mod="$mod: $modinfo"
		fi
		if [ -z "$CHOICES" ]; then
			CHOICES="$mod"
		else
			CHOICES="$CHOICES, $mod"
		fi
	done

	if [ -n "$CHOICES" ]; then
		db_capb backup
		db_subst ethdetect/module_select CHOICES "$CHOICES"
		db_input high ethdetect/module_select || [ $? -eq 30 ]
		if ! db_go; then
			cleanup
			exit 10
		fi
		db_capb

		db_get ethdetect/module_select
		if [ "$RET" = "no ethernet card" ]; then
			break
		elif [ "$RET" != "none of the above" ]; then
			module="$(echo $RET | cut -d: -f1)"
			if [ -n "$module" ] && is_not_loaded "$module" ; then
				register-module "$module"
				load_module "$module"
			fi
			continue
		fi
	fi

	if [ -e /usr/lib/debian-installer/retriever/floppy-retriever ]; then
		db_capb backup
		db_input critical hw-detect/load_floppy
		if ! db_go; then
			cleanup
			exit 10
		fi
		db_capb
		db_get hw-detect/load_floppy
		if [ "$RET" = true ] && \
		   anna floppy-retriever && \
		   hw-detect ethdetect/detect_progress_title; then
			continue
		fi
	fi

	db_capb backup
	db_input high ethdetect/cannot_find
	if ! db_go; then
		cleanup
		exit 10
	fi
	db_capb

	if [ -z "$CHOICES" ]; then
		sysfs-update-devnames || true
		cleanup
		exit 1
	fi
done

sysfs-update-devnames || true
cleanup
