
AM_CPPFLAGS = -I../src @APR_CPPFLAGS@ @XML2_CPPFLAGS@ @SQLITE_CPPFLAGS@

AM_CFLAGS = -DSTANDALONE -DTARGET_ARCH_OS_PLATFORM=\"$(target)\" -Wall -W -pedantic -Wno-long-long -Wstrict-prototypes -Wwrite-strings -Wpointer-arith

bin_PROGRAMS = jxtaShell

LDADD = @APR_LIBS@                  \
        @XML2_LIBS@                 \
        @SQLITE_LIBS@               \
	-L../src/                   \
	-L../src/.libs              \
	-L../src/jpr                \
	-L../src/jpr/.libs          \
	-lpthread                   \
        -ljpr                       \
        -ljxta


jxtaShell_SOURCES =  jxta_shell_application.c \
                     jxta_shell_environment.c \
                     jxta_shell_getopt.c  \
                     jxta_shell_main.c    \
                     jxta_shell_object.c  \
                     jxta_shell_tokenizer.c  \
                     \
                     TestApplication.c \
                     cat.c   \
                     groups.c  \
                     join.c  \
                     jxta_shell_app_env.c  \
                     kdb.c  \
                     leave.c  \
                     peers.c  \
                     publish.c   \
                     rdvcontrol.c  \
                     rdvstatus.c  \
                     search.c  \
                     talk.c  \
                     whoami.c  \
		     jxta_shell_application.h  \
                     jxta_shell_environment.h  \
                     jxta_shell_getopt.h       \
                     jxta_shell_main.h         \
		     jxta_shell_object.h       \
		     jxta_shell_tokenizer.h    \
		     TestApplication.h    \
		     cat.h                \
		     groups.h             \
		     join.h               \
		     jxta_shell_app_env.h \
		     kdb.h                \
		     leave.h              \
		     peers.h              \
                     publish.h            \
		     rdvcontrol.h          \
		     rdvstatus.h          \
		     search.h             \
		     talk.h               \
		     whoami.h 
