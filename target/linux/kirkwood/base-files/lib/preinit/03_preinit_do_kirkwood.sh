#!/bin/sh

do_kirkwood() {
	. /lib/kirkwood.sh

	kirkwood_board_detect
}

boot_hook_add preinit_main do_kirkwood
