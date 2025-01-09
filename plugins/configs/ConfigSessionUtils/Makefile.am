pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = session_utils_config.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CPPFLAGS := -I ./inc
AM_CPPFLAGS += -I ${abs_top_builddir}
AM_CPPFLAGS += -I ${abs_top_builddir}/api
AM_CPPFLAGS += -I ${abs_top_builddir}/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/utils/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/device/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/stream/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/session/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/context_manager/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/resource_manager/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/plugins/PluginManager/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/session/SessionAlsaPcm/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/session/SessionAR/inc

session_utils_sources = ./src/ConfigSessionUtils.cpp

library_includedir = $(includedir)/session_utils_config
lib_LTLIBRARIES     = libsession_utils_config.la
libsession_utils_config_la_SOURCES   = $(session_utils_sources)
libsession_utils_config_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa
libsession_utils_config_la_CPPFLAGS := $(AM_CPPFLAGS)
libsession_utils_config_la_CPPFLAGS += -std=c++14
libsession_utils_config_la_LDFLAGS   = --disable-shared -avoid-version
libsession_utils_config_la_CPPFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libsession_utils_config_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsession_utils_config_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

libsession_utils_config_la_LIBADD   += -L${abs_top_builddir}/.libs -lpal
libsession_utils_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAlsaPcm/.libs -lsession_pcm

if BUILDSYSTEM_OPENWRT
libsession_utils_config_la_LIBADD += -lglib-2.0
endif
