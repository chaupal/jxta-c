
AM_CPPFLAGS = @APR_CPPFLAGS@

AM_CFLAGS = -DTARGET_ARCH_OS_PLATFORM=\"$(target)\" -Wall -W -pedantic -Wno-long-long -Wstrict-prototypes -Wwrite-strings -Wpointer-arith

jprincludedir = $(includedir)/jpr

lib_LTLIBRARIES = libjpr.la

libjpr_la_LIBADD = @APR_LIBS@ @SQLITE_LIBS@

nobase_jprinclude_HEADERS =  jpr_apr_wrapper.h	\
			     jpr_core.h		\
                             jpr_errno.h        \
			     jpr_excep.h        \
			     jpr_excep_proto.h  \
			     jpr_setjmp.h       \
			     jpr_types.h
         
libjpr_la_SOURCES = inet_ntop.c		 \
		    jpr.c                \
                    jpr_ckcompat.c       \
                    jpr_excep.c          \
		    jpr_types.c          \
                    jpr_priv.h
