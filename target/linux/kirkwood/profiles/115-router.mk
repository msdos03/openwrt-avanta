#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/DIR665
  NAME:=D-Link DIR-665
  PACKAGES:= \
	kmod-mwl8k kmod-usb2 kmod-usb-storage \
	kmod-leds-gpio kmod-ledtrig-netdev \
	kmod-ledtrig-usbdev wpad-mini \
	swconfig
endef

define Profile/DIR665/Description
 Package set compatible with the D-Link DIR-665
endef

define Profile/EA4500
  NAME:=Linksys EA4500
  PACKAGES:= \
	kmod-mwl8k kmod-usb2 kmod-usb-storage \
	uboot-envtools
endef

define Profile/EA4500/Description
 Package set compatible with Linksys EA4500 board.
endef

EA4500_UBIFS_OPTS:="-m 2048 -e 126KiB -c 4096"
EA4500_UBI_OPTS:="-m 2048 -p 128KiB -s 512"

$(eval $(call Profile,DIR665))
$(eval $(call Profile,EA4500))
