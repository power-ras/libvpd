lib_hpp_files = src/libvpd-2/vpdretriever.hpp \
		src/libvpd-2/system.hpp \
		src/libvpd-2/component.hpp \
		src/libvpd-2/vpdexception.hpp \
		src/libvpd-2/dataitem.hpp \
		src/libvpd-2/Source.hpp \
		src/libvpd-2/logger.hpp \
		src/libvpd-2/lsvpd.hpp \
		src/libvpd-2/debug.hpp \
		src/libvpd-2/helper_functions.hpp \
		src/libvpd-2/lsvpd_error_codes.hpp \
		src/libvpd-2/vpddbenv.hpp

lib_h_files = src/libvpd-2/vpdretriever.h \
		src/libvpd-2/system.h \
		src/libvpd-2/component.h \
		src/libvpd-2/dataitem.h \
		src/libvpd-2/common.h \
		src/libvpd-2/vpddbenv.h

EXTRA_DIST = bootstrap.sh 90-vpdupdate.rules run.vpdupdate

lib_LTLIBRARIES = libvpd_cxx.la libvpd.la
library_includedir=$(includedir)/libvpd-2/
library_include_HEADERS = $(lib_hpp_files) $(lib_h_files)

AM_CPPFLAGS = -I./src/ -Wall -fstack-protector-all -fPIE -Wstack-protector
ACLOCAL_AMFLAGS = -I m4

libvpd_la_SOURCES = src/vpdretriever_c.c \
		src/common.c \
		src/vpddbenv_c.c \
		src/system_c.c \
		src/component_c.c \
		src/dataitem_c.c \
		$(lib_h_files)

libvpd_cxx_la_SOURCES = src/vpdretriever.cpp \
		src/helper_functions.cpp \
		src/vpddbenv.cpp \
		src/logger.cpp \
		src/system.cpp \
		src/component.cpp \
		src/vpdexception.cpp \
		src/dataitem.cpp \
		src/Source.cpp \
		$(lib_hpp_files)
		
CXX_VERSION=@GENERIC_CXX_LIBRARY_VERSION@
C_VERSION=@GENERIC_C_LIBRARY_VERSION@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libvpd-2.pc libvpd_cxx-2.pc

libvpd_cxx_la_LDFLAGS = -module -lstdc++ -version-info \
	$(CXX_VERSION) -release @GENERIC_RELEASE@
libvpd_la_LDFLAGS = -module -version-info \
	$(C_VERSION) -release @GENERIC_RELEASE@

AM_CXXFLAGS = -DDEST_DIR='"${exec_prefix}"'
AM_CFLAGS = -DDEST_DIR='"${exec_prefix}"'

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

install-exec-hook:
	mkdir -p $(DESTDIR)/${sysconfdir}/udev/rules.d/
	mkdir -p $(DESTDIR)/${localstatedir}/lib/lsvpd/
	install -D --mode=644 90-vpdupdate.rules \
                ${DESTDIR}/${sysconfdir}/udev/rules.d/90-vpdupdate.rules
