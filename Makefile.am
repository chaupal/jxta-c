
SUBDIRS = src test shell tutorials

EXTRA_DIST = doxconf \
		win32 \
		lib/libxml2

dist-hook:
	rm -fr `find $(distdir)/win32 -name CVS -o -name .cvsignore`
	rm -fr `find $(distdir)/lib -name CVS -o -name .cvsignore`

dox:
	doxygen doxconf

