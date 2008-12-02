AC_INIT([utimer], [0.1], [weboide@codealpha.net])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

PKG_CHECK_MODULES([GLIB],[glib-2.0 >= 2.18.2])
PKG_CHECK_MODULES([GIO], [gio-unix-2.0])
AC_CHECK_LIB(gthread-2.0, g_thread_init)
#PKG_CHECK_MODULES([GOBJECT], [ glib-2.0 gobject-2.0 ])

AC_PROG_INTLTOOL([0.40.5])


GETTEXT_PACKAGE=utimer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AM_GLIB_GNU_GETTEXT

UTIMERLOCALEDIR='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(UTIMERLOCALEDIR)

AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in])


AC_OUTPUT
