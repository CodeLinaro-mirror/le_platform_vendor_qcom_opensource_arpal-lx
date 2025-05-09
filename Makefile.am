pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pal.pc
EXTRA_DIST = $(pkgconfig_DATA)


if BUILDSYSTEM_OPENWRT

AM_CPPFLAGS := -I $(srcdir)/stream/inc
AM_CPPFLAGS += -I $(srcdir)/device/inc
AM_CPPFLAGS += -I $(srcdir)/session/inc
AM_CPPFLAGS += -I $(srcdir)/session/SessionAR/inc
AM_CPPFLAGS += -I $(srcdir)/context_manager/inc
AM_CPPFLAGS += -I $(srcdir)/plugins/PluginManager/inc
AM_CPPFLAGS += -I $(srcdir)/resource_manager/inc
AM_CPPFLAGS += -I $(srcdir)/utils/inc
AM_CPPFLAGS += -I $(srcdir)/inc
AM_CPPFLAGS += -I $(srcdir)

AM_CPPFLAGS += $(SPF_CFLAGS)/
AM_CPPFLAGS += $(ACDBDATA_CFLAGS)
AM_CPPFLAGS += -I $(TOPDIR)/src/audio/mm-audio-headers/capiv2_api/
AM_CPPFLAGS += -I $(TOPDIR)/system/media/audio_route/include/


pal_sources = ./stream/src/Stream.cpp \
              ./device/src/Device.cpp \
              ./session/src/Session.cpp \
              ./context_manager/src/ContextManager.cpp \
              ./resource_manager/src/SndCardMonitor.cpp \
              ./resource_manager/src/ResourceManager.cpp \
              ./Pal.cpp \
              ./utils/src/SoundTriggerPlatformInfo.cpp \
              ./utils/src/MetadataParser.cpp \
              ./utils/src/VoiceUIPlatformInfo.cpp \
              ./utils/src/AudioHapticsInterface.cpp \
              ./utils/src/ACDPlatformInfo.cpp \
              ./utils/src/STUtils.cpp
else
AM_CPPFLAGS := -I $(top_srcdir)/stream/inc
AM_CPPFLAGS += -I $(top_srcdir)/device/inc
AM_CPPFLAGS += -I $(top_srcdir)/session/inc
AM_CPPFLAGS += -I $(top_srcdir)/session/SessionAR/inc
AM_CPPFLAGS += -I $(top_srcdir)/plugins/PluginManager/inc
AM_CPPFLAGS += -I $(top_srcdir)/resource_manager/inc
AM_CPPFLAGS += -I $(top_srcdir)/utils/inc
AM_CPPFLAGS += -I $(top_srcdir)/context_manager/inc
AM_CPPFLAGS += -I $(top_srcdir)/inc
AM_CPPFLAGS += -I $(top_srcdir)/

pal_sources = $(top_srcdir)/stream/src/Stream.cpp \
              $(top_srcdir)/device/src/Device.cpp \
              $(top_srcdir)/session/src/Session.cpp \
              $(top_srcdir)/context_manager/src/ContextManager.cpp \
              $(top_srcdir)/resource_manager/src/SndCardMonitor.cpp \
              $(top_srcdir)/resource_manager/src/ResourceManager.cpp \
              $(top_srcdir)/Pal.cpp \
              $(top_srcdir)/utils/src/SoundTriggerPlatformInfo.cpp \
              $(top_srcdir)/utils/src/MetadataParser.cpp \
              $(top_srcdir)/utils/src/VoiceUIPlatformInfo.cpp \
              $(top_srcdir)/utils/src/AudioHapticsInterface.cpp \
              $(top_srcdir)/utils/src/ACDPlatformInfo.cpp \
              $(top_srcdir)/utils/src/STUtils.cpp

endif

AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED -DPAL_MEMLOG_UNSUPPORTED -DPAL_SIGNAL_HANDLER_UNSUPPORTED -DA2DP_SINK_SUPPORTED
AM_CPPFLAGS += -DCONFIG_GSL
AM_CPPFLAGS += @AGM_CFLAGS@
AM_CPPFLAGS += @SPF_CFLAGS@
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@
AM_CPPFLAGS += @AROUTE_CFLAGS@
AM_CPPFLAGS += @MMHEADERS_CFLAGS@
AM_CPPFLAGS += @PALHEADERS_CFLAGS@
AM_CPPFLAGS += -Wno-sign-compare -Wno-unused-result -Wl,-z,defs
AM_CPPFLAGS += -DSND_AUDIOCODEC_ALAC=0x00000020 -DSND_AUDIOCODEC_APE=0x00000021
AM_CPPFLAGS += -DFEATURE_IPQ_OPENWRT


library_includedir = $(includedir)/pal
lib_LTLIBRARIES     = libpal.la
libpal_la_SOURCES   = $(pal_sources)
if BUILDSYSTEM_OPENWRT
libpal_la_LIBADD    = $(GLIB_LIBS) -ltinyalsa -laudioroute -lar-osal -lexpat -ltinycompress -lagmclientwrapper
libpal_la_LIBADD   += -lar-gsl -lacdbdata
else
libpal_la_LIBADD    = $(GLIB_LIBS) -ltinyalsa -laudioroute -lar-osal -lexpat -ltinycompress
endif
if IS_SDXLEMUR
libpal_la_LIBADD    += -lagmclientwrapper
endif
if IS_SA525M
libpal_la_LIBADD    += -lagmclientwrapper
endif
libpal_la_CPPFLAGS := $(AM_CPPFLAGS)
libpal_la_CPPFLAGS += -std=c++14
libpal_la_LDFLAGS   = -shared -avoid-version
libpal_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"

if USE_GLIB
libpal_la_LIBADD += -lglib-2.0
libpal_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libpal_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif

if BUILDSYSTEM_OPENWRT
libpal_la_LIBADD += -lglib-2.0
endif
if COMPILE_COMPRESS
libpal_la_CPPFLAGS += -DSND_COMPRESS_DEC_HDR
endif

if USE_SYSLOG
libpal_la_CPPFLAGS += -DPAL_USE_SYSLOG
endif

libpal_la_LIBADD   += -L${abs_top_builddir}/plugins/vui_interface/.libs -lvui_intf
libpal_la_LIBADD   += -L${abs_top_builddir}/plugins/PluginManager/.libs -lplugin_manager

SUBDIRS = plugins . session stream device
