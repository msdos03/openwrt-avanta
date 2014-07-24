#!/bin/sh

do_kirkwood() {
	. /lib/kirkwood.sh

	kirkwood_board_detect
	insmod gpio-button-hotplug
}

boot_hook_add preinit_main do_kirkwood
