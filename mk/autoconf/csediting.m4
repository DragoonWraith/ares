# csediting.m4                                                       -*- Autoconf -*-
#==============================================================================
# CSEDITING detection macros
# Copyright (C)2005 by Eric Sunshine <sunshine@sunshineco.com>
#
#    This library is free software; you can redistribute it and/or modify it
#    under the terms of the GNU Library General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or (at your
#    option) any later version.
#
#    This library is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
#    License for more details.
#
#    You should have received a copy of the GNU Library General Public License
#    along with this library; if not, write to the Free Software Foundation,
#    Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#==============================================================================
# If you modify this file then please modify also the file
# 'CS/scripts/jamtemplate/csediting.m4'
#==============================================================================
AC_PREREQ([2.56])

m4_define([cse_min_version_default], [2.1])

#------------------------------------------------------------------------------
# CS_PATH_CSE_CHECK([MINIMUM-VERSION], [ACTION-IF-FOUND],
#                   [ACTION-IF-NOT-FOUND], [REQUIRED-LIBS], [OPTIONAL-LIBS])
#	Checks for Crystal Editing (CSE) paths and libraries by consulting
#	cse-config. It first looks for cse-config in the paths mentioned by
#	$CSE, then in the paths mentioned by $PATH, and then in
#	/usr/local/cse/bin.  Emits an error if it can not locate cse-config, if
#	the CSE test program fails, or if the available version number is
#	unsuitable.  Exports the variables CSE_CONFIG_TOOL, CSE_AVAILABLE,
#	CSE_VERSION, CSE_CFLAGS, CSE_LIBS, CSE_INCLUDE_DIR, and
#	CSE_AVAILABLE_LIBS.  If the check succeeds, then CSE_AVAILABLE will be
#	'yes', and the other variables set to appropriate values. If it fails,
#	then CSE_AVAILABLE will be 'no', and the other variables empty.  If
#	REQUIRED-LIBS is specified, then it is a list of CSE libraries which
#	must be present, and for which appropriate compiler and linker flags
#	will be reflected in CSE_CFLAGS and CSE_LFLAGS. If OPTIONAL-LIBS is
#	specified, then it is a list of CSE libraries for which appropriate
#	compiler and linker flags should be returned if the libraries are
#	available.  It is not an error for an optional library to be
#	absent. The client can check CSE_AVAILABLE_LIBS for a list of all
#	libraries available for this particular installation of CSE.  The
#	returned list is independent of REQUIRED-LIBS and OPTIONAL-LIBS.  Use
#	the results of the check like this: CFLAGS="$CFLAGS $CSE_CFLAGS" and
#	LDFLAGS="$LDFLAGS $CSE_LIBS"
#------------------------------------------------------------------------------
AC_DEFUN([CS_PATH_CSE_CHECK],
[AC_REQUIRE([CS_PATH_CRYSTAL_CHECK])
AC_ARG_WITH([cse-prefix], 
    [AC_HELP_STRING([--with-cse-prefix=CSE_PREFIX],
	[specify location of CSEDITING installation; this is the \$prefix value used
        when installing the SDK])],
    [CSE="$withval"
    export CSE])
AC_ARG_VAR([CSE], [Prefix where CSEDITING is installed])
AC_ARG_ENABLE([csetest], 
    [AC_HELP_STRING([--enable-csetest], 
	[verify that the CSEDITING SDK is actually usable (default YES)])], [],
	[enable_csetest=yes])

# Try to find an installed cse-config.
cse_path=''
AS_IF([test -n "$CSE"],
    [my_IFS=$IFS; IFS=$PATH_SEPARATOR
    for cse_dir in $CSE; do
	AS_IF([test -n "$cse_path"], [cse_path="$cse_path$PATH_SEPARATOR"])
	cse_path="$cse_path$cse_dir$PATH_SEPARATOR$cse_dir/bin"
    done
    IFS=$my_IFS])

AS_IF([test -n "$cse_path"], [cse_path="$cse_path$PATH_SEPARATOR"])
cse_path="$cse_path$PATH$PATH_SEPARATOR/usr/local/cse/bin"

# Find a suitable CSEDITING version.
# For a given desired version X.Y, the compatibility rules are as follows:
#  Y is even (stable version): compatible are X.Y+1 and X.Y+2.
#  Y is odd (development version): compatible are X.Y+1 up to X.Y+3, assuming 
#                                  no deprecated features are used.
# Generally, an exact version match is preferred. If that is not the case,
# stable versions are preferred over development version, with a closer
# version number preferred.
# This yields the following search order:
#  Y is even (stable version): X.Y, X.Y+2, X.Y+1
#  Y is odd (development version): X.Y, X.Y+1, X.Y+3, X.Y+2

cse_version_desired=m4_default([$1],[cse_min_version_default])
sed_expr_base=[\\\([0-9]\\\+\\\)\.\\\([0-9]\\\+\\\).*]
cse_version_major=`echo $cse_version_desired | sed "s/$sed_expr_base/\1/"`
cse_version_minor=`echo $cse_version_desired | sed "s/$sed_expr_base/\2/"`

cse_version_sequence="$cse_version_major.$cse_version_minor"

cse_version_desired_is_unstable=`expr $cse_version_minor % 2`

AS_IF([test $cse_version_desired_is_unstable -eq 1],
  [# Development version search sequence
  y=`expr $cse_version_minor + 1`
  cse_version_sequence="$cse_version_sequence $cse_version_major.$y"
  y=`expr $cse_version_minor + 3`
  cse_version_sequence="$cse_version_sequence $cse_version_major.$y"
  y=`expr $cse_version_minor + 2`
  cse_version_sequence="$cse_version_sequence $cse_version_major.$y"],
  [# Stable version search sequence
  y=`expr $cse_version_minor + 2`
  cse_version_sequence="$cse_version_sequence $cse_version_major.$y"
  y=`expr $cse_version_minor + 1`
  cse_version_sequence="$cse_version_sequence $cse_version_major.$y"])

for test_version in $cse_version_sequence; do
  AC_PATH_TOOL([CSE_CONFIG_TOOL], [cse-config-$test_version], [], [$cse_path])
  AS_IF([test -n "$CSE_CONFIG_TOOL"],
    [break])
done
# Legacy: CSEDITING 1.0 used a bare-named cse-config
AS_IF([test -z "$CSE_CONFIG_TOOL"],
  [AC_PATH_TOOL([CSE_CONFIG_TOOL], [cse-config], [], [$cse_path])])

AS_IF([test -n "$CSE_CONFIG_TOOL"],
    [cfg="$CSE_CONFIG_TOOL"

    CS_CHECK_PROG_VERSION([CSE], [$cfg --version],
	m4_default([$1],[cse_min_version_default]), [9.9|.9],
	[cse_sdk=yes], [cse_sdk=no])

    AS_IF([test $cse_sdk = yes],
	[cse_liblist="$4"
	cse_optlibs=CS_TRIM([$5])
	AS_IF([test -n "$cse_optlibs"],
	    [cse_optlibs=`$cfg --available-libs $cse_optlibs`
	    cse_liblist="$cse_liblist $cse_optlibs"])
	CSE_VERSION=`$cfg --version $cse_liblist`
	CSE_CFLAGS=CS_RUN_PATH_NORMALIZE([$cfg --cflags $cse_liblist])
	CSE_LIBS=CS_RUN_PATH_NORMALIZE([$cfg --lflags $cse_liblist])
	CSE_INCLUDE_DIR=CS_RUN_PATH_NORMALIZE(
	    [$cfg --includedir $cse_liblist])
	CSE_AVAILABLE_LIBS=`$cfg --available-libs`
	CSE_STATICDEPS=`$cfg --static-deps`
	AS_IF([test -z "$CSE_LIBS"], [cse_sdk=no])])],
    [cse_sdk=no])

AS_IF([test "$cse_sdk" = yes && test "$enable_csetest" = yes],
    [CS_CHECK_BUILD([if CSEDITING SDK is usable], [cse_cv_cse_sdk],
	[AC_LANG_PROGRAM(
	    [#include <cssysdef.h>
	    #include <csutil/ref.h>
	    #include <ieditor/space.h>],
	    [/* TODO write a suitable test */])],
	[CS_CREATE_TUPLE([$CSE_CFLAGS],[],[$CSE_LIBS])], [C++],
	[], [cse_sdk=no], [],
	[$CRYSTAL_CFLAGS], [], [$CRYSTAL_LIBS])])

AS_IF([test "$cse_sdk" = yes],
   [CSE_AVAILABLE=yes
   $2],
   [CSE_AVAILABLE=no
   CSE_CFLAGS=''
   CSE_VERSION=''
   CSE_LIBS=''
   CSE_INCLUDE_DIR=''
   $3])
])


#------------------------------------------------------------------------------
# CS_PATH_CSE_HELPER([MINIMUM-VERSION], [ACTION-IF-FOUND],
#                        [ACTION-IF-NOT-FOUND], [REQUIRED-LIBS],
#                        [OPTIONAL-LIBS])
#	Deprecated: Backward compatibility wrapper for CS_PATH_CSE_CHECK().
#------------------------------------------------------------------------------
AC_DEFUN([CS_PATH_CSE_HELPER],
[CS_PATH_CSE_CHECK([$1],[$2],[$3],[$4],[$5])])


#------------------------------------------------------------------------------
# CS_PATH_CSE([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                 [REQUIRED-LIBS], [OPTIONAL-LIBS])
#	Convenience wrapper for CS_PATH_CSE_CHECK() which also invokes
#	AC_SUBST() for CSE_AVAILABLE, CSE_VERSION, CSE_CFLAGS,
#	CSE_LIBS, CSE_INCLUDE_DIR, and CSE_AVAILABLE_LIBS.
#------------------------------------------------------------------------------
AC_DEFUN([CS_PATH_CSE],
[CS_PATH_CSE_CHECK([$1],[$2],[$3],[$4],[$5])
AC_SUBST([CSE_AVAILABLE])
AC_SUBST([CSE_VERSION])
AC_SUBST([CSE_CFLAGS])
AC_SUBST([CSE_LIBS])
AC_SUBST([CSE_INCLUDE_DIR])
AC_SUBST([CSE_AVAILABLE_LIBS])
AC_SUBST([CSE_STATICDEPS])])


#------------------------------------------------------------------------------
# CS_PATH_CSE_EMIT([MINIMUM-VERSION], [ACTION-IF-FOUND],
#                      [ACTION-IF-NOT-FOUND], [REQUIRED-LIBS], [OPTIONAL-LIBS],
#                      [EMITTER])
#	Convenience wrapper for CS_PATH_CSE_CHECK() which also emits
#	CSE_AVAILABLE, CSE_VERSION, CSE_CFLAGS, CSE_LIBS,
#	CSE_INCLUDE_DIR, and CSE_AVAILABLE_LIBS as the build properties
#	CSE.AVAILABLE, CSE.VERSION, CSE.CFLAGS, CSE.LIBS,
#	CSE.INCLUDE_DIR, and CSE.AVAILABLE_LIBS, respectively, using
#	EMITTER.  EMITTER is a macro name, such as CS_JAMCONFIG_PROPERTY or
#	CS_MAKEFILE_PROPERTY, which performs the actual task of emitting the
#	property and value. If EMITTER is omitted, then
#	CS_EMIT_BUILD_PROPERTY()'s default emitter is used.
#------------------------------------------------------------------------------
AC_DEFUN([CS_PATH_CSE_EMIT],
[CS_PATH_CSE_CHECK([$1],[$2],[$3],[$4],[$5])
_CS_PATH_CSE_EMIT([CSE.AVAILABLE],[$CSE_AVAILABLE],[$6])
_CS_PATH_CSE_EMIT([CSE.VERSION],[$CSE_VERSION],[$6])
_CS_PATH_CSE_EMIT([CSE.CFLAGS],[$CSE_CFLAGS],[$6])
_CS_PATH_CSE_EMIT([CSE.LFLAGS],[$CSE_LIBS],[$6])
_CS_PATH_CSE_EMIT([CSE.INCLUDE_DIR],[$CSE_INCLUDE_DIR],[$6])
_CS_PATH_CSE_EMIT([CSE.AVAILABLE_LIBS],[$CSE_AVAILABLE_LIBS],[$6])
_CS_PATH_CSE_EMIT([CSE.STATICDEPS],[$CSE_STATICDEPS],[$6])
])

AC_DEFUN([_CS_PATH_CSE_EMIT],
[CS_EMIT_BUILD_PROPERTY([$1],[$2],[],[],[$3])])


#------------------------------------------------------------------------------
# CS_PATH_CSE_JAM([MINIMUM-VERSION], [ACTION-IF-FOUND],
#                     [ACTION-IF-NOT-FOUND], [REQUIRED-LIBS], [OPTIONAL-LIBS])
#	Deprecated: Jam-specific backward compatibility wrapper for
#	CS_PATH_CSE_EMIT().
#------------------------------------------------------------------------------
AC_DEFUN([CS_PATH_CSE_JAM],
[CS_PATH_CSE_EMIT([$1],[$2],[$3],[$4],[$5],[CS_JAMCONFIG_PROPERTY])])
