# Put lvm-related utilities here.
# This file is sourced from test-lib.sh.

# Copyright (C) 2007, 2008 Red Hat, Inc. All rights reserved.
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

export LVM_SUPPRESS_FD_WARNINGS=1

ME=$(basename "$0")
warn() { echo >&2 "$ME: $@"; }

unsafe_losetup_()
{
  f=$1

  test -n "$G_dev_" \
    || error "Internal error: unsafe_losetup_ called before init_root_dir_"

  # Iterate through $G_dev_/loop{,/}{0,1,2,3,4,5,6,7,8,9}
  for slash in '' /; do
    for i in 0 1 2 3 4 5 6 7 8 9; do
      dev=$G_dev_/loop$slash$i
      losetup $dev > /dev/null 2>&1 && continue;
      losetup "$dev" "$f" > /dev/null && { echo "$dev"; return 0; }
      break
    done
  done

  return 1
}

loop_setup_()
{
  file=$1
  dd if=/dev/zero of="$file" bs=1M count=1 seek=1000 > /dev/null 2>&1 \
    || { warn "loop_setup_ failed: Unable to create tmp file $file"; return 1; }

  # NOTE: this requires a new enough version of losetup
  dev=$(unsafe_losetup_ "$file" 2>/dev/null) \
    || { warn "loop_setup_ failed: Unable to create loopback device"; return 1; }

  echo "$dev"
  return 0;
}

compare_vg_field_()
{
if test "$verbose" = "t"
then
  echo "compare_vg_field_ VG1: `vgs --noheadings -o $3 $1` VG2: `vgs --noheadings -o $3 $2`"
fi
  return $(test $(vgs --noheadings -o $3 $1) == $(vgs --noheadings -o $3 $2) )
}

check_vg_field_()
{
if test "$verbose" = "t"
then
  echo "check_vg_field_ VG=$1, field=$2, actual=`vgs --noheadings -o $2 $1`, expected=$3"
fi
  return $(test $(vgs --noheadings -o $2 $1) == $3)
}

check_pv_field_()
{
if test "$verbose" = "t"
then
  echo "check_pv_field_ PV=$1, field=$2, actual=`pvs --noheadings -o $2 $1`, expected=$3"
fi
  return $(test $(pvs --noheadings -o $2 $1) == $3)
}

check_lv_field_()
{
if test "$verbose" = "t"
then
  echo "check_lv_field_ LV=$1, field=$2, actual=`lvs --noheadings -o $2 $1`, expected=$3"
fi
  return $(test $(lvs --noheadings -o $2 $1) == $3)
}

vg_validate_pvlv_counts_()
{
	local local_vg=$1
	local num_pvs=$2
	local num_lvs=$3
	local num_snaps=$4

	check_vg_field_ $local_vg pv_count $num_pvs &&
	check_vg_field_ $local_vg lv_count $num_lvs &&
	check_vg_field_ $local_vg snap_count $num_snaps
}

dmsetup_has_dm_devdir_support_()
{
  # Detect support for the envvar.  If it's supported, the
  # following command will fail with the expected diagnostic.
  out=$(DM_DEV_DIR=j dmsetup version 2>&1)
  test "$?:$out" = "1:Invalid DM_DEV_DIR envvar value." -o \
       "$?:$out" = "1:Invalid DM_DEV_DIR environment variable value."
}

# set up private /dev and /etc
init_root_dir_()
{
  test -n "$test_dir_rand_" \
    || error "Internal error: called init_root_dir_ before" \
      "defining \$test_dir_rand_"

  # Define these two globals.
  G_root_=$test_dir_rand_/root
  G_dev_=$G_root_/dev

  export LVM_SYSTEM_DIR=$G_root_/etc
  export DM_DEV_DIR=$G_dev_

  # Only the first caller does anything.
  mkdir -p $G_root_/etc $G_dev_ $G_dev_/mapper
  for i in 0 1 2 3 4 5 6 7; do
    mknod $G_root_/dev/loop$i b 7 $i
  done
  cat > $G_root_/etc/lvm.conf <<-EOF
  devices {
    dir = "$G_dev_"
    scan = "$G_dev_"
    filter = [ "a/loop/", "a/mirror/", "a/mapper/", "r/.*/" ]
    cache_dir = "$G_root_/etc"
    sysfs_scan = 0
  }
EOF
}

if test $(this_test_) != 000-basic; then
  init_root_dir_
fi
