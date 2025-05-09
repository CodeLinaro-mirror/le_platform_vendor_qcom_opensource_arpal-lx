pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = plugin_manager.pc
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
AM_CPPFLAGS += -I $(srcdir)/../../api/vui-interface
AM_CPPFLAGS += @MMHEADERS_CFLAGS@

plugin_manager_sources = ./src/PluginManager.cpp

library_includedir = $(includedir)/plugin_manager

lib_LTLIBRARIES     = libplugin_manager.la
libplugin_manager_la_SOURCES   = $(plugin_manager_sources)
libplugin_manager_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat
libplugin_manager_la_CPPFLAGS := $(AM_CPPFLAGS)
libplugin_manager_la_CPPFLAGS += -std=c++14
libplugin_manager_la_LDFLAGS   = --disable-shared -avoid-version
libplugin_manager_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libplugin_manager_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libplugin_manager_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libplugin_manager_la_LIBADD += -lglib-2.0
libplugin_manager_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libplugin_manager_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif

