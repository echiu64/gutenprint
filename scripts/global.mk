## Global rules and macros to be included in all Makefiles.


# Variables

export STP_MODULE_PATH = $(top_builddir)/src/main/.libs
export STP_DATA_PATH = $(top_srcdir)/src/main

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/intl $(GNUCFLAGS) $(LOCAL_CFLAGS)

LIBS = $(INTLLIBS) @LIBS@

# Libraries

GIMPPRINT_LIBS = $(top_builddir)/src/main/libgimpprint.la
GIMPPRINTUI_LIBS = $(top_builddir)/src/libgimpprintui/libgimpprintui.la
LIBPRINTUT = $(top_builddir)/lib/libprintut.la

# Rules

$(top_builddir)/src/main/libgimpprint.la:
	cd $(top_builddir)/src/main; \
	$(MAKE)

$(top_builddir)/src/libgimpprintui/libgimpprintui.la:
	cd $(top_builddir)/src/libgimpprintui; \
	$(MAKE)
