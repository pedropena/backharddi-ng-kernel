#!/bin/sh

set -e
. /usr/share/debconf/confmodule
#set -x

if [ "$(uname)" != Linux ]; then
	exit 0
fi

is_not_loaded() {
	! ((cut -d" " -f1 /proc/modules | grep -q "^$1\$") || \
	   (cut -d" " -f1 /proc/modules | sed -e 's/_/-/g' | grep -q "^$1\$"))
}

list_modules_dir() {
	if [ -d "$1" ]; then
		find $1 -type f | sed -e 's/\.k\?o$//' -e 's/.*\///'
	fi
}

list_disk_modules() {
	# FIXME: not all of this stuff is disk driver modules, find a way
	# to separate out only the disk stuff.
	list_modules_dir /lib/modules/*/kernel/drivers/ide
	list_modules_dir /lib/modules/*/kernel/drivers/scsi
	list_modules_dir /lib/modules/*/kernel/drivers/block
	list_modules_dir /lib/modules/*/kernel/drivers/message/fusion
	list_modules_dir /lib/modules/*/kernel/drivers/message/i2o
}

disk_found() {
	for try in 1 2 3; do
		if search-path parted_devices; then
			# Use partman's parted_devices if available.
			if [ -n "$(parted_devices)" ]; then
				return 0
			fi
		else
			# Essentially the same approach used by partitioner and
			# autopartkit to find their disks.
			if [ -n "$(list-devices disk)" ]; then
				return 0
			fi
		fi

		# Wait for disk to be activated.
		if [ "$try" != 3 ]; then
			sleep 2
		fi
	done

	return 1
}

module_probe() {
	local module="$1"
	local priority="$2"
	local template=""
	local question="$template/$module"
	local modinfo=""
	local devs=""
	local olddevs=""
	local newdev=""
	
	if ! log-output -t disk-detect modprobe -v "$module"; then
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
			if ! log-output -t disk-detect modprobe -v "$module" $params; then
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
}

hw-detect disk-detect/detect_progress_title || true

while ! disk_found; do
	CHOICES=""
	for mod in $(list_disk_modules | sort); do
		if [ -z "$CHOICES" ]; then
			CHOICES="$mod"
		else
			CHOICES="$CHOICES, $mod"
		fi
	done

	if [ -n "$CHOICES" ]; then
		db_capb backup
		db_subst disk-detect/module_select CHOICES "$CHOICES"
		db_input high disk-detect/module_select || [ $? -eq 30 ]
		if ! db_go; then
			exit 10
		fi
		db_capb

		db_get disk-detect/module_select
		if [ "$RET" = "continue with no disk drive" ]; then
			break
		elif [ "$RET" != "none of the above" ]; then
			module="$RET"
			if [ -n "$module" ] && is_not_loaded "$module" ; then
				register-module "$module"
				module_probe "$module"
			fi
			continue
		fi
	fi

	if [ -e /usr/lib/debian-installer/retriever/floppy-retriever ]; then
		db_capb backup
		db_input critical hw-detect/load_floppy
		if ! db_go; then
			exit 10
		fi
		db_capb
		db_get hw-detect/load_floppy
		if [ "$RET" = true ] && \
		   anna floppy-retriever && \
		   hw-detect disk-detect/detect_progress_title; then
			continue
		fi
	fi

	db_capb backup
	db_input high disk-detect/cannot_find
	if ! db_go; then
		exit 10
	fi
	db_capb
done
