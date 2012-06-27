dnl
dnl media-service-upnp
dnl
dnl Copyright (C) 2012 Intel Corporation. All rights reserved.
dnl
dnl This program is free software; you can redistribute it and/or modify it
dnl under the terms and conditions of the GNU Lesser General Public License,
dnl version 2.1, as published by the Free Software Foundation.
dnl
dnl This program is distributed in the hope it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
dnl FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
dnl for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public License
dnl along with this program; if not, write to the Free Software Foundation, Inc.,
dnl 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
dnl
dnl Ludovic Ferrandis <ludovic.ferrandis@intel.com>
dnl

AC_DEFUN_ONCE([RSU_COMPILER_FLAGS], [
	if (test x"${CFLAGS}" = x""); then
		CFLAGS="-Wall -O2 -D_FORTIFY_SOURCE=2"
	fi
	if (test x"$USE_MAINTAINER_MODE" = x"yes"); then
		CFLAGS+=" -Wextra"
		CFLAGS+=" -Wno-unused-parameter"
		CFLAGS+=" -Wno-missing-field-initializers"
		CFLAGS+=" -Wdeclaration-after-statement"
		CFLAGS+=" -Wmissing-declarations"
		CFLAGS+=" -Wredundant-decls"
		CFLAGS+=" -Wcast-align"
		CFLAGS+=" -DG_DISABLE_DEPRECATED"
	fi
])
