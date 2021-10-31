# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2009-2013 OpenWrt.org

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

KERNEL_LOADADDR:=0x8000

define Device/kernel-size-migration
  DEVICE_COMPAT_VERSION := 2.0
  DEVICE_COMPAT_MESSAGE := Partition design has changed compared to \
	older versions (up to 21.02) due to kernel size restrictions. \
	Upgrade via sysupgrade mechanism is not possible, so new \
	installation via factory style image is required.
endef

define Device/Default
  PROFILES := Default
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

define Device/zte_f660
  DEVICE_VENDOR := ZTE
  DEVICE_MODEL := F660
  DEVICE_DTS := avanta-f660
  DEVICE_PACKAGES := kmod-fs-ext4 kmod-usb2 kmod-usb-storage \
  	kmod-leds-gpio kmod-ledtrig-activity kmod-ledtrig-audio \
  	kmod-ledtrig-gpio kmod-ledtrig-oneshot kmod-ledtrig-transient \
	kmod-gpio-button-hotplug kmod-rtl8192ce swconfig wpad
  PAGESIZE := 512
  SUBPAGESIZE := 256
  BLOCKSIZE := 16k
  KERNEL_SIZE := 3072k
  KERNEL_IN_UBI :=
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := 31744k
  IMAGE/factory.bin := append-kernel | pad-to $$$$(KERNEL_SIZE) | append-ubi | \
	check-size
endef
TARGET_DEVICES += zte_f660

define Device/hqw_hgg420n
  DEVICE_VENDOR := HQW
  DEVICE_MODEL := HGG420N
  DEVICE_DTS := avanta-hgg420n
  DEVICE_PACKAGES := kmod-fs-ext4 kmod-usb2 kmod-usb-storage \
  	kmod-leds-gpio kmod-ledtrig-activity kmod-ledtrig-audio \
  	kmod-ledtrig-gpio kmod-ledtrig-oneshot kmod-ledtrig-transient \
	kmod-gpio-button-hotplug kmod-rtl8192ce swconfig wpad
  PAGESIZE := 512
  SUBPAGESIZE := 256
  BLOCKSIZE := 16k
  KERNEL_SIZE := 3072k
  KERNEL_IN_UBI :=
  UBINIZE_OPTS := -E 5
  IMAGE_SIZE := 64512k
  IMAGE/factory.bin := append-kernel | pad-to $$$$(KERNEL_SIZE) | append-ubi | \
	check-size
endef
TARGET_DEVICES += hqw_hgg420n

$(eval $(call BuildImage))
