AC_INIT([originalclock], [0.1], [munya2@munyamunya.noor.jp])
dnl C++コンパイラーの候補を列挙する
AC_PROG_CXX([g++])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_CONFIG_FILES([Makefile])
dnl pkg-config --cflags gtk+-3.0、pkg-config --libs gtk+-3.0の結果を、
dnl それぞれ XXX_CFLAGS, XXX_LIBS にセット、
dnl Makefile.amで、appname_CXXFLAGS=$(XXX_CFLAGS)のように
dnl 指定できるようにする
PKG_CHECK_MODULES(GTKMOD, [gtk+-3.0])
AC_OUTPUT

