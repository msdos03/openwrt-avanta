#
# Copyright (C) 2014 OpenWrt.org
#

. /lib/kirkwood.sh

RAMFS_COPY_DATA=/lib/kirkwood.sh

platform_check_image() {
	local board="$(kirkwood_board_name)"

	[ "$#" -gt 1 ] && return 1

	case "$board" in
	"mi424wr")
		nand_do_platform_check $board $1
		return $?;
		;;
	esac

	echo "Sysupgrade is not yet supported on $board."
	return 1
}
