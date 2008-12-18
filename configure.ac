AC_INIT([utimer], [0.1.1], [bugs@utimer.codealpha.net])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

PKG_CHECK_MODULES([GLIB],[glib-2.0 >= 2.18.2])
PKG_CHECK_MODULES([GIO], [gio-unix-2.0])
AC_CHECK_LIB(gthread-2.0, g_thread_init)
#PKG_CHECK_MODULES([GOBJECT], [ glib-2.0 gobject-2.0 ])



# -- i18n --

AC_PROG_INTLTOOL([0.40.5])
GETTEXT_PACKAGE=utimer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(LOCALEDIR, "${ac_default_prefix}/${DATADIRNAME}/locale", [Locale dir])
else
  AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Locale dir])
fi

AM_GLIB_GNU_GETTEXT


# -- -- --


UTIMER_CODENAME='sunrise'
AC_SUBST(UTIMER_CODENAME)

UTIMER_STABILITY='*UNSTABLE*'
AC_SUBST(UTIMER_STABILITY)

AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in README data/Makefile])


AC_OUTPUT
