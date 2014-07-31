#
# Copyright (C) 2009-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=avanta
BOARDNAME:=Marvell Avanta
FEATURES:=targz usb jffs2_nand nand ubifs squashfs
CPU_TYPE:=xscale

LINUX_VERSION:=3.14.14

include $(INCLUDE_DIR)/target.mk

KERNELNAME:="zImage dtbs"

$(eval $(call BuildTarget))
