
AC_DEFUN([NPSGATE_PLUGIN_SUMMARY], [
		echo "Enabled Plugins:   $ENABLED_PLUGINS";
		echo "Disabled Plugins:  $DISABLED_PLUGINS";
		echo;
])

AC_DEFUN([NPSGATE_PLUGIN_DISABLE_LIST], [
		want_$1="yes";
		if test -e "DISABLED_PLUGINS" ; then
			grep $1 DISABLED_PLUGINS > /dev/null
			if test $? -eq 0; then
				want_$1="no";
			fi;
		fi;
])

