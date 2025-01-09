pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = session_compress_config.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CPPFLAGS := -I ./inc
AM_CPPFLAGS += -I ${abs_top_builddir}/
AM_CPPFLAGS += -I ${abs_top_builddir}/api
AM_CPPFLAGS += -I ${abs_top_builddir}/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/utils/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/device/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/stream/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/session/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/context_manager/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/resource_manager/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/session/SessionAR/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/plugins/PluginManager/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/session/SessionAlsaPcm/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/session/SessionAlsaCompress/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/plugins/configs/ConfigSessionUtils/inc

session_compress_config_sources = ./src/ConfigSessionAlsaCompress.cpp

library_includedir = $(includedir)/session_compress_config
lib_LTLIBRARIES     = libsession_compress_config.la
libsession_compress_config_la_SOURCES   = $(session_compress_config_sources)
libsession_compress_config_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa -ltinycompress
libsession_compress_config_la_CPPFLAGS := $(AM_CPPFLAGS)
libsession_compress_config_la_CPPFLAGS += -std=c++14
libsession_compress_config_la_LDFLAGS   = -shared -avoid-version
libsession_compress_config_la_CPPFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libsession_compress_config_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsession_compress_config_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

libsession_compress_config_la_LIBADD   += -L${abs_top_builddir}/.libs -lpal
libsession_compress_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAR/.libs -lsession_ar
libsession_compress_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAlsaCompress/.libs -lsession_compress
libsession_compress_config_la_LIBADD   += ${abs_top_builddir}/plugins/configs/ConfigSessionUtils/.libs/libsession_utils_config.a

if BUILDSYSTEM_OPENWRT
libsession_compress_config_la_LIBADD += -lglib-2.0
endif
