# Pioneers - Implementation of the excellent Settlers of Catan board game.
#   Go buy a copy.
#
# Copyright (C) 1999 Dave Cole
# Copyright (C) 2003-2007 Bas Wijnen <shevek@fmf.nl>
# Copyright (C) 2004-2014 Roland Clobus <rclobus@rclobus.nl>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

AC_PREREQ([2.68])
AC_INIT([pioneers],[15.6],[pio-develop@lists.sourceforge.net])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([1.11])
AC_CONFIG_SRCDIR([client])
AC_CONFIG_HEADERS([config.h])
 
META_PROTOCOL_VERSION=1.3
PIONEERS_DEFAULT_GAME_PORT=5556
PIONEERS_DEFAULT_GAME_HOST=localhost
PIONEERS_DEFAULT_ADMIN_PORT=5555
PIONEERS_DEFAULT_META_PORT=5557
PIONEERS_DEFAULT_METASERVER=pioneers.debian.net

GLIB_REQUIRED_VERSION=2.26
GIO_REQUIRED_VERSION=2.28
GTK_REQUIRED_VERSION=3.22
GTK_OPTIMAL_VERSION=3.22
LIBNOTIFY_REQUIRED_VERSION=0.7.4

AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)

AM_MAINTAINER_MODE

LT_INIT
AC_PATH_PROG(ECHO, echo)

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MKDIR_P
AC_HEADER_STDC

# The Windows ports (Cygwin and MinGW) are client-only
pioneers_is_mingw_port=no;
case $host in
	*-*-cygwin*)
		pioneers_is_windows_port=yes;;
	*-*-mingw*)
		pioneers_is_windows_port=yes;
		pioneers_is_mingw_port=yes;;
	*)
		pioneers_is_windows_port=no;;
esac

if test $USE_MAINTAINER_MODE = yes; then
	GOB2_CHECK([[2.0.0]])
fi

# Try to find a suitable renderer for the svg images
AC_SUBST(whitespace_trick, [" "])
AC_PATH_PROG(svg_renderer_path, rsvg-convert)
if test x$svg_renderer_path != x; then
	AC_SUBST(svg_renderer_width, ["--width \$(whitespace_trick)"])
	AC_SUBST(svg_renderer_height, ["\$(whitespace_trick) --height \$(whitespace_trick)"])
	AC_SUBST(svg_renderer_output, ["\$(whitespace_trick) -o \$(whitespace_trick)"])
else
AC_PATH_PROG(svg_renderer_path, rsvg)
if test x$svg_renderer_path != x; then
	AC_SUBST(svg_renderer_width, ["--width \$(whitespace_trick)"])
	AC_SUBST(svg_renderer_height, ["\$(whitespace_trick) --height \$(whitespace_trick)"])
	AC_SUBST(svg_renderer_output, ["\$(whitespace_trick)"])
else
AC_PATH_PROG(svg_renderer_path, convert)
if test x$svg_renderer_path != x; then
	AC_SUBST(svg_renderer_width, ["-background \"\#000001\" -transparent \"\#000001\" -resize \$(whitespace_trick)"])
	AC_SUBST(svg_renderer_height, ["x"])
	AC_SUBST(svg_renderer_output, ["\$(whitespace_trick)"])
else 
	# Add other SVG rendering programs here
	# Don't let configure fail, in the distributed tarballs is already
	# a current .png file
	AC_SUBST(svg_renderer_path, [false])
fi
fi
fi

AC_ARG_ENABLE([warnings], AS_HELP_STRING([--enable-warnings],
	[Compile with check for compiler warnings (gcc-only).]),
[case "${enableval}" in
  full) pioneers_warnings=full;;
  yes)  pioneers_warnings=yes ;;
  "")   pioneers_warnings=yes ;;
  *)    pioneers_warnings=no  ;;
esac],
	[pioneers_warnings=$USE_MAINTAINER_MODE])

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug],
	[Enable debug information.]),
[case "${enableval}" in
  yes)  pioneers_debug=yes ;;
  "")   pioneers_debug=yes ;;
  *)    pioneers_debug=no  ;;
esac],
	[pioneers_debug=$USE_MAINTAINER_MODE])

AC_ARG_ENABLE([deprecation-checks],
	AS_HELP_STRING([--enable-deprecation-checks],
		[Enable strict deprecation checks.]),
[case "${enableval}" in
  yes)  pioneers_deprecationChecks=yes ;;
  "")   pioneers_deprecationChecks=yes ;;
  *)    pioneers_deprecationChecks=no  ;;
esac],
	[pioneers_deprecationChecks=$USE_MAINTAINER_MODE])

AC_ARG_ENABLE([help],
	AS_HELP_STRING([--enable-help],
		[Enable help in the programs that support it.]),
[case "${enableval}" in
  yes)  enable_help=yes ;;
  "")   enable_help=yes ;;
  *)    enable_help=no  ;;
esac],
	[enable_help=yes])

AC_ARG_ENABLE([hardening],
	AS_HELP_STRING([--enable-hardening],
		[Enable hardening compiler options.]),
[case "${enableval}" in
  yes)  enable_hardening=yes ;;
  "")   enable_hardening=yes ;;
  *)    enable_hardening=no  ;;
esac],
	[enable_hardening=$USE_MAINTAINER_MODE])

AC_ARG_WITH([gtk],
	AS_HELP_STRING([--with-gtk],
		[Use GTK+ for the graphical programs.]),
[case "${withval}" in
  yes)  with_gtk=yes ;;
  "")   with_gtk=yes ;;
  *)    with_gtk=no  ;;
esac],
	[with_gtk=yes])

AC_ARG_WITH([avahi],
	AS_HELP_STRING([--with-avahi],
		[Use AVAHI for discovering games.]),
[case "${withval}" in
  yes)  with_avahi=yes ;;
  "")   with_avahi=yes ;;
  *)    with_avahi=no  ;;
esac],
	[with_avahi=yes])

AC_ARG_WITH([notify],
	AS_HELP_STRING([--with-notify],
		[Use LIBNOTIFY to notify the player during the game.]),
[case "${withval}" in
  yes)  with_notify=yes ;;
  "")   with_notify=yes ;;
  *)    with_notify=no  ;;
esac],
	[with_notify=yes])

if test "x$GCC" = xyes && test $enable_hardening = yes; then
	if test "$pioneers_is_windows_port" = "no"; then
		# Flags from Debian hardening (dpkg-buildflags --get CFLAGS)
		AC_SUBST(AM_CFLAGS, ["$AM_CFLAGS -g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Wformat-security -Werror=format-security"])
		AC_SUBST(AM_CFLAGS, ["$AM_CFLAGS -D_FORTIFY_SOURCE=2"])
		AC_SUBST(AM_CFLAGS, ["$AM_CFLAGS -pie -fPIE"])

	 	# Flags from Debian hardening (dpkg-buildflags --get LDFLAGS)
		AC_SUBST(AM_LDFLAGS, ["$AM_LDFLAGS -Wl,-z,relro"])
		AC_SUBST(AM_LDFLAGS, ["$AM_LDFLAGS -Wl,-z,now"])
	else
		enable_hardening="no, disabled for MS Windows"
	fi
fi

## The warnings are in the same order as in 'man gcc'
if test "x$GCC" = xyes; then
	# Only link the directly needed libraries	
	AC_SUBST(AM_CFLAGS, ["$AM_CFLAGS -Wl,--as-needed"])

	if test "$pioneers_warnings" = yes -o "$pioneers_warnings" = full; then
		AC_SUBST(WARNINGS, "-Wall")
		AC_SUBST(WARNINGS, "$WARNINGS -W")
		AC_SUBST(WARNINGS, "$WARNINGS -Wpointer-arith")
		AC_SUBST(WARNINGS, "$WARNINGS -Wwrite-strings")
		AC_SUBST(WARNINGS, "$WARNINGS -Wsign-compare")
		AC_SUBST(WARNINGS, "$WARNINGS -Waggregate-return")
		AC_SUBST(WARNINGS, "$WARNINGS -Wstrict-prototypes")
		AC_SUBST(WARNINGS, "$WARNINGS -Wmissing-prototypes")
		AC_SUBST(WARNINGS, "$WARNINGS -Wmissing-declarations")
		AC_SUBST(WARNINGS, "$WARNINGS -Wredundant-decls")
		AC_SUBST(WARNINGS, "$WARNINGS -Wnested-externs")
		AC_SUBST(WARNINGS, "$WARNINGS -O")
		if test "$pioneers_deprecationChecks" = no; then
			AC_SUBST(WARNINGS, "$WARNINGS -Wno-deprecated-declarations")
		fi
	fi
	if test "$pioneers_warnings" = full; then
		flags="-Wfloat-equal"
		flags="$flags -Wdeclaration-after-statement"
		flags="$flags -Wundef"
		flags="$flags -Wendif-labels"
		flags="$flags -Wshadow"
		flags="$flags -Wbad-function-cast"
		flags="$flags -Wconversion"
		flags="$flags -Wold-style-definition"
		flags="$flags -Wunreachable-code"
		flags="$flags -Wcast-qual"
		flags="$flags -pedantic"

		# This for loop comes from gnome-compiler-flags.m4
		for option in $flags; do
			SAVE_CFLAGS="$CFLAGS"
			CFLAGS="$CFLAGS $option"
			AC_MSG_CHECKING([whether gcc understands $option])
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[has_option=yes],[has_option=no])
			CFLAGS="$SAVE_CFLAGS"
			AC_MSG_RESULT($has_option)
			if test $has_option = yes; then
				AC_SUBST(WARNINGS, "$WARNINGS $option")
			fi
			unset has_option
			unset SAVE_CFLAGS
		done
		unset option
		unset flags
	fi
fi
AC_SUBST(WARNINGS)

if test "$pioneers_debug" = yes; then
	AC_SUBST(DEBUGGING, "-ggdb3")
fi
AC_SUBST(DEBUGGING)

if test "$pioneers_deprecationChecks" = yes; then
	AC_SUBST(GLIB_DEPRECATION, "-DG_DISABLE_DEPRECATED")
	AC_SUBST(GLIB_DEPRECATION, "$GLIB_DEPRECATION -DG_DISABLE_SINGLE_INCLUDES")
	AC_SUBST(GTK_DEPRECATION, "-DGDK_DISABLE_DEPRECATED")
	AC_SUBST(GTK_DEPRECATION, "$GTK_DEPRECATION -DGTK_DISABLE_DEPRECATED")
	AC_SUBST(GTK_DEPRECATION, "$GTK_DEPRECATION -DGDK_DISABLE_SINGLE_INCLUDES")
	AC_SUBST(GTK_DEPRECATION, "$GTK_DEPRECATION -DGTK_DISABLE_SINGLE_INCLUDES")
	AC_SUBST(GTK_DEPRECATION, "$GTK_DEPRECATION -DGSEAL_ENABLE")
fi
AC_SUBST(GLIB_DEPRECATION)
AC_SUBST(GTK_DEPRECATION)

if test "$enable_help" = no; then
	pioneers_help="no, disabled in configure"
else
	if test "$pioneers_is_windows_port" = "no"; then
		# The help system is optional, it comes from yelp-tools
		m4_ifndef([YELP_HELP_INIT], enable_help=no)
		if test "$enable_help" = no; then
			pioneers_help="no, yelp-tools is not installed"
		else
			pioneers_help="yes"
			YELP_HELP_INIT
		fi
	else
		pioneers_help="no, disabled for MS Windows"
	fi
fi
AM_CONDITIONAL(BUILD_HELP, [test "$pioneers_help" = "yes"])

if test "$pioneers_help" = "yes"; then
	AC_DEFINE(HAVE_HELP, 1,
		[Defined when online help is present])
fi

# glib is always needed
PKG_CHECK_MODULES(GLIB2, glib-2.0 >= $GLIB_REQUIRED_VERSION)
PKG_CHECK_MODULES(GOBJECT2, gobject-2.0 >= $GLIB_REQUIRED_VERSION)
PKG_CHECK_MODULES(GIO2, gio-2.0 >= $GIO_REQUIRED_VERSION)

#
# Check for libnotify
#
if test $with_notify = no; then
	have_libnotify="no, disabled in configure"
else
	PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= $LIBNOTIFY_REQUIRED_VERSION,
		[have_libnotify=yes],
		[have_libnotify="no, libnotify not installed or too old"])
	if test "$have_libnotify" = "yes"; then
		AC_DEFINE(HAVE_NOTIFY, 1,
			[Defined if libnotify is present])
	fi
fi

# GTK+ support
#
if test $with_gtk = no; then
	have_gtk="no, disabled in configure"
else
	PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED_VERSION,
		have_gtk=yes, 
		[PKG_CHECK_EXISTS(gtk+-3.0,
			[have_gtk="no, GTK+ version too old"],
			[have_gtk="no, GTK+ not installed"])
		AC_MSG_RESULT($have_gtk)
	])
fi
if test "$have_gtk" = "yes"; then
	PKG_CHECK_MODULES(GTK_OPTIMAL_VERSION, gtk+-3.0 >= $GTK_OPTIMAL_VERSION,
		[],
		AC_DEFINE(HAVE_OLD_GTK, [1],
			[Defined if an older version of GTK+ is available]))
fi

have_graphical=$have_gtk;
AM_CONDITIONAL(HAVE_GTK, [test "$have_graphical" = "yes"])

# Avahi support
if test $with_avahi = no; then
	have_avahi="no, disabled in configure"
else
	PKG_CHECK_MODULES(AVAHI_CLIENT, avahi-client,
		[PKG_CHECK_MODULES(AVAHI_GLIB, avahi-glib,
			[have_avahi="yes"
			AC_DEFINE(HAVE_AVAHI, [1], [Define if AVAHI available])
			AC_DEFINE(AVAHI_ANNOUNCE_NAME, ["_pioneers._tcp"], [The name of the Avahi service])],
			[have_avahi="no, avahi-glib is missing"
			AC_MSG_RESULT($have_avahi)])],
		[have_avahi="no, avahi-client is missing"
		AC_MSG_RESULT($have_avahi)])

fi

AC_ARG_ENABLE([admin-gtk], AS_HELP_STRING([--enable-admin-gtk],
	[Turn on (unstable) network administration support.]),
[case "${enableval}" in
  yes) admin_gtk_support=yes  ;;
  "")  admin_gtk_support=yes  ;;
  *)   admin_gtk_support=no ;;
esac], [admin_gtk_support=no])
AM_CONDITIONAL(ADMIN_GTK_SUPPORT, [test x$admin_gtk_support = xyes])

AC_ARG_ENABLE([protocol],
	AS_HELP_STRING([--enable-protocol],
	[Specify the network protocol for Avahi (IPv4/unspecified)]),
[case "${enableval}" in
  IPv4)        avahi_network_protocol=AVAHI_PROTO_INET;;
  *)           avahi_network_protocol=AVAHI_PROTO_UNSPEC;;
esac], [avahi_network_protocol=AVAHI_PROTO_UNSPEC])
AC_DEFINE_UNQUOTED([AVAHI_NETWORK_PROTOCOL],[$avahi_network_protocol],
	[The Avahi network protocol value])

AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([syslog.h],
	[pioneers_have_syslog=yes;],
	[pioneers_have_syslog=no;])
AC_HEADER_TIME

AC_C_CONST

# Functions
AC_FUNC_FORK

# Mathematics
AC_CHECK_FUNC(rint,
	AC_DEFINE(HAVE_RINT, 1, [Define to 1 if you have the rint function.]),
	AC_CHECK_LIB(m, rint, 
		[ AC_DEFINE(HAVE_RINT) LIBS="$LIBS -lm"]))
AC_CHECK_FUNCS([sqrt])
# String functions
AC_CHECK_FUNCS([strchr strspn strstr strcspn])
AC_CHECK_FUNCS([memmove memset])

# Functions needed to support NLS
AC_CHECK_FUNCS([setlocale])

# Data types
AC_STRUCT_TM
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Defines, accessible for all source files
AC_DEFINE_UNQUOTED(META_PROTOCOL_VERSION, "$META_PROTOCOL_VERSION",
		   [Protocol version used by the metaserver])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_GAME_PORT, "$PIONEERS_DEFAULT_GAME_PORT",
		   [The default port for a new game])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_GAME_HOST, "$PIONEERS_DEFAULT_GAME_HOST",
		   [The default host for a new game])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_ADMIN_PORT, "$PIONEERS_DEFAULT_ADMIN_PORT",
		   [The default port for the admin interface])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_META_PORT, "$PIONEERS_DEFAULT_META_PORT",
		   [The port for the metaserver])
AC_DEFINE_UNQUOTED(PIONEERS_DEFAULT_METASERVER, 
		   "$PIONEERS_DEFAULT_METASERVER", [The default metaserver])

## internationalization support
IT_PROG_INTLTOOL([0.35])
AM_NLS
if test $USE_NLS = yes; then
	AC_DEFINE(ENABLE_NLS, 1, [Enable NLS])
fi

GETTEXT_PACKAGE=pioneers
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext package name])

if test $pioneers_is_mingw_port = yes; then
	# No console window for the graphical applications
	AC_SUBST(GTK_LIBS, "$GTK_LIBS -mwindows")
	# Don't use bin, lib and share subdirectories
	datadir='${prefix}'
	bindir='${prefix}'
	libdir='${prefix}'
	pioneers_datadir=.
	pioneers_iconthemedir=$datadir/share/icons/hicolor
	pioneers_themedir=$datadir/themes
	pioneers_themedir_embed=themes
	pioneers_localedir=locale
	DATADIRNAME=.
else
	pioneers_datadir=$datadir
	pioneers_iconthemedir=$datadir/icons/hicolor
	pioneers_themedir=$datadir/games/pioneers/themes
	pioneers_themedir_embed=$pioneers_themedir
	pioneers_localedir=$datadir/locale
fi
AC_SUBST(pioneers_datadir)
AC_SUBST(pioneers_iconthemedir)
AC_SUBST(pioneers_themedir)
AC_SUBST(pioneers_themedir_embed)
AC_SUBST(pioneers_localedir)

# All checks are completed.
# Determine which executables cannot be built
pioneers_build_client_ai=yes;
pioneers_build_client_gtk=yes;
pioneers_build_editor=yes;
pioneers_build_server_console=yes;
pioneers_build_server_gtk=yes;
pioneers_build_metaserver=yes;

if test "$pioneers_have_syslog" = "no"; then
	pioneers_build_metaserver=no;
fi
if test "$have_graphical" != "yes"; then
	pioneers_build_client_gtk=$have_graphical;
	pioneers_build_editor=$have_graphical;
	pioneers_build_server_gtk=$have_graphical;
fi

# The metaserver functionality is not ported to MS Windows
if test "$pioneers_is_windows_port" = "yes"; then
	pioneers_build_metaserver="no, not implemented for MS Windows";
fi

AM_CONDITIONAL(USE_WINDOWS_ICON, [test $pioneers_is_windows_port = yes])

AM_CONDITIONAL(BUILD_CLIENT, [test "$pioneers_build_client_gtk" = "yes" -o "$pioneers_build_client_ai" = yes])
AM_CONDITIONAL(BUILD_EDITOR, [test "$pioneers_build_editor" = "yes"])
AM_CONDITIONAL(BUILD_SERVER, [test "$pioneers_build_server_gtk" = "yes" -o "$pioneers_build_server_console" = "yes"])
AM_CONDITIONAL(BUILD_METASERVER, [test "$pioneers_build_metaserver" = "yes"])
AM_CONDITIONAL(IS_MINGW_PORT, [test "$pioneers_is_mingw_port" = "yes"])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([pioneers.spec])
AC_CONFIG_FILES([Doxyfile])
AC_CONFIG_FILES([MinGW/pioneers.nsi])
AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([client/help/Makefile])

AC_OUTPUT

AC_MSG_NOTICE([
$PACKAGE v$VERSION configuration:

        Source code location:     ${srcdir}
        Install location:         ${prefix}
        Compiler:                 ${CC}
        Build graphical client    $pioneers_build_client_gtk
        Build computer player     $pioneers_build_client_ai
        Build game editor         $pioneers_build_editor
        Build graphical server    $pioneers_build_server_gtk
        Build console server      $pioneers_build_server_console
        Build metaserver          $pioneers_build_metaserver
        Build help                $pioneers_help
        AVAHI support             $have_avahi
        LIBNOTIFY support         $have_libnotify
    Developers only:
        Use compiler warnings     $pioneers_warnings
        Add debug information     $pioneers_debug
        Enable deprecation checks $pioneers_deprecationChecks
        Hardening compiler flags  $enable_hardening
])
