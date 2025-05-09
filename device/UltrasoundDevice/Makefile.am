pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dev_ultrasound.pc
EXTRA_DIST = $(pkgconfig_DATA)


AM_CPPFLAGS := -I $(srcdir)/inc
AM_CPPFLAGS += -I $(srcdir)/../../inc
AM_CPPFLAGS += -I $(srcdir)/../../
AM_CPPFLAGS += -I $(srcdir)/../../utils/inc
AM_CPPFLAGS += -I $(srcdir)/../../device/inc
AM_CPPFLAGS += -I $(srcdir)/../../session/inc
AM_CPPFLAGS += -I $(srcdir)/../../stream/inc
AM_CPPFLAGS += -I $(srcdir)/../../session/SessionAR/inc
AM_CPPFLAGS += -I $(srcdir)/../../plugins/PluginManager/inc
AM_CPPFLAGS += -I $(srcdir)/../../resource_manager/inc
AM_CPPFLAGS += -I $(srcdir)/../../context_manager/inc
AM_CPPFLAGS += @MMHEADERS_CFLAGS@
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@


dev_ultrasound_sources = ./src/UltrasoundDevice.cpp

library_includedir = $(includedir)/dev_ultrasound

lib_LTLIBRARIES     = libdev_ultrasound.la
libdev_ultrasound_la_SOURCES   = $(dev_ultrasound_sources)
libdev_ultrasound_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa -ltinycompress
libdev_ultrasound_la_CPPFLAGS := $(AM_CPPFLAGS)
libdev_ultrasound_la_CPPFLAGS += -std=c++14
libdev_ultrasound_la_LDFLAGS   = -shared -avoid-version
libdev_ultrasound_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libdev_ultrasound_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libdev_ultrasound_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libdev_ultrasound_la_LIBADD += -lglib-2.0
libdev_ultrasound_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libdev_ultrasound_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
