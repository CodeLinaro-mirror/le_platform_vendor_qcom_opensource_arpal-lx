pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = streamsensorpcmdata_headers.pc
EXTRA_DIST = $(pkgconfig_DATA)


AM_CPPFLAGS := -I $(srcdir)/inc
AM_CPPFLAGS += -I $(srcdir)/../inc
AM_CPPFLAGS += -I $(srcdir)/../../../inc
AM_CPPFLAGS += -I $(srcdir)/../../../
AM_CPPFLAGS += -I $(srcdir)/../../../utils/inc
AM_CPPFLAGS += -I $(srcdir)/../../../device/inc
AM_CPPFLAGS += -I $(srcdir)/../../../session/inc
AM_CPPFLAGS += -I $(srcdir)/../../../stream/inc
AM_CPPFLAGS += -I $(srcdir)/../../../session/SessionAR/inc
AM_CPPFLAGS += -I $(srcdir)/../../../plugins/PluginManager/inc
AM_CPPFLAGS += -I $(srcdir)/../../../resource_manager/inc
AM_CPPFLAGS += -I $(srcdir)/../../../context_manager/inc
AM_CPPFLAGS += -I $(srcdir)/../../../api/vui-interface
AM_CPPFLAGS += @MMHEADERS_CFLAGS@
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@


streamsensorpcmdata_headers_sources = ./src/StreamSensorPCMData.cpp

library_includedir = $(includedir)/streamsensorpcmdata_headers

lib_LTLIBRARIES     = libstreamsensorpcmdata_headers.la
libstreamsensorpcmdata_headers_la_SOURCES   = $(streamsensorpcmdata_headers_sources)
libstreamsensorpcmdata_headers_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -L${abs_top_builddir}/stream/StreamCommon/.libs -lstream_common
libstreamsensorpcmdata_headers_la_CPPFLAGS := $(AM_CPPFLAGS)
libstreamsensorpcmdata_headers_la_CPPFLAGS += -std=c++14
libstreamsensorpcmdata_headers_la_LDFLAGS   = -shared -avoid-version
libstreamsensorpcmdata_headers_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libstreamsensorpcmdata_headers_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libstreamsensorpcmdata_headers_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libstreamsensorpcmdata_headers_la_LIBADD += -lglib-2.0
libstreamsensorpcmdata_headers_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libstreamsensorpcmdata_headers_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
