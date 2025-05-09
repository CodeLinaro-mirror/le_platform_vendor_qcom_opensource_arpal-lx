pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dev_display.pc
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
AM_CPPFLAGS += @MMHEADERS_CFLAGS@
AM_CPPFLAGS += @SPFHEADERS_CFLAGS@


dev_display_sources = ./src/DisplayPort.cpp

library_includedir = $(includedir)/dev_display

lib_LTLIBRARIES     = libdev_display.la
libdev_display_la_SOURCES   = $(dev_display_sources)
libdev_display_la_LIBADD    = @GLIB_LIBS@ -llog -lcutils -lexpat -ltinyalsa -ltinycompress
libdev_display_la_LIBADD    += -L${abs_top_builddir}/session/SessionAR/.libs -lsession_ar
libdev_display_la_CPPFLAGS := $(AM_CPPFLAGS)
libdev_display_la_CPPFLAGS += -std=c++14
libdev_display_la_LDFLAGS   = -shared -avoid-version
libdev_display_la_CPPFLAGS += -DACD_SM_FILEPATH=\"/etc/models/acd/\"
libdev_display_la_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED
libdev_display_la_list      = ../../configs/$(MACHINE_ENABLED)/kvh2xml.xml \
                                 ../../configs/$(MACHINE_ENABLED)/usecaseKvManager.xml

if BUILDSYSTEM_OPENWRT
libdev_display_la_LIBADD += -lglib-2.0
endif

if USE_GLIB
libdev_display_la_LIBADD += -lglib-2.0
libdev_display_la_CPPFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if NO_STRLCPY_STRLCAT
libdev_display_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
