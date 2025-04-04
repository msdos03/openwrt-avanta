RAMFS_COPY_BIN='fw_printenv fw_setenv strings'
RAMFS_COPY_DATA='/etc/fw_env.config /var/lock/fw_printenv.lock'

PART_NAME=firmware
REQUIRE_IMAGE_METADATA=1

platform_check_image() {
	local board="$(board_name)"

	case "$board" in
	*)
		return 0
		;;
	esac
}

platform_do_upgrade() {
	local board="$(board_name)"

	case "$board" in
	*)
		nand_do_upgrade "$1"
		;;
	esac
}
