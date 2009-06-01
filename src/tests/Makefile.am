include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS = $(GLIB_CFLAGS) $(UT_DEBUG_FLAGS)

# ==  Tests  ==

progs_ldadd = $(GLIB_LIBS) $(GIO_LIBS)
TEST_PROGS          += maintests
maintests_SOURCES    = maintests.c \
                       $(top_srcdir)/src/utimer.h \
                       $(top_srcdir)/src/timer.c  $(top_srcdir)/src/timer.h \
                       $(top_srcdir)/src/utils.h  $(top_srcdir)/src/utils.c \
                       $(top_srcdir)/src/log.c    $(top_srcdir)/src/log.h

maintests_LDADD     = $(progs_ldadd)

# == End Tests ==

noinst_PROGRAMS = $(TEST_PROGS)
