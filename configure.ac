AC_INIT([NpsGate], [0.1])
AM_INIT_AUTOMAKE([foreign -Wall no-define subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_MACRO_DIR([m4])
AM_PROG_AR
AM_SILENT_RULES([yes])

AC_PROG_CXX
LT_INIT([dlopen shared disable-static])

srcdir=`pwd`
bindir="$srcdir/run"
libdir="$srcdir/run/plugins"

LIBS="-rdynamic -ldl -lpcap -lresolv -lrt $LIBS"

AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_FUNC_MALLOC
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T

AC_ARG_ENABLE([debug],
	AS_HELP_STRING(
		[--enable-debug=@<:@ARG@:>@],
		[enable debugging build @<:@default=yes@:>@]
	),
	[
	if test "$enableval" = "yes"; then
		DEBUG="yes"
	else
		DEBUG="no"
	fi
	],
	[DEBUG="yes"]
)

if test "x$DEBUG" = "xyes"; then
	AC_DEFINE([ENABLE_DEBUG])
	CXXFLAGS="-g2 -Wall -O0"
else
	CXXFLAGS="-Wall -O2 -s"
	LDFLAGS="-Wl,-S"
fi

AX_PTHREAD(
	LIBS="$PTHREAD_LIBS $LIBS"
	CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
	CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS"
	)

# Check to see if the user wants to build a statically linked binary for Vyatta
ENABLE_VYATTA_BUILD

BOOST_REQUIRE
BOOST_SYSTEM()
BOOST_THREADS()

AC_LANG(C++)

# Check to make sure libconfig++ is installed and accessable
AX_LIBCONFIGXX

# Check to make sure libcrafter is installed and accessable
AX_CRAFTER

# NFQueue plugin requires libnetfilter-queue
AX_NETFILTER_QUEUE

# Look for plugins that need to be built
NPSGATE_PLUGIN_PROBE

# Create the list of makefiles for all plugins
NPSGATE_MAKEFILES
AC_OUTPUT

echo;
echo "Configuration of NpsGate complete.";
echo;
echo;
echo "Installation Dir:   $bindir";
echo "Plugin Dir:         $libdir";
echo;
echo "libconfig++:        $LIBCONFIGXX_MODE";
echo "libcrafter:         $CRAFTER_MODE";
echo "boost_system:       $BOOST_THREAD_LIBS";
echo;
NPSGATE_PLUGIN_SUMMARY
echo;
echo "Type 'make && make install' to build and install";
echo;



