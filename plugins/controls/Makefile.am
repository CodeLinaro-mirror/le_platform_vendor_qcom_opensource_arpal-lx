pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = default_plugin_controls.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CPPFLAGS = -I $(top_srcdir) \
              -I $(top_srcdir)/../../utils/inc

c_sources = ${top_srcdir}/defaultPluginControls.cpp

h_sources = ${top_srcdir}/PluginControlIntf.h

include_HEADERS = $(h_sources)

lib_LTLIBRARIES = lib_default_plugin_controls.la
lib_default_plugin_controls_la_SOURCES = $(c_sources)
lib_default_plugin_controls_la_LIBADD = $(GLIB_LIBS) -lar-pal
lib_default_plugin_controls_la_CPPFLAGS := $(AM_CPPFLAGS) -D__unused=__attribute__\(\(__unused__\)\) -DLINUX_ENABLED
lib_default_plugin_controls_la_LDFLAGS = -shared -avoid-version -lcutils -llog
