# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2009-2013 OpenWrt.org

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

KERNEL_LOADADDR:=0x8000

define Device/kernel-size-migration
  DEVICE_COMPAT_VERSION := 2.0
  DEVICE_COMPAT_MESSAGE := \n$\
   !The partitioning of the router has changed! \n$\
  Partition design has changed compared to \
	older versions (up to 23.05) due to kernel size restrictions. \
	Upgrade via sysupgrade mechanism is not possible, so new \
	installation via factory image is required.\n$\
  Please adjust bootcmd in u-boot console:\n$\
  setenv bootcmd \
  'nand read 2000000 100000 400000;bootm'\n$\
  saveenv\n$\
  and reinstall image with factory image. \
  Please see original instructions for more details.
endef

define Device/Default
  PROFILES := Default
ifdef CONFIG_LINUX_6_6
  DEVICE_DTS_DIR := $(DTS_DIR)/marvell
endif
  DEVICE_DTS = avanta-$(lastword $(subst _, ,$(1)))
  KERNEL_DEPENDS = $$(wildcard $(DTS_DIR)/$$(DEVICE_DTS).dts)
  KERNEL := kernel-bin | append-dtb | uImage none
  KERNEL_NAME := zImage
  KERNEL_SUFFIX  := -uImage
  KERNEL_IN_UBI := 1

  PAGESIZE := 2048
  SUBPAGESIZE := 512
  BLOCKSIZE := 128k
  IMAGES := sysupgrade.bin factory.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
  IMAGE/factory.bin := append-ubi
endef

define Device/zte_f420-f620
  $(Device/kernel-size-migration)
  DEVICE_VENDOR := ZTE
  DEVICE_MODEL := F420-F620
  DEVICE_DTS := avanta-f420-f620
  DEVICE_PACKAGES := kmod-gpio-button-hotplug kmod-rtc-mv
  PAGESIZE := 512
  SUBPAGESIZE := 256
  BLOCKSIZE := 16k
  KERNEL_SIZE := 4096k
  KERNEL_IN_UBI :=
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := 31744k
  IMAGE/factory.bin := append-kernel | pad-to $$$$(KERNEL_SIZE) | append-ubi | \
	check-size
endef
TARGET_DEVICES += zte_f420-f620

define Device/zte_f460-f660
  $(Device/kernel-size-migration)
  DEVICE_VENDOR := ZTE
  DEVICE_MODEL := F460-F660
  DEVICE_DTS := avanta-f460-f660
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-storage kmod-usb-ledtrig-usbport \
	kmod-gpio-button-hotplug kmod-rtl8192ce kmod-rtc-mv
  PAGESIZE := 512
  SUBPAGESIZE := 256
  BLOCKSIZE := 16k
  KERNEL_SIZE := 4096k
  KERNEL_IN_UBI :=
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := 31744k
  IMAGE/factory.bin := append-kernel | pad-to $$$$(KERNEL_SIZE) | append-ubi | \
	check-size
endef
TARGET_DEVICES += zte_f460-f660

define Device/hqw_hgg420n
  $(Device/kernel-size-migration)
  DEVICE_VENDOR := HQW
  DEVICE_MODEL := HGG420N
  DEVICE_DTS := avanta-hgg420n
  DEVICE_PACKAGES := kmod-usb2 kmod-usb-storage kmod-usb-ledtrig-usbport \
	kmod-gpio-button-hotplug kmod-rtl8192ce kmod-rtc-mv
  PAGESIZE := 512
  SUBPAGESIZE := 256
  BLOCKSIZE := 16k
  KERNEL_SIZE := 4096k
  KERNEL_IN_UBI :=
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := 64512k
  IMAGE/factory.bin := append-kernel | pad-to $$$$(KERNEL_SIZE) | append-ubi | \
	check-size
endef
TARGET_DEVICES += hqw_hgg420n

$(eval $(call BuildImage))
