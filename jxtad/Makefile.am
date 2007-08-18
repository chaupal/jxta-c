AM_CPPFLAGS = -I$(top_srcdir)/src @APR_CPPFLAGS@ @XML2_CPPFLAGS@ @SQLITE_CPPFLAGS@

AM_CFLAGS = -DSTANDALONE -DTARGET_ARCH_OS_PLATFORM=\"$(target)\" 

bin_PROGRAMS = jxtad

LDADD = \
	$(top_builddir)/src/libjxta.la \
	@APR_LIBS@              \
        @XML2_LIBS@             \
        @SQLITE_LIBS@

jxtad_SOURCES = jxtad.c 
