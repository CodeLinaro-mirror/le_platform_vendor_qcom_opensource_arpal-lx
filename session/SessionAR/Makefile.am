pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = session_ar.pc
EXTRA_DIST = $(pkgconfig_DATA)


AM_CPPFLAGS := -I $(srcdir)/inc
AM_CPPFLAGS += -I $(srcdir)/../../inc
AM_CPPFLAGS += -I $(srcdir)/../../
AM_CPPFLAGS += -I $(srcdir)/../../utils/inc
AM_CPPFLAGS += -I $(srcdir)/../../device/inc
AM_CPPFLAGS += -I $(srcdir)/../../session/inc
AM_CPPFLAGS += -I $(srcdir)/../../stream/inc
AM_CPPFLAGS += -I $(srcdir)/../../plugins/PluginManager/inc
AM_CPPFLAGS += -I $(srcdir)/../../plugins/codecs
AM_CPPFLAGS += -I $(srcdir)/../../resource_manager/inc
AM_CPPFLAGS += -I $(srcdir)/../../context_manager/inc
AM_CPPFLAGS += @MMHEADERS_CFLAGS@
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@


session_ar_sources = ./src/SessionAR.cpp \
                     ./src/SessionAlsaUtils.cpp \
                     ./src/PayloadBuilder.cpp

library_includedir = $(includedir)/session_ar

lib_LTLIBRARIES     = libsession_ar.la
libsession_ar_la_SOURCES   = $(session_ar_sources)
libsession_ar_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa -ltinycompress -lar-gsl
libsession_ar_la_LIBADD   += -L${abs_top_builddir}/.libs -lpal
libsession_ar_la_CPPFLAGS := $(AM_CPPFLAGS)
libsession_ar_la_CPPFLAGS += -std=c++14
libsession_ar_la_LDFLAGS   = -shared -avoid-version
libsession_ar_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libsession_ar_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED

if BUILDSYSTEM_OPENWRT
libsession_ar_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libsession_ar_la_LIBADD += -lglib-2.0
libsession_ar_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libsession_ar_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
