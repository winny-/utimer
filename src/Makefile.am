AM_CPPFLAGS = $(GLIB_CFLAGS) -DLOCALEDIR=\""$(UTIMERLOCALEDIR)"\"

bin_PROGRAMS = utimer
utimer_SOURCES = utimer.c utimer.h \
                 timer.c  timer.h \
                 utils.h  utils.c \
                 log.c    logs.h \
                 ut_config.h
utimer_LDADD = $(GLIB_LIBS) $(GIO_LIBS)

