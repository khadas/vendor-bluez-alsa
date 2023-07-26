# BlueALSA - Makefile.am
# Copyright (c) 2016-2017 Arkadiusz Bokowy

bin_PROGRAMS =

if ENABLE_APLAY
bin_PROGRAMS += bluealsa-aplay
bluealsa_aplay_SOURCES = \
	../src/shared/ctl-client.c \
	../src/shared/ffb.c \
	../src/shared/log.c \
	aplay.c
bluealsa_aplay_CFLAGS = \
	-I$(top_srcdir)/src \
	@ALSA_CFLAGS@ \
	@BLUEZ_CFLAGS@ \
	@GIO2_CFLAGS@
bluealsa_aplay_LDADD = \
	@ALSA_LIBS@ \
	@BLUEZ_LIBS@ \
	@GIO2_LIBS@
endif

if ENABLE_HCITOP
bin_PROGRAMS += hcitop
hcitop_CFLAGS = \
	@BLUEZ_CFLAGS@ \
	@LIBBSD_CFLAGS@ \
	@NCURSES_CFLAGS@
hcitop_LDADD = \
	@BLUEZ_LIBS@ \
	@LIBBSD_LIBS@ \
	@NCURSES_LIBS@
endif

bin_PROGRAMS += a2dp_ctl
a2dp_ctl_SOURCES = a2dp_ctl.c \
	lib_a2dp_ctl.c
a2dp_ctl_CFLAGS = \
	@ALSA_CFLAGS@ \
	@GIO2_CFLAGS@
a2dp_ctl_LDADD = \
	@ALSA_LIBS@ \
	@GIO2_LIBS@

bin_PROGRAMS += hfp_ctl
hfp_ctl_SOURCES = hfp_ctl.c \
	sco_handler.c \
	../src/shared/log.c \
	../src/shared/ctl-socket.c
hfp_ctl_CFLAGS = -lpthread \
	-I$(top_srcdir)/src \
	@ALSA_CFLAGS@
hfp_ctl_LDADD = \
	@ALSA_LIBS@

lib_LTLIBRARIES = liba2dp_ctl.la

liba2dp_ctl_la_SOURCES = \
	lib_a2dp_ctl.c

AM_CFLAGS = \
	@ALSA_CFLAGS@ \
	@BLUEZ_CFLAGS@ \
	@GLIB2_CFLAGS@

AM_LDFLAGS = -module -avoid-version

liba2dp_ctl_la_LIBADD = \
	@ALSA_LIBS@

include_HEADERS=$(top_srcdir)/utils/a2dp_ctl.h
