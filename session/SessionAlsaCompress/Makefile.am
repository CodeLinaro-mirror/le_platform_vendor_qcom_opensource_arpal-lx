pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = session_compress.pc
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

session_compress_sources = ./src/SessionAlsaCompress.cpp

library_includedir = $(includedir)/session_compress

lib_LTLIBRARIES     = libsession_compress.la
libsession_compress_la_SOURCES   = $(session_compress_sources)
libsession_compress_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa -ltinycompress
libsession_compress_la_LIBADD    += -L${abs_top_builddir}/plugins/vui_interface/.libs -lvui_intf
libsession_compress_la_LIBADD    += -L${abs_top_builddir}/session/SessionAR/.libs -lsession_ar -L${abs_top_builddir}/.libs -lpal
libsession_compress_la_CPPFLAGS := $(AM_CPPFLAGS)
libsession_compress_la_CPPFLAGS += -std=c++14
libsession_compress_la_LDFLAGS   = -shared -avoid-version
libsession_compress_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsession_compress_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libsession_compress_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libsession_compress_la_LIBADD += -lglib-2.0
libsession_compress_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libsession_compress_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
