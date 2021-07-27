#!/bin/sh
#
# Copyright (C) 2014 OpenWrt.org
#

KIRKWOOD_BOARD_NAME=
KIRKWOOD_MODEL=

kirkwood_board_detect() {
	local machine
	local name

	machine=$(cat /proc/device-tree/model)

	case "$machine" in
	"ZTE F660")
		name="f660"
		;;
	"HQW HGG420N")
		name="hgg420n"
		;;
	*)
		name="generic"
		;;
	esac

	[ -z "$KIRKWOOD_BOARD_NAME" ] && KIRKWOOD_BOARD_NAME="$name"
	[ -z "$KIRKWOOD_MODEL" ] && KIRKWOOD_MODEL="$machine"

	[ -e "/tmp/sysinfo/" ] || mkdir -p "/tmp/sysinfo/"

	echo "$KIRKWOOD_BOARD_NAME" > /tmp/sysinfo/board_name
	echo "$KIRKWOOD_MODEL" > /tmp/sysinfo/model
}

kirkwood_board_name() {
	local name

	[ -f /tmp/sysinfo/board_name ] || kirkwood_board_detect
	[ -f /tmp/sysinfo/board_name ] && name=$(cat /tmp/sysinfo/board_name)
	[ -z "$name" ] && name="unknown"

	echo "$name"
}
