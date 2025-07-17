pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = stream_asr.pc
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

stream_asr_sources = ./src/StreamASR.cpp \
                     ./src/ASREngine.cpp

library_includedir = $(includedir)/stream_asr

lib_LTLIBRARIES     = libstream_asr.la
libstream_asr_la_SOURCES   = $(stream_asr_sources)
libstream_asr_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -lar-gsl
libstream_asr_la_LIBADD    += -L${abs_top_builddir}/session/SessionAR/.libs -lsession_ar
libstream_asr_la_LIBADD    += -L${abs_top_builddir}/.libs -lpal
libstream_asr_la_CPPFLAGS := $(AM_CPPFLAGS)
libstream_asr_la_CPPFLAGS += -std=c++14
libstream_asr_la_LDFLAGS   = -shared -avoid-version
libstream_asr_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libstream_asr_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libstream_asr_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libstream_asr_la_LIBADD += -lglib-2.0
libstream_asr_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libstream_asr_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
