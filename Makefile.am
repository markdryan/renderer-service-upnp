INCLUDES = -DG_LOG_DOMAIN=\"RSU\"

if DEBUG
DEBUG_CFLAGS = -DDEBUG
endif

AM_CFLAGS = \
		$(DEBUG_CFLAGS) \
		$(LOG_CFLAGS)

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

rendererservice_sources = \
		src/async.c \
		src/device.c \
		src/error.c \
		src/host-service.c \
		src/log.c \
		src/renderer-service-upnp.c \
		src/task.c \
		src/upnp.c

rendererservice_headers = \
		src/async.h \
		src/device.h \
		src/error.h \
		src/host-service.h \
		src/log.h \
		src/prop-defs.h \
		src/task.h \
		src/upnp.h

bin_PROGRAMS = renderer-service-upnp
renderer_service_upnp_SOURCES = $(rendererservice_headers) $(rendererservice_sources)
renderer_service_upnp_CPPFLAGS = $(GLIB_CFLAGS)  $(GIO_CFLAGS) $(GUPNP_CFLAGS) $(GUPNPAV_CFLAGS) $(SOUP_CFLAGS)
renderer_service_upnp_LDADD = $(GLIB_LIBS) $(GIO_LIBS) $(GUPNP_LIBS) $(GUPNPAV_LIBS) $(SOUP_LIBS)

dbussessiondir = @DBUS_SESSION_DIR@
dbussession_DATA = src/com.intel.renderer-service-upnp.service

EXTRA_DIST = test/cap.py

MAINTAINERCLEANFILES =	Makefile.in \
			aclocal.m4 \
			configure \
			config.h.in \
			config.h.in~ \
			build-aux/depcomp \
			build-aux/compile \
			build-aux/missing \
			build-aux/install-sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = renderer-service-upnp.pc
DISTCLEANFILES = $(pkgconfig_DATA)
