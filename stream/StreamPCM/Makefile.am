pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = stream_pcm.pc
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
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@


stream_pcm_sources = ./src/StreamPCM.cpp

library_includedir = $(includedir)/stream_pcm

lib_LTLIBRARIES     = libstream_pcm.la
libstream_pcm_la_SOURCES   = $(stream_pcm_sources)
libstream_pcm_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat
libstream_pcm_la_LIBADD   += -L${abs_top_builddir}/.libs -lpal
libstream_pcm_la_CPPFLAGS := $(AM_CPPFLAGS)
libstream_pcm_la_CPPFLAGS += -std=c++14
libstream_pcm_la_LDFLAGS   = -shared -avoid-version
libstream_pcm_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libstream_pcm_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED


if BUILDSYSTEM_OPENWRT
libstream_pcm_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libstream_pcm_la_LIBADD += -lglib-2.0
libstream_pcm_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libstream_pcm_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
