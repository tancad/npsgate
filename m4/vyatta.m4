AC_DEFUN([ENABLE_VYATTA_BUILD], [
	AC_ARG_ENABLE([vyatta-build],
		AS_HELP_STRING(
			[--enable-vyatta-build],
			[link for use with Vyatta @<:@default=no@:>]
		),
		[
		if test "x$enableval" = "xyes"; then
			VYATTA_BUILD="yes"	
		else 
			VYATTA_BUILD="no"
		fi
		],[
			VYATTA_BUILD="no"
		]
	)
	AC_SUBST([VYATTA_BUILD])
])
