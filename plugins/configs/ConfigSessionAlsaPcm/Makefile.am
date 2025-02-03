pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = session_pcm_config.pc
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
AM_CPPFLAGS += -I ${abs_top_builddir}/plugins/configs/ConfigSessionUtils/inc

session_pcm_config_sources = ./src/ConfigSessionAlsaPcm.cpp

library_includedir = $(includedir)/session_pcm_config
lib_LTLIBRARIES     = libsession_pcm_config.la
libsession_pcm_config_la_SOURCES   = $(session_pcm_config_sources)
libsession_pcm_config_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -laudioroute -ltinyalsa
libsession_pcm_config_la_CPPFLAGS := $(AM_CPPFLAGS)
libsession_pcm_config_la_CPPFLAGS += -std=c++14
libsession_pcm_config_la_LDFLAGS   = -shared -avoid-version
libsession_pcm_config_la_CPPFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libsession_pcm_config_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsession_pcm_config_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

libsession_pcm_config_la_LIBADD   += -L${abs_top_builddir}/.libs -lpal
libsession_pcm_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAlsaPcm/.libs -lsession_pcm
libsession_pcm_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAR/.libs -lsession_ar
libsession_pcm_config_la_LIBADD   += ${abs_top_builddir}/plugins/configs/ConfigSessionUtils/.libs/libsession_utils_config.a

if BUILDSYSTEM_OPENWRT
libsession_pcm_config_la_LIBADD += -lglib-2.0
endif
