pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = session_compress_config.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CPPFLAGS := -I $(srcdir)/inc
AM_CPPFLAGS += -I ${top_srcdir}/
AM_CPPFLAGS += -I ${top_srcdir}/api
AM_CPPFLAGS += -I ${top_srcdir}/inc
AM_CPPFLAGS += -I ${top_srcdir}/utils/inc
AM_CPPFLAGS += -I ${top_srcdir}/device/inc
AM_CPPFLAGS += -I ${top_srcdir}/stream/inc
AM_CPPFLAGS += -I ${top_srcdir}/session/inc
AM_CPPFLAGS += -I ${top_srcdir}/context_manager/inc
AM_CPPFLAGS += -I ${top_srcdir}/resource_manager/inc
AM_CPPFLAGS += -I ${top_srcdir}/session/SessionAR/inc
AM_CPPFLAGS += -I ${top_srcdir}/plugins/PluginManager/inc
AM_CPPFLAGS += -I ${top_srcdir}/session/SessionAlsaPcm/inc
AM_CPPFLAGS += -I ${top_srcdir}/session/SessionAlsaCompress/inc
AM_CPPFLAGS += -I ${top_srcdir}/plugins/configs/ConfigSessionUtils/inc
AM_CPPFLAGS += -I ${top_srcdir}/api/vui-interface
AM_CPPFLAGS += @MMHEADERS_CFLAGS@
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@

session_compress_config_sources = $(srcdir)/src/ConfigSessionAlsaCompress.cpp

library_includedir = $(includedir)/session_compress_config
lib_LTLIBRARIES     = libsession_compress_config.la
libsession_compress_config_la_SOURCES   = $(session_compress_config_sources)
libsession_compress_config_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa -ltinycompress
libsession_compress_config_la_CPPFLAGS := $(AM_CPPFLAGS)
libsession_compress_config_la_CPPFLAGS += -std=c++14
libsession_compress_config_la_LDFLAGS   = -shared -avoid-version
libsession_compress_config_la_CPPFLAGS += @GLIB_CFLAGS@ -include glib.h
libsession_compress_config_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsession_compress_config_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

libsession_compress_config_la_LIBADD   += -L${abs_top_builddir}/.libs -lpal
libsession_compress_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAR/.libs -lsession_ar
libsession_compress_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAlsaCompress/.libs -lsession_compress
libsession_compress_config_la_LIBADD   += ${abs_top_builddir}/plugins/configs/ConfigSessionUtils/.libs/libsession_utils_config.a

if NO_STRLCPY_STRLCAT
libsession_compress_config_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif

if BUILDSYSTEM_OPENWRT
libsession_compress_config_la_LIBADD += -lglib-2.0
endif
