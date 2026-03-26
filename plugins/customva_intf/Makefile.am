pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = customva_intf.pc
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
AM_CPPFLAGS += -I $(srcdir)/../../api/vui-interface
AM_CPPFLAGS += -I $(srcdir)/../../resource_manager/inc
AM_CPPFLAGS += -I $(srcdir)/../../context_manager/inc
AM_CPPFLAGS += -I $(srcdir)/../codecs
AM_CPPFLAGS += @MMHEADERS_CFLAGS@
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@


customva_intf_sources = ./src/CustomVAInterface.cpp

library_includedir = $(includedir)/customva_intf

lib_LTLIBRARIES     = libcustomva_intf.la
libcustomva_intf_la_SOURCES   = $(customva_intf_sources)
libcustomva_intf_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat
libcustomva_intf_la_CPPFLAGS := $(AM_CPPFLAGS)
libcustomva_intf_la_CPPFLAGS += -std=c++14
libcustomva_intf_la_LDFLAGS   = -shared -avoid-version
libcustomva_intf_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libcustomva_intf_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libcustomva_intf_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libcustomva_intf_la_LIBADD += -lglib-2.0
libcustomva_intf_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libcustomva_intf_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
