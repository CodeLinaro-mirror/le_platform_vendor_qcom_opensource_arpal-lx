# Makefile.am for the audioadsprpcd program

if PAL_SSR_ENABLE
AM_CFLAGS = -DPAL_SSR_ENABLE
endif

bin_PROGRAMS = audioadsprpcd

adsprpc_sources = adsprpcd.c

audioadsprpcd_SOURCES = $(adsprpc_sources)

audioadsprpcd_LDADD = -ldl

audioadsprpcd_CFLAGS = -fPIC $(AM_CFLAGS)
