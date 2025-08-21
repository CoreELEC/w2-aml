VERSION_PATCHLEVEL = $(shell printf "%d%03d" $(VERSION) $(PATCHLEVEL))

# FIXME: remove CONFIG_AML_USE_TASK, always use tasklet to handle PCIE IRQ
ifeq ($(shell test $(VERSION_PATCHLEVEL) -ge 5004; echo $$?),0)
# by default, Use task to handle PCIe IRQ if kernel >= 5.4
CONFIG_AML_USE_TASK ?= y
else
# always use tasklet to handle PCIe IRQ if kernel is 4.9 or lower
CONFIG_AML_USE_TASK ?= n
endif

# Enable A-MSDU support (need FW support)
## Select this if FW is compiled with AMSDU support
CONFIG_AML_SPLIT_TX_BUF ?= y
## Select this TO send AMSDU
CONFIG_AML_AMSDUS_TX ?= y

# Enable BFMER support (need FW support)
CONFIG_AML_BFMER ?= y

# Support of MU-MIMO transmission (need FW support)
ifeq ($(CONFIG_AML_BFMER), y)
CONFIG_AML_MUMIMO_TX ?= n
else
CONFIG_AML_MUMIMO_TX = n
endif

# Enable suspend mode
CONFIG_AML_SUSPEND ?= y

# Enable handling of radar event
CONFIG_AML_RADAR ?= y

# Enable HW queue for Broadcast/Multicast traffic (need FW support)
CONFIG_AML_BCMC ?= y

# Enable Monitor+Data interface support (need FW support)
CONFIG_AML_MON_DATA ?= n

# extra DEBUG config
CONFIG_AML_SW_PROFILING ?= n
CONFIG_AML_PRODUCT ?= W2
CONFIG_AML_LOG_BUILD_LEVEL ?= LOGLEVEL_DEBUG

#Enable LA (for non-PCIE platform only)
#should make PROJ=SDIO VER=LA for firmware also
CONFIG_AML_LA ?= n

#CSA mode scc
CONIG_AML_CSA_MODE ?= y

#usb large page
CONFIG_AML_USB_LARGE_PAGE ?= y

#
# For PCIE:
#  * firmware reorder is the only way.
#
# For SDIO/USB:
#  * host reorder is mandatory.
#  * firmware reorder is for internal test.
#  If driver is built with CONFIG_AML_SDIO_USB_FW_REORDER=y, then it supports both of them.
#  that's to say, the driver follows firmware's setting.
#
#  PS: To enable firmware reorder, build SDIO/USB firmware with FW_REORDER=y
#      (e.g. make PROJ=SDIO FW_REORDER=y),
#      by default, SDIO/USB firmware supports host reorder. (e.g. make PROJ=USB)
#
# FIXME: remove CONFIG_AML_SDIO_USB_FW_REORDER and related code in the future
#
CONFIG_AML_SDIO_USB_FW_REORDER ?= y

#Enable APF (Android Packet Filter)
ifeq ($(CONFIG_AML_ANDROID),16)
CONFIG_AML_APF ?= y
endif

obj-m := w2.o
w2-y := ../aml_msg_tx.o        \
        ../aml_msg_rx.o        \
        ../aml_reorder.o       \
        ../aml_sdio_usb_rx.o   \
        ../aml_utils.o         \
        ../aml_cmds.o          \
        ../aml_irqs.o          \
        ../aml_cfg.o           \
        ../aml_strs.o          \
        aml_rx.o            \
        aml_tx.o            \
        ../aml_txq.o           \
        aml_main.o          \
        ../aml_mod_params.o    \
        aml_mesh.o          \
        ../aml_platform.o      \
        ../ipc_host.o          \
        aml_tdls.o          \
        ../hal_desc.o          \
        ../aml_prealloc.o      \
        ../aml_regdom.o        \
        ../aml_android.o       \
        ../aml_iwpriv_cmds.o   \
        ../aml_rate.o          \
        ../aml_p2p.o           \
        ../aml_scc.o           \
        ../aml_wq.o            \
        ../aml_recy.o          \
        ../aml_tcp_ack.o       \
        ../aml_task.o          \
        ../aml_rps.o           \
        ../aml_sap.o           \
        ../aml_mdns_offload.o  \
        ../aml_cfgvendor.o     \
        ../aml_fw_trace.o

obj-m += w2_comm.o
w2_comm-objs := \
        ../aml_bus/aml_interface.o \
        ../aml_bus/usb_common.o    \
        ../aml_bus/sdio_common.o   \
        ../aml_bus/aml_w2_pci.o    \
        ../aml_bus/aml_w2_v7.o     \
        ../aml_bus/w2_sdio.o       \
        ../aml_bus/aml_static_buf.o \
        ../aml_bus/w2_usb.o

w2-$(CONFIG_AML_RADAR)       += ../aml_radar.o
w2-$(CONFIG_DEBUG_FS)         += ../aml_debugfs.o
w2-$(CONFIG_DEBUG_FS)         += ../aml_fw_dump.o
w2-$(CONFIG_NL80211_TESTMODE) += ../aml_testmode.o
w2-$(CONFIG_AML_BFMER)       += ../aml_bfmer.o
w2-$(CONFIG_AML_MUMIMO_TX)   += ../aml_mu_group.o

ccflags-y := -DCONFIG_AML_FULLMAC
ifneq ($(CUR_DIR),)
src :=$(CUR_DIR)/fullmac
endif
ccflags-y += -I$(src)
ccflags-y += -I$(src)/..
ccflags-y += -I$(src)/../aml_bus
ccflags-y += -I$(src)/../../common
ifneq ($(CONFIG_AML_PRODUCT),)
ccflags-y += -DCONFIG_AML_LOG_PREFIX=\"[$(CONFIG_AML_PRODUCT)]\"
endif
ccflags-$(CONFIG_AML_USE_TASK) += -DCONFIG_AML_USE_TASK
ccflags-$(CONFIG_AML_RADAR) += -DCONFIG_AML_RADAR
ccflags-$(CONFIG_AML_MON_DATA) += -DCONFIG_AML_MON_DATA
ccflags-$(CONFIG_AML_BFMER) += -DCONFIG_AML_BFMER
ccflags-$(CONFIG_AML_SPLIT_TX_BUF) += -DCONFIG_AML_SPLIT_TX_BUF
ifeq ($(CONFIG_AML_SPLIT_TX_BUF), y)
ccflags-$(CONFIG_AML_AMSDUS_TX) += -DCONFIG_AML_AMSDUS_TX
endif
ccflags-$(CONFIG_AML_LOG_BUILD_LEVEL) += -DCONFIG_AML_LOG_BUILD_LEVEL=$(CONFIG_AML_LOG_BUILD_LEVEL)
ccflags-$(CONFIG_AML_SW_PROFILING) += -DCONFIG_AML_SW_PROFILING
ccflags-$(CONFIG_AML_MUMIMO_TX) += -DCONFIG_AML_MUMIMO_TX
ccflags-$(CONFIG_AML_SUSPEND) += -DCONFIG_AML_SUSPEND
ccflags-$(CONFIG_AML_LA) += -DCONFIG_AML_LA
ccflags-$(CONIG_AML_CSA_MODE) += -DCONIG_AML_CSA_MODE
ccflags-$(CONFIG_AML_USB_LARGE_PAGE) += -DCONFIG_AML_USB_LARGE_PAGE
ccflags-$(CONFIG_AML_SDIO_USB_FW_REORDER) += -DCONFIG_AML_SDIO_USB_FW_REORDER
ccflags-$(CONFIG_AML_APF) += -DCONFIG_AML_APF

ifeq ($(CONFIG_AML_MUMIMO_TX), y)
ccflags-y += -DCONFIG_USER_MAX=2
else
ccflags-y += -DCONFIG_USER_MAX=1
endif

ifeq ($(CONFIG_AML_BCMC), y)
ccflags-y += -DNX_TXQ_CNT=5
else
ccflags-y += -DNX_TXQ_CNT=4
endif

#for buildroot
ifeq ($(CONFIG_BUILDROOT), y)
ccflags-y += -DCONFIG_BUILDROOT
ccflags-y += -Wno-error=format=
ccflags-y += -Wno-format
ccflags-y += -Wno-error=implicit-fallthrough=
ccflags-y += -Wno-error=vla
ccflags-y += -Wno-error=overflow
endif

# for host debug
ccflags-y += -DDEBUG_CODE
# supplicant scc mode config
#ccflags-y += -DSUPPLICANT_SCC_MODE
# drv scc mode for p2p
ccflags-y += -DDRV_P2P_SCC_MODE
#hostapd cannot handle sta+softap scc mode,so do sta_softap scc in fw&drv
ccflags-y += -DSCC_STA_SOFTAP

# FIXME: remove the following lines, treat any warning as error
ifeq ($(shell test $(VERSION_PATCHLEVEL) -eq 6012; echo $$?),0)
ccflags-y += -Wno-error
endif

# kernel-4.9 compile
ifeq ($(shell test $(VERSION_PATCHLEVEL) -eq 4009 ; echo $$?),0)
ccflags-y += -Wno-error=array-bounds
endif

ifeq ($(CONFIG_PT_MODE),y)
# FIXME: relax PT build for now
ccflags-y += -Wno-error
endif

# For old kernel (<=3.19)
ifeq ($(shell test $(VERSION_PATCHLEVEL) -le 3019 \
                   -a "$(CONFIG_VENDOR_AML)" = y ; echo $$?),0)
ccflags-y += -DCONFIG_VENDOR_AML_VHT_NO80
endif

build_dir = $(if $(KBUILD_EXTMOD),,$(srctree)/)$(src)/..
quiet_cmd_genvers = GENVERSION $@
      cmd_genvers = ($(build_dir)/mkvers.sh $(build_dir)/aml_version_gen.h $@)

$(obj)/aml_main.o: $(obj)/aml_version_gen.h

# To avoid always regenerating aml_version_gen.h (i.e. depend on FORCE), add
# dependency on all object (which already include dependency on headers files).
# Still Cannot add dependency on aml_main.o otherwise this will create circular
# dependency, so instead add dependency only on aml_main.c. This will fail to
# regenerate aml_version_gen.h if dependency of only aml_main.c is changed.
$(obj)/aml_version_gen.h: $(addprefix $(obj)/,$(filter-out aml_main.o, $(aml_fdrv-y))) $(src)/aml_main.c
	$(call cmd,genvers)

clean-files := aml_version_gen.h
