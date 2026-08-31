AML_VERS_NUM=v2.1.0_2025-07.18-w2 driver:c8090ae2
# KERNELDIR ?= /usr/lib/modules/$(shell uname -r)/build
PWD=$(shell pwd)
ifeq ($(ARCH),)
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
else ifeq ($(ARCH),arm)
SYSTEM_TYPE = SYSTEM32
CROSS_COMPILE ?= arm-linux-gnueabihf-
EXTRA_CFLAGS += -DSYSTEM32
else ifeq ($(ARCH),arm64)
SYSTEM_TYPE = SYSTEM64
CROSS_COMPILE ?= aarch64-linux-gnu-
EXTRA_CFLAGS += -DSYSTEM64
endif


ifeq ($(PROJ_NAME),p-amlogic)
SRCTOP ?= /proj/vlsi.wifi/p-amlogic
KERNELDIR ?= $(SRCTOP)/out/target/product/newton/obj/KERNEL_OBJ
EXTRA_CFLAGS += -DSYSTEM64
else ifeq ($(PROJ_NAME),yeke)
SRCTOP ?= /proj/vlsi.wifi/yeke_64bit
KERNELDIR ?= $(SRCTOP)/out/target/product/p281/obj/KERNEL_OBJ
EXTRA_CFLAGS += -DSYSTEM64
else ifeq ($(PROJ_NAME),9-xiaomi)
SRCTOP ?= /proj/vlsi.wifi/9-xiaomi
KERNELDIR ?= $(SRCTOP)/out/target/product/aquaman/obj/KERNEL_OBJ
EXTRA_CFLAGS += -DSYSTEM64
else ifeq ($(PROJ_NAME),gva)
SRCTOP ?= /proj/vlsi.wifi/gva
KERNELDIR ?= $(SRCTOP)/kernel-4.9/common
EXTRA_CFLAGS += -DSYSTEM64
else
ifeq ($(SYSTEM_TYPE),)
ARCH = arm
CROSS_COMPILE = arm-linux-gnueabihf-
EXTRA_CFLAGS += -DSYSTEM32
endif
SRCTOP ?= /proj/vlsi.wifi/p212_32bit
KERNELDIR ?= $(SRCTOP)/out/target/product/ampere/obj/KERNEL_OBJ
endif

CC        ?= $(CROSS_COMPILE)gcc
STRIP     ?= $(CROSS_COMPILE)strip

M ?= $(shell pwd)
ifneq ($(KERNEL_SRC),)
CUR_DIR  := $(KERNEL_SRC)/$(M)
else
CUR_DIR  := $(M)
endif
EXT_INCS := $(CUR_DIR)/../common
INCS     := -I$(CUR_DIR) -I$(EXT_INCS)
KERNEL_SRC ?= $(KERNELDIR)
TARGET = vlsicomm

#
# WAITING FOR KCONFIG {
#
CONFIG_AML_SOFTMAC ?= n
CONFIG_AML_FULLMAC ?= m
CONFIG_AML_FHOST ?= n

#
# DEBUG OPTIONS
CONFIG_AML_UM_HELPER_DFLT ?= "/usr/bin/aml_umh.sh"

# Support of P2P DebugFS for enabling/disabling NoA and OppPS
CONFIG_AML_P2P_DEBUGFS ?= y

# Need open it when use FPGA pcie version
CONFIG_AML_FPGA_PCIE ?= n

# Product test mode
CONFIG_PT_MODE ?= n

# Amlogic specified android(/w Kernel) platform depends
CONFIG_AML_PLATFORM_ANDROID ?= y

ifeq ($(CONFIG_AML_KERNEL_4_9)$(CONFIG_AML_PLATFORM_ANDROID),yy)
CONFIG_AML_SDIO_IRQ_VIA_GPIO ?= n
else
CONFIG_AML_SDIO_IRQ_VIA_GPIO := n
endif

# Use platform random mac
CONFIG_AML_PLATFORM_RANDOM_MAC ?= n

# Support to use preallocing static buffer for wlan driver reloadable
CONFIG_AML_PREALLOC_BUF_STATIC ?= y

# Support to use preallocing buffer for debug SKB alloc failed
CONFIG_AML_PREALLOC_BUF_SKB ?= n

# Config to enable WFA certification mode
CONFIG_AML_WFA_CERT_MODE ?= n

# Config to enable Power_Save mode
CONFIG_AML_POWER_SAVE_MODE ?= n

# Config to enable GOOGLE_CAST wake host
CONFIG_AML_WOW_GOOGLE_CAST_EN ?= n

# Config to enable MAGIC_PKT wake host
CONFIG_AML_WOW_MAGIC_PACKET_EN ?= n

# Support recovery function
CONFIG_AML_RECOVERY ?= y
# } // WAITING FOR KCONFIG
#

# Support rx minisize function
CONFIG_AML_RX_MINISIZE ?= n

CONFIG_ND_RANDOM_MAC_ADDR ?= n

#support NAPI
CONFIG_AML_NAPI ?= y

# support MDNS_OFFLOAD feature
CONFIG_MDNS_OFFLOAD_FEATURE ?= y

# Support 4.9 ax
ifeq ($(CONFIG_AML_KERNEL_4_9),y)
CONFIG_KERNEL_AX_PATCH ?= y
endif
# } // NEED 4.9 PATCH
#

#support customer 11d forbidden
CONFIG_CUSTOMER_11D_FORBIDDEN ?= y

CONFIG_LIGHT_WEIGHT_MEM ?=y

#support window size scale set
CONFIG_TCP_WIN_SCALE ?= n

ifeq ($(CONFIG_GOOGLE_CAST_EN),y)
CONFIG_AML_WOW_GOOGLE_CAST_EN = y
endif

ifeq ($(CONFIG_MAGIC_PACKET_EN),y)
CONFIG_AML_WOW_MAGIC_PACKET_EN = y
endif

subdir-ccflags-$(CONFIG_PT_MODE) += -DCONFIG_PT_MODE
subdir-ccflags-$(CONFIG_AML_PLATFORM_ANDROID) += -DCONFIG_AML_PLATFORM_ANDROID
subdir-ccflags-$(CONFIG_DEBUG_FS) += -DCONFIG_AML_DEBUGFS
subdir-ccflags-$(CONFIG_DEBUG_FS) += -DCONFIG_AML_UM_HELPER_DFLT=\"$(CONFIG_AML_UM_HELPER_DFLT)\"
subdir-ccflags-$(CONFIG_AML_P2P_DEBUGFS) += -DCONFIG_AML_P2P_DEBUGFS
subdir-ccflags-$(CONFIG_AML_FPGA_PCIE) += -DCONFIG_AML_FPGA_PCIE
subdir-ccflags-$(CONFIG_AML_PREALLOC_BUF_STATIC) += -DCONFIG_AML_PREALLOC_BUF_STATIC
subdir-ccflags-$(CONFIG_AML_PREALLOC_BUF_SKB) += -DCONFIG_AML_PREALLOC_BUF_SKB
subdir-ccflags-$(CONFIG_AML_WFA_CERT_MODE) += -DCONFIG_AML_WFA_CERT_MODE
subdir-ccflags-$(CONFIG_AML_POWER_SAVE_MODE) += -DCONFIG_AML_POWER_SAVE_MODE
subdir-ccflags-$(CONFIG_AML_RECOVERY) += -DCONFIG_AML_RECOVERY
subdir-ccflags-$(CONFIG_AML_RX_MINISIZE) += -DCONFIG_AML_W2_RX_MINISIZE
subdir-ccflags-$(CONFIG_AML_NAPI) += -DCONFIG_AML_NAPI
subdir-ccflags-$(CONFIG_KERNEL_AX_PATCH) += -DCONFIG_KERNEL_AX_PATCH
subdir-ccflags-$(CONFIG_MDNS_OFFLOAD_FEATURE) += -DMDNS_OFFLOAD_FEATURE
subdir-ccflags-$(CONFIG_AML_WOW_GOOGLE_CAST_EN) += -DAML_WOW_GOOGLE_CAST_EN
subdir-ccflags-$(CONFIG_AML_WOW_MAGIC_PACKET_EN) += -DAML_WOW_MAGIC_PACKET_EN
subdir-ccflags-$(CONFIG_LIGHT_WEIGHT_MEM) += -DLIGHT_WEIGHT_MEM
subdir-ccflags-$(CONFIG_TCP_WIN_SCALE) += -DCONFIG_TCP_WIN_SCALE
subdir-ccflags-$(CONFIG_ND_RANDOM_MAC_ADDR) += -DCONFIG_ND_RANDOM_MAC_ADDR

# FW VARS
subdir-ccflags-y += -DNX_VIRT_DEV_MAX=4
subdir-ccflags-y += -DNX_REMOTE_STA_MAX=10
subdir-ccflags-y += -DNX_MU_GROUP_MAX=62
subdir-ccflags-y += -DNX_TX_MAX_RATES=4
subdir-ccflags-y += -DNX_CHAN_CTXT_CNT=3

ifeq ($(CONFIG_AML_PLATFORM_ANDROID),y)
subdir-ccflags-$(CONFIG_AML_SDIO_IRQ_VIA_GPIO) += -DCONFIG_AML_SDIO_IRQ_VIA_GPIO
subdir-ccflags-$(CONFIG_ANDROID_GKI) += -DCONFIG_ANDROID_GKI

ifneq ($(CONFIG_AML_ANDROID),)
subdir-ccflags-y += -DCONFIG_AML_ANDROID=$(CONFIG_AML_ANDROID)
endif
endif

ifeq ($(CONFIG_CUSTOMER_11D_FORBIDDEN),y)
subdir-ccflags-y += -DCONFIG_CUSTOMER_11D_FORBIDDEN
endif

obj-$(CONFIG_AML_FULLMAC) += fullmac/

all: modules

modules clean:
	@$(PWD)/mklink.sh
	$(MAKE) CUR_DIR=$(CUR_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(M) $@
	@$(PWD)/mklink.sh clean
#ifneq (${OUT_DIR},)
#	$(CROSS_COMPILE)strip --strip-unneeded ${OUT_DIR}/$(M)/$(TARGET).ko
#else
#	$(STRIP) -S $(TARGET).ko
#endif

modules_install:
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name "*.ko" -exec cp {} ${OUT_DIR}/../vendor_lib/modules/ \;

clean-files := aml_version_gen.h

