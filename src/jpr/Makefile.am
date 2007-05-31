
AM_CPPFLAGS = @APR_CPPFLAGS@ -I${prefix}/include -I..

AM_CFLAGS = -Wall -W -pedantic -Wno-long-long -Wstrict-prototypes -Wwrite-strings -Wpointer-arith -g

jprincludedir = $(includedir)/jpr

lib_LTLIBRARIES = libjpr.la

nobase_jprinclude_HEADERS =  jpr_errno.h        \
			     jpr_excep.h        \
			     jpr_excep_proto.h  \
			     jpr_setjmp.h       \
			     jpr_thread.h       \
			     jpr_threadonce.h   \
			     jpr_types.h	\
			     jpr_apr_wrapper.h	\
			     arch/beos/jpr_threadonce.h \
			     arch/netware/jpr_threadonce.h \
			     arch/os2/jpr_threadonce.h \
			     arch/unix/jpr_threadonce.h \
			     arch/win32/jpr_threadonce.h 
         
libjpr_la_SOURCES = jpr_excep.c          \
                    jpr_ckcompat.c       \
                    jpr_thread.c	 \
		    inet_ntop.c
