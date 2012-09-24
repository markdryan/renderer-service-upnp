AC_PREREQ([2.66])

AC_INIT([renderer-service-upnp],
	[0.0.4],
	[https://github.com/01org/renderer-service-upnp/issues/new],
	,
	[https://01.org/dleyna/])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/renderer-service-upnp.c])

AC_PREFIX_DEFAULT(/usr/local)

AM_INIT_AUTOMAKE([subdir-objects])

AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

RSU_COMPILER_FLAGS

# Checks for languages.
AC_LANG_C

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MKDIR_P

# Checks for libraries.
PKG_PROG_PKG_CONFIG(0.16)
PKG_CHECK_MODULES([DBUS], [dbus-1])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.26.1])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.26.1])
PKG_CHECK_MODULES([GUPNP], [gupnp-1.0 >= 0.17.2])
PKG_CHECK_MODULES([GUPNPAV], [gupnp-av-1.0])
PKG_CHECK_MODULES([SOUP], [libsoup-2.4])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h syslog.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT8_T
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strchr strrchr strstr])

# Define Log Level values
LOG_LEVEL_0=0x00
LOG_LEVEL_1=0x01
LOG_LEVEL_2=0x02
LOG_LEVEL_3=0x04
LOG_LEVEL_4=0x08
LOG_LEVEL_5=0x10
LOG_LEVEL_6=0x20
LOG_LEVEL_7=0x13
LOG_LEVEL_8=0x3F

AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_DISABLED], [${LOG_LEVEL_0}], [Log level flag for disabled messages])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_ERROR],    [${LOG_LEVEL_1}], [Log level flag for errors])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_CRITICAL], [${LOG_LEVEL_2}], [Log level flag for critical messages])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_WARNING],  [${LOG_LEVEL_3}], [Log level flag for warnings])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_MESSAGE],  [${LOG_LEVEL_4}], [Log level flag for messages])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_INFO],     [${LOG_LEVEL_5}], [Log level flag for informational messages])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_DEBUG],    [${LOG_LEVEL_6}], [Log level flag for debug messages])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_DEFAULT],  [${LOG_LEVEL_7}], [Log level flag to display default level messages])
AC_DEFINE_UNQUOTED([RSU_LOG_LEVEL_ALL],      [${LOG_LEVEL_8}], [Log level flag for all messages])


AC_ARG_ENABLE(debug,
		AS_HELP_STRING(
			[--enable-debug],
			[enable compiling with debugging information]),
		[],
		[enable_debug=no])

AS_CASE("${enable_debug}",
	[yes],	[	CFLAGS="$CFLAGS -g";
			AC_DEFINE_UNQUOTED([RSU_DEBUG_ENABLED],[1], [Compiling with debugging information enabled])
		],
	[no],	[],
	[AC_MSG_ERROR([bad value ${enable_debug} for --enable-debug])])


AC_ARG_ENABLE(werror,
		AS_HELP_STRING(
			[--enable-werror],
			[warnings are treated as errors]),
		[],
		[enable_werror=no])

AS_CASE("${enable_werror}",
	[yes], [CFLAGS="$CFLAGS -Werror"],
	[no], [],
	[AC_MSG_ERROR([bad value ${enable_werror} for --enable-werror])])


AC_ARG_ENABLE(optimization,
		AS_HELP_STRING(
			[--disable-optimization],
			[disable code optimization through compiler]),
		[],
		[enable_optimization=yes])

AS_CASE("${enable_optimization}",
	[yes], [disable_optimization=no],
	[no], [CFLAGS="$CFLAGS -O0"; disable_optimization=yes],
	[AC_MSG_ERROR([bad value ${enable_optimization} for --enable-werror])])


AC_ARG_ENABLE(never-quit,
		AS_HELP_STRING(
			[--enable-never-quit],
			[Service doesn't quit when last client disconnects]),
		[],
		[enable_never_quit=no])

AS_CASE("${enable_never_quit}",
	[yes], [never_quit=true; NQ=TRUE],
	[no], [never_quit=false; NQ=FALSE],
	[AC_MSG_ERROR([bad value ${enable_never_quit} for --enable-never-quit])])

AC_DEFINE_UNQUOTED([RSU_NEVER_QUIT], [${NQ}], [Default service behavior when last client disconnects])


AC_ARG_WITH(log-level,
		AS_HELP_STRING(
			[--with-log-level],
			[enable logging information (0,1..6,7,8)\
			 0=disabled \
			 7=default (=1,2,5) \
			 8=all (=1,2,3,4,5,6) \
			 1,..,6=a comma separated list of log level\
			]),
		[],
		[with_log_level=7])

RSU_LOG_LEVEL_CHECK([${with_log_level}])


AC_ARG_WITH(log-type,
		AS_HELP_STRING(
			[--with-log-type],
			[Select log output technology \
			 0=syslog  1=GLib \
			]),
		[],
		[with_log_type=0])

RSU_LOG_TYPE_CHECK([${with_log_type}])


DBUS_SESSION_DIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
AC_SUBST(DBUS_SESSION_DIR)

AC_DEFINE([RSU_SERVICE], "com.intel.RendererServiceUPnP", [Prefixed used for dbus names])
AC_DEFINE([RSU_OBJECT], "/com/intel/RendererServiceUPnP", [Name of object exposed by renderer-service-upnp])
AC_DEFINE([RSU_SERVER_PATH], "/com/intel/RendererServiceUPnP/server", [Path of server objects])

RSU_SERVER_NAME=com.intel.renderer-service-upnp
AC_SUBST(RSU_SERVER_NAME)
AC_DEFINE([RSU_SERVER_NAME], "com.intel.renderer-service-upnp",
				 [d-Bus Name of renderer-service-upnp])

RSU_INTERFACE_MANAGER=com.intel.RendererServiceUPnP.Manager
AC_SUBST(RSU_INTERFACE_MANAGER)
AC_DEFINE([RSU_INTERFACE_MANAGER], "com.intel.RendererServiceUPnP.Manager",
			       [d-Bus Name of renderer-service-upnp main interface])

RSU_INTERFACE_PUSH_HOST=com.intel.RendererServiceUPnP.PushHost
AC_SUBST(RSU_INTERFACE_PUSH_HOST)
AC_DEFINE([RSU_INTERFACE_PUSH_HOST], "com.intel.RendererServiceUPnP.PushHost",
			       [d-Bus Name of renderer-service-upnp push host interface])

AC_SUBST([never_quit])
AC_SUBST([with_log_level])
AC_SUBST([with_log_type])

AC_CONFIG_FILES([Makefile \
		 src/com.intel.renderer-service-upnp.service \
		 renderer-service-upnp.pc \
		 renderer-service-upnp.conf])

AC_OUTPUT

AS_ECHO(["-------------------------------------------------

${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix    : '${prefix}'.
Compiler  : '${CC}'
CFLAGS    : '${CFLAGS}'

-Package features:
	- enable-werror       : ${enable_werror}
	- enable-debug        : ${enable_debug}
	- disable-optimization: ${disable_optimization}
	- enable-never-quit   : ${enable_never_quit}
	- with-log-level      : ${with_log_level}
	- with-log-type       : ${with_log_type}

--------------------------------------------------"])
