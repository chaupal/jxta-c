
AM_CPPFLAGS = @APR_CPPFLAGS@ 

AM_CFLAGS = -DTARGET_ARCH_OS_PLATFORM=\"$(target)\" 

jprincludedir = $(includedir)/jpr

lib_LTLIBRARIES = libjpr.la

libjpr_la_LDFLAGS = -static

libjpr_la_LIBADD = @APR_LIBS@ @SQLITE_LIBS@

if APR_THREADPOOL
THREADPOOL_SRC_FILES = 
THREADPOOL_HDR_FILES = 
else
THREADPOOL_SRC_FILES = apr_thread_pool.c
THREADPOOL_HDR_FILES = apr_thread_pool.h
endif

nobase_jprinclude_HEADERS =  jpr_apr_wrapper.h	\
			     jpr_core.h		\
                             jpr_errno.h        \
			     jpr_excep.h        \
			     jpr_excep_proto.h  \
			     jpr_setjmp.h       \
			     jpr_types.h \
			     $(THREADPOOL_HDR_FILES)
         
libjpr_la_SOURCES = inet_ntop.c		 \
		    jpr.c                \
                    jpr_ckcompat.c       \
                    jpr_excep.c          \
		    jpr_types.c          \
                    jpr_priv.h           \
		    $(THREADPOOL_SRC_FILES)
