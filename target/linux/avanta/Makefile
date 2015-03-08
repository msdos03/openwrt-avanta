#
# Copyright (C) 2009-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=avanta
BOARDNAME:=Marvell Avanta
FEATURES:=usb nand ubifs squashfs
CPU_TYPE:=xscale

KERNEL_PATCHVER:=3.14

include $(INCLUDE_DIR)/target.mk

KERNELNAME:=zImage dtbs

$(eval $(call BuildTarget))
