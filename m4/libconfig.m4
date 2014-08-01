
AC_DEFUN([AX_LIBCONFIGXX], [
		CONFIG_CPPFLAGS=""
		CONFIG_LDFLAGS=""
		CONFIG_LIBS="-lconfig++"

		AC_CHECK_HEADERS([libconfig.h++], [], [
			AC_MSG_FAILURE([could not find libconfig.h++])
			])

		if test "x$VYATTA_BUILD" = "xyes"; then
			saved_LIBS=$LIBS
			LIBCONFIGXX_LIBS="$srcdir/static_libs/lib64/libconfig++.a $srcdir/static_libs/lib64/libconfig.a"
			LIBCONFIGXX_MODE="static (vyatta)"
			LIBS="$LIBS $LIBCONFIGXX_LIBS"
			AC_MSG_CHECKING([for libconfig])
			AC_LINK_IFELSE([
					AC_LANG_PROGRAM(
						[#include<libconfig.h++>
						using namespace libconfig;],
						[Config* cfg = new Config();]
					)],
					[
					AC_MSG_RESULT([yes])
					],
					[
					AC_MSG_RESULT([no])
					AC_MSG_FAILURE([could not find or link against static libconfig for Vyatta build.])
					]
			)
			LIBS=$saved_LIBS
		else
			PKG_CHECK_MODULES([libconfigxx], [libconfig++ >= 1.4])
			LIBCONFIGXX_MODE="shared"
			LIBCONFIGXX_LIBS="-lconfig++"
		fi

		AC_SUBST([LIBCONFIGXX_MODE])
		AC_SUBST([LIBCONFIGXX_CFLAGS])
		AC_SUBST([LIBCONFIGXX_LDFLAGS])
		AC_SUBST([LIBCONFIGXX_LIBS])
])
