AM_CPPFLAGS = $(GLIB_CFLAGS) -DLOCALEDIR=\""$(LOCALEDIR)"\"

bin_PROGRAMS = utimer
utimer_SOURCES = utimer.c utimer.h \
                 timer.c  timer.h \
                 utils.h  utils.c \
                 log.c    log.h \
                 ut_config.h

utimer_LDADD = $(GLIB_LIBS) $(GIO_LIBS)

