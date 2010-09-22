
. /lib/partman/definitions.sh

BACKHARDDI=/var/lib/backharddi
LOG=/var/log/backharddi
REST_MBR_ERROR=/tmp/restore_mbr.error

to_secure_string() {
        tr " " "_" | sed "s/[^a-zA-Z0-9ñÑçÇáéíóúàèìòù\+\.,:;-]/_/g"
}

to_original_string() {
	tr "_" " "
}

escape_string() {
	sed "s/+/%2b/g;s/ /%20/g;s/\//%2f/g"
}


sendstatus(){
	db_get backharddi/net/server
	server="$RET"
	if [ x"$server" != x ]; then
		db_get backharddi/net/service_port
		service_port="$RET"
		if [ x"$service_port" = x ]; then
			service_port=4600
		fi
		sendstatus_to_server "$server" "$service_port" $@
	fi
}
	
sendstatus_to_server(){
	server=$1
	port=$2
	status=$3
	shift
	shift
	shift
	wget -q -O /dev/null http://$server:$port/status?status=$status\;msg=$(echo $@ | escape_string)
}

update_partition() {
    local u
    cd $1
    open_dialog PARTITION_INFO $2
    read_line part
    close_dialog
    [ "$part" ] || return 0
    for u in /lib/backharddi/update.d/*; do
	[ -x "$u" ] || continue
	$u $1 $part
    done
}

update_metadata() {
	for dev in $DEVICES/*; do
		[ -d $dev ] || continue
		for id in $dev/*; do
			[ -d $id ] || continue
			rm -rf $id
		done
	done
	initcount=$(ls /lib/backharddi/inicio.d/* | wc -l)
	db_progress START 0 $initcount backharddi/progress/update/title
	for s in /lib/backharddi/inicio.d/*; do
		if [ -x $s ]; then
			base=$(basename $s | sed 's/[0-9]*//')
			if ! db_progress INFO backharddi/progress/update/$base; then
				db_progress INFO backharddi/progress/update/fallback
			fi

			if ! $s; then
				db_progress STOP
				exit 10
			fi
		fi
		db_progress STEP 1
	done
	db_progress STOP
}

backharddi_device() {
	db_get backharddi/medio
	[ "$RET" = hd-media ] || { echo no; return; }

	for id in $DEVICES/$1/*; do
		[ -f $id/detected_filesystem ] || continue
		[ -f $id/path ] || continue
		[ "$(cat $id/detected_filesystem)" = "ext3" ] && [ "$(e2label $(cat $id/path))" = "$label" ] && {
			echo $id
			return
		}
	done
	echo no
}

restore_mbr() {
	local dev x1 x2 start x3 size x4 ID x5 device num type fs
	cd $1
	open_dialog NEW_LABEL msdos
	close_dialog
	[ -f $REST_MBR_ERROR ] && rm $REST_MBR_ERROR
	sed -n "s/,//g; s/=/=\ /g; /start=/p" pt | while read dev x1 x2 start x3 size x4 ID x5; do
               num=${dev#$device}
               num=${num%:}
               if [ $num -gt 9 ]; then
                       start=$x2
                       size=$x3
                       ID=$x4
                       dev=${dev%:}
               fi
		[ $ID = 0 ] && continue
		device=$(cat device)
		type=primary
		[ $ID = 5 -o $ID = f ] && type=extended
		[ $num -gt 4 ] && type=logical
		id=$((start*512))-$(((start+size)*512-1))
		fs=ext3
		[ -f $id/detected_filesystem ] && fs=$(cat $id/detected_filesystem)
		open_dialog NEW_PARTITION $type $fs $id real 0
		read_line num newid newsize newtype fs path name
		close_dialog
		[ $id = $newid ] || { touch $REST_MBR_ERROR; break; } 
	done
	cd $2
	[ ! -f $REST_MBR_ERROR ]
}

restore_metadata() {
	for s in /lib/partman/undo.d/*; do
		if [ -x $s ]; then
			$s || true
		fi
	done
	update_metadata
}

backup_ok() {
	db_metaget backharddi/text/label description
	label="$RET"
	[ -z "$label" ] && label="backharddi"

	db_get backharddi/medio
	medio="$RET"
	if [ "$medio" = net ]; then
		db_get backharddi/net/server
		server="$RET"
		db_get backharddi/net/service_port
		service_port="$RET"
		if [ x"$service_port" = x ]; then
			service_port=4600
		fi
		wget -q -O - http://$server:$service_port/getBackupMetadata?backup=$(echo $1 | escape_string) | tar xz -C /target
	fi
	
	cd $1
	devices=false
	for dev in *; do
		[ -d $dev ] || continue
		[ -f $dev/device ] || continue
		[ -f $dev/size ] || continue
		[ -f $dev/model ] || continue
		if [ ! -d $DEVICES/$dev ]; then
			db_subst backharddi/no_dev dev $(device_name $dev)
			db_input critical backharddi/no_dev || true
			db_go || exit
			continue
		fi
		backharddi_dev=$(backharddi_device $dev)
		if [ $backharddi_dev != no ]; then
			backharddi="${backharddi_dev#$DEVICES/$dev/}"
			if [ ! -d $dev/$backharddi ]; then
				db_input critical backharddi/no_backharddi || true
				db_go || true
				exit
			fi
		fi
		if [ -f $dev/pt ]; then
			if [ "$(cat $dev/size)" -gt "$(cat $DEVICES/$dev/size)" ]; then
				db_subst backharddi/no_space dev $(device_name $dev)
				db_subst backharddi/no_space dev2 $(device_name $DEVICES/$dev)
				db_input critical backharddi/no_space || true
				db_go || true
				continue
			fi
			restore_mbr $dev $1
			devices=true
		fi
	done
	[ $devices = true ]
}

exec 2>>$LOG
echo "Script: $0" >&2
echo "********************************************************************************" >&2
set -xv
