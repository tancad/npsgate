AC_DEFUN([AX_CRAFTER], [
		CRAFTER_CPPFLAGS=""
		CRAFTER_LDFLAGS=""
		CRAFTER_LIBS="-lcrafter -lpcap -lresolv"

		AC_CHECK_HEADERS([crafter.h], [], [
			AC_MSG_FAILURE([could not find crafter.h])
			])


		if test "x$VYATTA_BUILD" = "xyes"; then
			saved_LIBS=$LIBS
			CRAFTER_LIBS="$srcdir/static_libs/lib64/libcrafter.a -Wl,-lpcap -Wl,-lresolv"
			CRAFTER_MODE="static (vyatta)"
			LIBS="$LIBS $CRAFTER_LIBS"
			AC_MSG_CHECKING([for static libcrafter])
			AC_LINK_IFELSE([
					AC_LANG_PROGRAM(
						[#include<crafter.h>],
						[Crafter::RawLayer hello("hello");]
					)],
					[
					AC_MSG_RESULT([yes])
					],
					[
					AC_MSG_RESULT([no])
					AC_MSG_FAILURE([could not find or link against static libcrafter for Vyatta build.])
					]
			)
			LIBS=$saved_LIBS
		else
			saved_LIBS=$LIBS
			LIBS="$LIBS -lcrafter -lpcap -lresolv"
			AC_MSG_CHECKING([for libcrafter])
			AC_LINK_IFELSE([
					AC_LANG_PROGRAM(
						[#include<crafter.h>],
						[Crafter::RawLayer hello("hello");]
					)],
					[
					CRAFTER_MODE="shared"
					CRAFTER_LIBS="-lcrafter -lpcap -lresolv"
					AC_MSG_RESULT([yes])
					],
					[
					AC_MSG_RESULT([no])
					AC_MSG_FAILURE([could not find or link against libcrafter.])
					]
			)
			LIBS=$saved_LIBS
		fi

		dnl
		dnl Export flags.
		dnl
		AC_SUBST([CRAFTER_MODE])
		AC_SUBST([CRAFTER_LIBS])
		AC_SUBST([CRAFTER_CPPFLAGS])
		AC_SUBST([CRAFTER_LDFLAGS])
])
