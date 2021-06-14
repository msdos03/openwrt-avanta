#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/MI424WR
  NAME:=Actiontec MI-424WR Rev I
  PACKAGES:= \
	kmod-usb2 kmod-usb-storage \
	kmod-leds-gpio kmod-ledtrig-netdev \
	kmod-ledtrig-usbdev wpad-mini \
	swconfig uboot-envtools
endef

define Profile/MI424WR/Description
 Package set compatible with Actiontec MI-424WR rev. I.
endef

MI424WR_UBIFS_OPTS:="-m 512 -e 15872 -c 8192"
MI424WR_UBI_OPTS:="-m 512 -p 16KiB -s 256"

$(eval $(call Profile,MI424WR))
