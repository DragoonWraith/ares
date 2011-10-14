#----------------------------------------------------------------------------
# Autoconf input script. Invoke the ./autogen.sh script to generate a
# configure script from this file.
#----------------------------------------------------------------------------
AC_PREREQ([2.54])

#----------------------------------------------------------------------------
# Initialize Autoconf.
#----------------------------------------------------------------------------
AC_INIT(
    [ares],
    [0.1],
    [jorrit.tyberghein@gmail.com])
CS_PACKAGEINFO(
    [The Ares Project],
    [Jorrit Tyberghein],
    [])
AC_CONFIG_SRCDIR([mk/jam/build.jam])
AC_CONFIG_AUX_DIR([mk/autoconf])
AC_CANONICAL_HOST
CS_PATH_INIT

#----------------------------------------------------------------------------
# Setup for the configuration header.
#----------------------------------------------------------------------------
AC_CONFIG_HEADERS([config.h])

#----------------------------------------------------------------------------
# Check for tools.
#----------------------------------------------------------------------------
CS_PROG_CC
AS_IF([test -z "$CC"],
    [AC_MSG_ERROR([Could not find a usable C compiler.])])
CS_PROG_CXX
AS_IF([test -z "$CXX"],
    [AC_MSG_ERROR([Could not find a usable C++ compiler.])])
CS_PROG_LINK

# The default RANLIB in Jambase is wrong on some platforms, and is also
# unsuitable during cross-compilation, so we set the value unconditionally
# (sixth argument of CS_EMIT_BUILD_PROPERTY).
AC_PROG_RANLIB
CS_EMIT_BUILD_PROPERTY([RANLIB], [$RANLIB], [], [], [], [Y])

CS_CHECK_TOOLS([WINDRES], [windres])
CS_EMIT_BUILD_PROPERTY([CMD.WINDRES], [$WINDRES])

CS_CHECK_PROGS([DOXYGEN], [doxygen])
CS_EMIT_BUILD_PROPERTY([CMD.DOXYGEN], [$DOXYGEN])

CS_CHECK_TOOLS([DOT], [dot])
CS_EMIT_BUILD_PROPERTY([CMD.DOT], [$DOT])

CS_CHECK_PROGS([TEXI2DVI], [texi2dvi])
CS_EMIT_BUILD_PROPERTY([CMD.TEXI2DVI], [$TEXI2DVI])

CS_CHECK_PROGS([TEXI2PDF], [texi2pdf])
CS_EMIT_BUILD_PROPERTY([CMD.TEXI2PDF], [$TEXI2PDF])

CS_CHECK_PROGS([DVIPS], [dvips])
CS_EMIT_BUILD_PROPERTY([CMD.DVIPS], [$DVIPS])

CS_CHECK_PROGS([DVIPDF], [dvipdf])
CS_EMIT_BUILD_PROPERTY([CMD.DVIPDF], [$DVIPDF])

CS_CHECK_PROGS([MAKEINFO], [makeinfo])
CS_EMIT_BUILD_PROPERTY([CMD.MAKEINFO], [$MAKEINFO])

CS_CHECK_PROGS([INSTALL], [install])
CS_EMIT_BUILD_PROPERTY([INSTALL], [$INSTALL])

CS_CHECK_LIBTOOL
CS_EMIT_BUILD_PROPERTY([LIBTOOL], [$LIBTOOL])
CS_EMIT_BUILD_PROPERTY([APPLE_LIBTOOL], [$APPLE_LIBTOOL])

CS_CHECK_MKDIR
CS_EMIT_BUILD_PROPERTY([CMD.MKDIR], [$MKDIR])
CS_EMIT_BUILD_PROPERTY([CMD.MKDIRS], [$MKDIRS])

CS_CHECK_PROGS([PERL], [perl5 perl])
CS_EMIT_BUILD_PROPERTY([PERL], [$PERL])

CS_CHECK_TEMPLATE_TOOLKIT2([emit])

#----------------------------------------------------------------------------
# Determine system type
#----------------------------------------------------------------------------
CS_CHECK_HOST

#----------------------------------------------------------------------------
# Check for syntax problems / header files
#----------------------------------------------------------------------------
# Nothing yet.

#----------------------------------------------------------------------------
# Check for external libraries
#----------------------------------------------------------------------------
CS_CHECK_CPPUNIT([emit])

#----------------------------------------------------------------------------
# Check for Crystal Space (CS)
#----------------------------------------------------------------------------
AC_ARG_WITH([cs], [AC_HELP_STRING([--without-cs],
  [do not look for an installed Crystal Space SDK; this option is useful only
  if you need to perform routine maintenance tasks, such as generating
  project files or converting documentation, without having the Crystal Space
  SDK installed])])
AS_IF([test -z "$with_cs"], [with_cs=yes])

AS_IF([test $with_cs != no],
[CS_PATH_CRYSTAL_EMIT([1.1], [],
  [AC_MSG_ERROR([
*** Crystal Space could not be found or was unusable. The latest version is
*** always available from http://www.crystalspace3d.org/
*** Also, be sure that you have either installed Crystal Space or set the
*** CRYSTAL_1_1 environment variable properly.
])], [crystalspace])])

#----------------------------------------------------------------------------
# Check for Crystal Entity Layer (CEL)
#----------------------------------------------------------------------------
m4_define([cel_desired], [yes])
CS_EMIT_BUILD_PROPERTY([CEL.DESIRED], [cel_desired])
m4_if(cel_desired, [yes],
[AC_ARG_WITH([cel], [AC_HELP_STRING([--without-cel],
  [do not look for an installed Crystal Entity Layer SDK; this option is useful
  only if you need to perform routine maintenance tasks, such as generating
  project files or converting documentation, without having the CEL SDK
  installed])])
AS_IF([test -z "$with_cel"], [with_cel=yes])

AS_IF([test $with_cel != no],
[CS_PATH_CEL_EMIT([1.1], [],
  [AC_MSG_ERROR([
*** CEL could not be found or was unusable. The latest version is always
*** available from http://cel.crystalspace3d.org/
])])
AC_DEFINE([USE_CEL], [], [Define to enable CEL.])
AC_DEFINE_UNQUOTED([CEL_PLUGIN_DIR], ["$CEL_PLUGIN_DIR"],
  [Define with path to CEL plugins.])
])])

#----------------------------------------------------------------------------
# Win32libs additions.
#----------------------------------------------------------------------------
CS_CHECK_CSWIN32LIBS

#------------------------------------------------------------------------------
# Use the CPPFLAGS, CXXFLAGS, CFLAGS, and LDFLAGS passed to configure, as well
# as any additional flags provided by cs-win32libs.
#------------------------------------------------------------------------------
CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS], [$CPPFLAGS $CFLAGS], [+])
CS_EMIT_BUILD_PROPERTY([COMPILER.C++FLAGS], [$CPPFLAGS $CXXFLAGS], [+])
CS_EMIT_BUILD_PROPERTY([COMPILER.LFLAGS], [$LDFLAGS], [+])

#----------------------------------------------------------------------------
# Check for CEGUI.
#----------------------------------------------------------------------------
# The check fails on some distros with missing references to libdl
# (eg on OpenSUSE), so pass -ldl along as well
CS_CHECK_LIB_WITH([CEGUI],
    [AC_LANG_PROGRAM([[#include <CEGUI.h>]],
    [CEGUI::WindowManager::getSingleton();])], [], [C++],
    [], [], [], [], [$cs_cv_libdl_libs], [CEGUIBase])

AS_IF([test $cs_cv_libCEGUI = yes],
    [CS_CHECK_BUILD([if CEGUI is sufficiently recent], [cs_cv_CEGUI_recent],
	[AC_LANG_PROGRAM(
	    [[#include <falagard/CEGUIFalWidgetLookManager.h>]],
	    [CEGUI::WidgetLookManager::getSingleton();
	    #if (CEGUI_VERSION_MAJOR == 0) && (CEGUI_VERSION_MINOR < 7)
	    who took the ram from the ramalamadingdong?
	    #endif
	    ])],
	[], [C++],
	[CS_EMIT_BUILD_RESULT([cs_cv_libCEGUI], [CEGUI])], [], [],
	[$cs_cv_libCEGUI_cflags],
	[$cs_cv_libCEGUI_lflags],
	[$cs_cv_libCEGUI_libs])])


#------------------------------------------------------------------------------
# Check for wxWidgets. Also check if it was built with OpenGL support since it
# is required by the Crystal Space wxWidgets canvas.
#------------------------------------------------------------------------------
CS_CHECK_LIB_WITH([wx],
    [AC_LANG_PROGRAM(
	[[#include "wx/wxprec.h"
	#ifdef __BORLANDC__
	#pragma hdrstop
	#endif
	#ifndef WX_PRECOMP
	#include "wx/wx.h"
	#endif
	]],
	[[wxWindow* p = 0; int w,h; p->GetClientSize(&w,&h);]])],
    [], [C++])
    
AS_IF([test $cs_cv_libwx = yes],
    [AS_IF([test $cs_host_family = windows],
	[AC_CACHE_CHECK([if defining UNICODE is needed for WX],
	    [cs_cv_libwx_needs_UNICODE],
	    [CS_BUILD_IFELSE(
		[AC_LANG_PROGRAM(
		    [[#include <windows.h>
		    #include "wx/wxprec.h"
		    #ifdef __BORLANDC__
		    #pragma hdrstop
		    #endif
		    #ifndef WX_PRECOMP
		    #include "wx/wx.h"
		    #endif
		    ]],
		    [wxString s])],
		[CS_CREATE_TUPLE(
		    [$cs_cv_libwx_cflags],
		    [$cs_cv_libwx_lflags],
		    [$cs_cv_libwx_libs   $cs_cv_libwx_gllibs]) \
		CS_CREATE_TUPLE(
		    [$cs_cv_libwx_cflags $cs_cv_libglu_cflags],
		    [$cs_cv_libwx_lflags $cs_cv_libglu_lflags],
		    [$cs_cv_libwx_libs   $cs_cv_libglu_libs])  \
		CS_CREATE_TUPLE(
		    [$cs_cv_libwx_cflags $cs_cv_libglu_cflags],
		    [$cs_cv_libwx_lflags $cs_cv_libglu_lflags],
		    [$cs_cv_libwx_libs   $cs_cv_libwx_gllibs $cs_cv_libglu_libs])],
		[C++],
		[cs_cv_libwx_needs_UNICODE=no],
		[cs_cv_libwx_cflags="$cs_cv_libwx_cflags -DUNICODE -D_UNICODE"
		cs_cv_libwx_needs_UNICODE=yes])])])
    AS_IF([test -z "$cs_cv_libwx_opengl"],
	[AS_IF([test -n "$CONFIG_WX"],
	    [cs_cv_libwx_gllibs=CS_RUN_PATH_NORMALIZE([$CONFIG_WX --gl-libs])],
	    [cs_cv_libwx_gllibs=''])])
    CS_CHECK_BUILD([if wxWidgets is built with OpenGL support],
	[cs_cv_libwx_opengl],
	[AC_LANG_PROGRAM(
	    [[#include "wx/wxprec.h"
	    #ifdef __BORLANDC__
	    #pragma hdrstop
	    #endif
	    #ifndef WX_PRECOMP
	    #include "wx/wx.h"
	    #endif
	    #include "wx/glcanvas.h"
	    ]],
	    [wxGLCanvas* f = new wxGLCanvas(0, wxID_ANY, wxPoint());
	    glVertex3f(0,0,0)])],
	[CS_CREATE_TUPLE(
	    [$cs_cv_libwx_cflags],
	    [$cs_cv_libwx_lflags],
	    [$cs_cv_libwx_libs   $cs_cv_libwx_gllibs]) \
	CS_CREATE_TUPLE(
	    [$cs_cv_libwx_cflags $cs_cv_libglu_cflags],
	    [$cs_cv_libwx_lflags $cs_cv_libglu_lflags],
	    [$cs_cv_libwx_libs   $cs_cv_libglu_libs])  \
	CS_CREATE_TUPLE(
	    [$cs_cv_libwx_cflags $cs_cv_libglu_cflags],
	    [$cs_cv_libwx_lflags $cs_cv_libglu_lflags],
	    [$cs_cv_libwx_libs   $cs_cv_libwx_gllibs $cs_cv_libglu_libs])],
	[C++])],
    [cs_cv_libwx_opengl=no])
    
CS_EMIT_BUILD_RESULT([cs_cv_libwx], [WX])

AS_IF([test $cs_cv_libwx = yes],
    [CS_CHECK_BUILD([if wxWidgets is built with wxAUI support],
	[cs_cv_libwx_aui],
	[AC_LANG_PROGRAM(
	    [[#include "wx/wxprec.h"
	    #ifdef __BORLANDC__
	    #pragma hdrstop
	    #endif
	    #ifndef WX_PRECOMP
	    #include "wx/wx.h"
	    #endif
	    #include "wx/aui/aui.h"
	    ]],
	    [wxAuiManager mgr; wxWindow* w;
	     mgr.SetManagedWindow(w);
	     mgr.AddPane(w, wxAuiPaneInfo());])],
	[CS_CREATE_TUPLE(
	    [$cs_cv_libwx_cflags],
	    [$cs_cv_libwx_lflags],
	    [$cs_cv_libwx_libs])],
	[C++])],
    [cs_cv_libwx_aui=no])

AS_IF([test $cs_cv_libwx_aui = yes],
    [CS_EMIT_BUILD_RESULT([cs_cv_libwx_aui], [WX.AUI])])


#------------------------------------------------------------------------------
# Determine if plugin meta-information should be embedded directly into plugin
# modules or if it should exist in stand-alone .csplugin files.  On Unix,
# embedding is accomplished via libbfd which carries a GPL license.  If your
# project is compatible with GPL, then set gpl_compatible to "yes".  This will
# cause embedding to be enabled on Unix by default. Otherwise, set it to "no",
# which will cause embedding to be disabled by default on Unix. (In any event,
# the user can override the default setting with the
# --enable-meta-info-embedding option.)
#------------------------------------------------------------------------------
m4_define([gpl_compatible], [yes])
CS_META_INFO_EMBED([emit], gpl_compatible)
AS_IF([test $enable_meta_info_embedding = yes],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS],
	[AS_ESCAPE([$(EMBED_META.CFLAGS)])], [+])
    CS_EMIT_BUILD_PROPERTY([COMPILER.LFLAGS],
	[AS_ESCAPE([$(EMBED_META.LFLAGS)])], [+])])

#----------------------------------------------------------------------------
# Package configuration switches.
#----------------------------------------------------------------------------
AC_ARG_ENABLE([debug],
    [AC_HELP_STRING([--enable-debug],
	[build with debugging information (default NO)])],
    [], [enable_debug=no])

AC_MSG_CHECKING([build mode])
AS_IF([test $enable_debug = yes], [build_mode=debug], [build_mode=optimize])
AC_MSG_RESULT([$build_mode])

CS_EMIT_BUILD_PROPERTY([MODE], [$build_mode])

#-----------------------------------------------------------------------------
# Emit install paths and package information.
#-----------------------------------------------------------------------------
CS_OUTPUT_INSTALLDIRS
CS_EMIT_PACKAGEINFO

#----------------------------------------------------------------------------
# Emit generated files.
#----------------------------------------------------------------------------
CS_JAMCONFIG_OUTPUT([Jamconfig])
AC_CONFIG_FILES([Jamfile])
AC_OUTPUT

AC_MSG_NOTICE([

Please note that this project uses Jam as its build tool,
therefore you must type 'jam' to build the project.

Jam home: http://www.perforce.com/jam/jam.html
Jam source: ftp://ftp.perforce.com/jam/
])
