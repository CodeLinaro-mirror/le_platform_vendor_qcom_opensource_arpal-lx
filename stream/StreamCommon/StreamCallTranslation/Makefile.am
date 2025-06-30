pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = stream_calltranslation.pc
EXTRA_DIST = $(pkgconfig_DATA)


AM_CPPFLAGS := -I ./inc
AM_CPPFLAGS += -I ../inc
AM_CPPFLAGS += -I $(top_srcdir)/inc
AM_CPPFLAGS += -I $(top_srcdir)/
AM_CPPFLAGS += -I $(top_srcdir)/utils/inc
AM_CPPFLAGS += -I $(top_srcdir)/device/inc
AM_CPPFLAGS += -I $(top_srcdir)/session/inc
AM_CPPFLAGS += -I $(top_srcdir)/stream/inc
AM_CPPFLAGS += -I $(top_srcdir)/session/SessionAR/inc
AM_CPPFLAGS += -I $(top_srcdir)/plugins/PluginManager/inc
AM_CPPFLAGS += -I $(top_srcdir)/resource_manager/inc
AM_CPPFLAGS += -I $(top_srcdir)/context_manager/inc
AM_CPPFLAGS += -I $(top_srcdir)/plugins/vui_interface/api/vui-interface

stream_calltranslation_sources = ./src/StreamCallTranslation.cpp

library_includedir = $(includedir)/stream_calltranslation

lib_LTLIBRARIES     = libstream_calltranslation.la
libstream_calltranslation_la_SOURCES   = $(stream_calltranslation_sources)
libstream_calltranslation_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -L${abs_top_builddir}/stream/StreamCommon/.libs -lstream_common
libstream_calltranslation_la_CPPFLAGS := $(AM_CPPFLAGS)
libstream_calltranslation_la_CPPFLAGS += -std=c++14
libstream_calltranslation_la_LDFLAGS   = -shared -avoid-version
libstream_calltranslation_la_CPPFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libstream_calltranslation_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libstream_calltranslation_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libstream_calltranslation_la_LIBADD += -lglib-2.0
endif