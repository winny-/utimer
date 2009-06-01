include $(top_srcdir)/Makefile.decl 

SUBDIRS = tests

AM_CPPFLAGS = $(GLIB_CFLAGS) $(UT_DEBUG_FLAGS)

bin_PROGRAMS   = utimer
utimer_SOURCES = utimer.c utimer.h \
                 timer.c  timer.h \
                 utils.h  utils.c \
                 log.c    log.h

utimer_LDADD = $(GLIB_LIBS) $(GIO_LIBS)

noinst_PROGRAMS = $(TEST_PROGS)
