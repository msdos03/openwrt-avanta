# Use the default kernel version if the Makefile doesn't override it

LINUX_RELEASE?=1

<<<<<<< HEAD
LINUX_VERSION-3.14 = .79
LINUX_VERSION-3.18 = .11
LINUX_VERSION-4.0 = .1

LINUX_KERNEL_MD5SUM-3.14.79 = ec5b09d8ad2ebf92e6f51a727a338559
LINUX_KERNEL_MD5SUM-3.18.11 = 2def91951c9cedf7896efb864e0c090c
=======
LINUX_VERSION-3.18 = .14
LINUX_VERSION-4.0 = .1

LINUX_KERNEL_MD5SUM-3.18.14 = cb6f534b83333ba52f1fed7979824a1b
>>>>>>> 9157f62... kernel: update 3.18 to 3.18.14
LINUX_KERNEL_MD5SUM-4.0.1 = ea7fc80310be8a5b43b2c6dfa5c4169f

ifdef KERNEL_PATCHVER
  LINUX_VERSION:=$(KERNEL_PATCHVER)$(strip $(LINUX_VERSION-$(KERNEL_PATCHVER)))
endif

split_version=$(subst ., ,$(1))
merge_version=$(subst $(space),.,$(1))
KERNEL_BASE=$(firstword $(subst -, ,$(LINUX_VERSION)))
KERNEL=$(call merge_version,$(wordlist 1,2,$(call split_version,$(KERNEL_BASE))))
KERNEL_PATCHVER ?= $(KERNEL)

# disable the md5sum check for unknown kernel versions
LINUX_KERNEL_MD5SUM:=$(LINUX_KERNEL_MD5SUM-$(strip $(LINUX_VERSION)))
LINUX_KERNEL_MD5SUM?=x
