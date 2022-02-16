#			-*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

#base
AC_INIT([libvpd],[2.2.8],[hegdevasant@linux.vnet.ibm.com])
AC_CONFIG_HEADER([config/config.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall subdir-objects -Werror foreign])
AC_SUBST(DATE, [`date`])
AM_PROG_AR

#Generic variable setup
GENERIC_LIBRARY_NAME=libvpd

#Release version
GENERIC_MAJOR_VERSION=2
GENERIC_MINOR_VERSION=2
GENERIC_MICRO_VERSION=8

GENERIC_API_VERSION=$GENERIC_MAJOR_VERSION
AC_SUBST(GENERIC_API_VERSION)

#shared library versioning => current[:revision[:age]]
#https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
GENERIC_LIBRARY_VERSION=3:0:0

GENERIC_CXX_LIBRARY_VERSION=$GENERIC_LIBRARY_VERSION
GENERIC_C_LIBRARY_VERSION=$GENERIC_LIBRARY_VERSION
AC_SUBST(GENERIC_CXX_LIBRARY_VERSION)
AC_SUBST(GENERIC_C_LIBRARY_VERSION)


AC_SUBST(GENERIC_LIBRARY_VERSION)

PACKAGE=$GENERIC_LIBRARY_NAME
AC_SUBST(GENERIC_LIBRARY_NAME)

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

VERSION=$GENERIC_VERSION

#programs
AC_LANG([C++])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

#headers
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h sys/param.h syslog.h unistd.h ctype.h arpa/inet.h netinet/in.h])
AC_CHECK_HEADERS([sqlite3.h],,[AC_MSG_ERROR([sqlite header files are required for building libvpd])])


#libraries
AC_CHECK_LIB(sqlite3, sqlite3_libversion_number, [], [
			echo "sqlite3 library is required for lsvpd"
			exit 1 ])
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([memmove memset mkdir socket strdup strerror strtol uname sqlite3_prepare_v2])

#finished
AC_CONFIG_FILES([Makefile libvpd.spec libvpd-2.pc libvpd_cxx-2.pc])
AC_OUTPUT
