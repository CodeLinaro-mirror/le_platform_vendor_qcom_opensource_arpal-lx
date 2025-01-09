pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = session_voice_config.pc
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
AM_CPPFLAGS += -I ${abs_top_builddir}/session/SessionAlsaVoice/inc
AM_CPPFLAGS += -I ${abs_top_builddir}/plugins/configs/ConfigSessionUtils/inc


session_voice_config_sources = ./src/ConfigSessionAlsaVoice.cpp

library_includedir = $(includedir)/session_voice_config
lib_LTLIBRARIES     = libsession_voice_config.la
libsession_voice_config_la_SOURCES   = $(session_voice_config_sources)
libsession_voice_config_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa
libsession_voice_config_la_CPPFLAGS := $(AM_CPPFLAGS)
libsession_voice_config_la_CPPFLAGS += -std=c++14
libsession_voice_config_la_LDFLAGS   = -shared -avoid-version
libsession_voice_config_la_CPPFLAGS += @GLIB_CFLAGS@ -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat -include glib.h
libsession_voice_config_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsession_voice_config_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

libsession_voice_config_la_LIBADD   += -L${abs_top_builddir}/.libs -lpal
libsession_voice_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAR/.libs -lsession_ar
libsession_voice_config_la_LIBADD   += -L${abs_top_builddir}/session/SessionAlsaVoice/.libs -lsession_voice
libsession_voice_config_la_LIBADD   += ${abs_top_builddir}/plugins/configs/ConfigSessionUtils/.libs/libsession_utils_config.a

if BUILDSYSTEM_OPENWRT
libsession_voice_config_la_LIBADD += -lglib-2.0
endif
