   echo "========= Preparing uTimer ============"\
&& echo "======== Running autoreconf ==========="\
&& autoreconf --force --install --verbose \
&& echo "======== Running glib-gettextize ======"\
&& glib-gettextize --force --copy \
&& echo "======== Running intltoolize =========="\
&& echo "(there may be no output, it's fine)"\
&& intltoolize --copy --force --automake \
&& echo "=============== DONE ==================" \
&& echo -e "You can now type these commands: \n$ ./configure\n$ make\n$ make install\nAnd then uTimer should be installed!" \
&& echo "=============== EOF ==================="
