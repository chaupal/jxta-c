
SUBDIRS = src test shell @CS_DIR@ tutorials jxtad

EXTRA_DIST = doxconf.in \
		win32 \
		lib/libxml2 \
		jxta.net \
		jxta.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = jxta.pc

dist-hook:
	rm -fr `find $(distdir)/win32 -name CVS -o -name .cvsignore`
	rm -fr `find $(distdir)/lib -name CVS -o -name .cvsignore`
	rm -fr `find $(distdir)/jxta.net -name CVS -o -name .cvsignore`

dox:
	doxygen doxconf

