pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sva_intf.pc
EXTRA_DIST = $(pkgconfig_DATA)


AM_CPPFLAGS := -I ./inc
AM_CPPFLAGS += -I ../../../inc
AM_CPPFLAGS += -I ../../../
AM_CPPFLAGS += -I ../../../utils/inc
AM_CPPFLAGS += -I ../../../device/inc
AM_CPPFLAGS += -I ../../../session/inc
AM_CPPFLAGS += -I ../../../stream/inc
AM_CPPFLAGS += -I ../../../session/SessionAR/inc
AM_CPPFLAGS += -I ../../../plugins/PluginManager/inc
AM_CPPFLAGS += -I ../../../resource_manager/inc
AM_CPPFLAGS += -I ../../../context_manager/inc
AM_CPPFLAGS += -I ../../../api/vui-interface


sva_intf_sources += ./src/SVAInterface.cpp

library_includedir = $(includedir)/sva_intf

lib_LTLIBRARIES     = libsva_intf.la
libsva_intf_la_SOURCES   = $(sva_intf_sources)
libsva_intf_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat
libsva_intf_la_CPPFLAGS := $(AM_CPPFLAGS)
libsva_intf_la_CPPFLAGS += -std=c++14
libsva_intf_la_LDFLAGS   = -shared -avoid-version
libsva_intf_la_CPPFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libsva_intf_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsva_intf_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libsva_intf_la_LIBADD += -lglib-2.0
endif
