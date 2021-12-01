# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2009-2015 OpenWrt.org

include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=avanta
BOARDNAME:=Marvell Avanta
FEATURES:=usb nand squashfs ramdisk
CPU_TYPE:=xscale

KERNEL_PATCHVER:=5.10

include $(INCLUDE_DIR)/target.mk

KERNELNAME:=zImage dtbs

DEFAULT_PACKAGES += uboot-envtools

$(eval $(call BuildTarget))
