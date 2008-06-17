
SUBDIRS = src test shell @CS_DIR@ tutorials

EXTRA_DIST = doxconf.in \
		win32 \
		lib/libxml2 \
		jxta.net \
		jxta.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = jxta.pc

dist-hook:
	rm -fr `find $(distdir)/win32 -name .svn -o -name .svnignore`
	rm -fr `find $(distdir)/lib -name .svn -o -name .svnignore`
	rm -fr `find $(distdir)/jxta.net -name .svn -o -name .svnignore`


dox:
	doxygen doxconf

