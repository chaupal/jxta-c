
AM_CFLAGS = -DSTANDALONE -DTARGET_ARCH_OS_PLATFORM=\"$(target)\" -Wall -W -pedantic -Wno-long-long -Wstrict-prototypes -Wwrite-strings -Wpointer-arith

AM_CPPFLAGS =  -I../src @APR_CPPFLAGS@ @XML2_CPPFLAGS@ @SQLITE_CPPFLAGS@ 

bin_PROGRAMS = tutorial0 tutorial1 tutorial2 whisp
		      
#
#other tutorials project using the same structure       
#		tutorial1

LDADD = @APR_LIBS@                  \
        @XML2_LIBS@                 \
        @SQLITE_LIBS@               \
	-L../src/                   \
	-L../src/.libs              \
	-L../src/jpr                \
	-L../src/jpr/.libs          \
	  -lpthread                 \
          -ljpr                     \
          -ljxta

tutorial0_SOURCES =	jxta_tutorial.h \
			jxta_tutorial_shared.h \
			jxta_tutorial_main.c \
			jxta_tutorial_shared.c \
			jxta_tutorial0.c
			
tutorial1_SOURCES =	jxta_tutorial.h \
			jxta_tutorial_shared.h \
			jxta_tutorial_args.h \
			jxta_tutorial_main.c  \
			jxta_tutorial_shared.c \
			jxta_tutorial_args.c \
			jxta_tutorial1.c

tutorial2_SOURCES =	jxta_tutorial.h \
			jxta_tutorial_shared.h \
			jxta_tutorial_args.h \
			jxta_tutorial_main.c  \
			jxta_tutorial_shared.c \
			jxta_tutorial_args.c \
			jxta_tutorial2.c
			
			
whisp_SOURCES =		jxta_tutorial.h \
			jxta_tutorial_shared.h \
			jxta_tutorial_args.h \
			jxta_tutorial_main.c \
			jxta_tutorial_shared.c \
			jxta_tutorial_args.c \
			whisp.c 

#other tutorial# projects using the same structure build the same way

jxta_tutorial_main.o:  jxta_tutorial_main.c
	$(COMPILE) -DSTANDALONE -o jxta_tutorial_main.o -c jxta_tutorial_main.c

jxta_tutorial_shared.o:  jxta_tutorial_shared.c
	$(COMPILE) -DSTANDALONE -o jxta_tutorial_shared.o -c jxta_tutorial_shared.c

jxta_tutorial_args.o: jxta_tutorial_args.c
	$(COMPILE) -DSTANDALONE -o jxta_tutorial_args.o -c jxta_tutorial_args.c

jxta_tutorial0.o:  jxta_tutorial0.c
	$(COMPILE) -DSTANDALONE -o jxta_tutorial0.o -c jxta_tutorial0.c

jxta_tutorial1.o: jxta_tutorial1.c
	$(COMPILE) -DSTANDALONE -o jxta_tutorial1.o -c jxta_tutorial1.c

jxta_tutorial2.o: jxta_tutorial2.c
	$(COMPILE) -DSTANDALONE -o jxta_tutorial2.o -c jxta_tutorial2.c

whisp.o: whisp.c
	$(COMPILE) -DSTANDALONE -o whisp.o -c whisp.c

