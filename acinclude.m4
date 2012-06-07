AC_DEFUN([COMPILER_FLAGS], [
	if (test "${CFLAGS}" = ""); then
		CFLAGS="-Wall -O2 -D_FORTIFY_SOURCE=2"
	fi
	if (test "$USE_MAINTAINER_MODE" = "yes"); then
		CFLAGS="$CFLAGS -Wextra"
		CFLAGS="$CFLAGS -Wno-unused-parameter"
		CFLAGS="$CFLAGS -Wno-missing-field-initializers"
		CFLAGS="$CFLAGS -Wdeclaration-after-statement"
		CFLAGS="$CFLAGS -Wmissing-declarations"
		CFLAGS="$CFLAGS -Wredundant-decls"
		CFLAGS="$CFLAGS -Wcast-align"
		CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED"
	fi
])
