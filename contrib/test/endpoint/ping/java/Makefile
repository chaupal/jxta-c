#
# $Id: Makefile,v 1.2 2002/02/23 00:11:35 hamada Exp $
#


# if need to use a particular JDK set JAVA_HOME in your env.
# if you reference additional libraries they need to be set in the
# CLASSPATH
# 

ifneq ($(JAVA_HOME),)
 JAVAHOMEBIN      = $(JAVA_HOME)/bin/
else
 JAVAHOMEBIN      =
endif

MAKE	      = make
ZIP	      = zip
JAVA          = $(JAVAHOMEBIN)java
JAVAC         = $(JAVAHOMEBIN)javac
JAVADOC       = $(JAVAHOMEBIN)javadoc
JAR           = $(JAVAHOMEBIN)jar
CP	      = cp -f
HEADER	      = "Project JXTA"
JXTA	      = "Project JXTA (TM) Technology 0.1 JXTA-C java doc"
ifeq ($(TOP),)
TOP           = $(shell pwd)/../../../../../..
TESTTOP           = $(shell pwd)
endif
CLASSDIR      = $(TESTTOP)/classes
PKGDIR        = $(TESTTOP)/pkg
JAVAFILES  = $(shell find src  -name '*.java' -print | grep -v -w 'CVS')

ifeq ($(PLATFORM),)
 PLATFORM = $(TOP)/platform/binding/java
 PLATFORMCLASSES = $(PLATFORM)/classes
 PLATFORMLIB = $(PLATFORMCLASSES)/jxta.jar
endif

ifeq ($(JXTASHELL),)
 JXTASHELL = $(TOP)/shell/binding/java
 JXTASHELLCLASSES = $(JXTASHELL)/classes
 JXTASHELLLIB = $(JXTASHELLCLASSES)/jxtashell.jar
endif

ifeq ($(JETTYLIB),)
 JETTYLIB = $(TOP)/platform/binding/java/lib/org.mortbay.jetty.jar
endif

ifeq ($(LOG4JLIB),)
 LOG4JLIB = $(TOP)/platform/binding/java/lib/log4j.jar
endif

ifeq ($(JETTYLIB),)
 JETTYLIB = $(TOP)/platform/binding/java/lib/org.mortbay.jetty.jar
endif

ifeq ($(SERVLETLIB),)
 SERVLETLIB = $(TOP)/platform/binding/java/lib/servlet.jar
endif

ifeq ($(JXTASECLIB),)
 JXTASECLIB = $(TOP)/platform/binding/java/lib/jxtasecurity.jar
endif

ifeq ($(JXTAPTLSLIB),)
 JXTAPTLSLIB = $(TOP)/platform/binding/java/lib/jxtaptls.jar
endif

ifeq ($(JXTAMINIBCLIB),)
 JXTAMINIBCLIB = $(TOP)/platform/binding/java/lib/minimalBC.jar
endif

ifeq ($(JXTACRYPTIXASN1LIB),)
 JXTACRYPTIXASN1LIB = $(TOP)/platform/binding/java/lib/cryptix-asn1.jar
endif

ifeq ($(JXTACRYPTIX32LIB),)
 JXTACRYPTIX32LIB = $(TOP)/platform/binding/java/lib/cryptix32.jar
endif

ifeq ($(JXTATLSLIB),)
 JXTATLSLIB = $(TOP)/platform/binding/java/lib/cryptix-asn1.jar
endif

PKGJARNAME = "j2sepeer.jar"
ZIPFILE = "j2sepeer.zip"

ifneq ($(CLASSPATH),)
 JXTACLASSPATH=$(CLASSPATH)$(CPSEP)$(CLASSDIR)$(CPSEP)$(PLATFORMLIB)$(CPSEP)$(JXTASHELLLIB)$(CPSEP)$(JXTAXMLLIB)$(CPSEP)$(LOG4JLIB)$(CPSEP)$(SERVLETLIB)$(CPSEP)$(JETTYLIB)$(CPSEP)$(JXTATLSLIB)$(CPSEP)$(JXTACRYPTIX32LIB)$(CPSEP)$(JXTACRYPTIXASN1LIB)$(CPSEP)$(JXTAMINIBCLIB)$(CPSEP)$(JXTAPTLSLIB)$(CPSEP)$(JXTASECLIB)
else
 JXTACLASSPATH=$(CLASSDIR)$(CPSEP)$(PLATFORMLIB)$(CPSEP)$(JXTASHELLLIB)$(CPSEP)$(JXTAXMLLIB)$(CPSEP)$(LOG4JLIB)$(CPSEP)$(SERVLETLIB)$(CPSEP)$(JETTYLIB)$(CPSEP)$(JXTATLSLIB)$(CPSEP)$(JXTACRYPTIX32LIB)$(CPSEP)$(JXTACRYPTIXASN1LIB)$(CPSEP)$(JXTAMINIBCLIB)$(CPSEP)$(JXTAPTLSLIB)$(CPSEP)$(JXTASECLIB)
endif

ifeq (true,$(OPTIMIZE))
 JAVACOPT=-O -g:none
else
 JAVACOPT=
endif

#
# Cygwin uses the MS-Windows style classpath, so "fix" it for them
#
ifeq (cygwin,$(OSTYPE))
  CPSEP = ";"
  TOP := "$(shell cygpath --path --windows "$(TOP)")"
  JXTACLASSPATH := "$(shell cygpath --path --windows "$(JXTACLASSPATH)")"
#  JAVAHOMEBIN := "$(shell cygpath --path --windows "$(JAVAHOMEBIN)")"
else
  CPSEP = ":"
endif

cmds:
	@echo building ALL using $(JAVAC)
	@echo CLASSPATH = $(JXTACLASSPATH)
	echo  SHELL= $(JXTASHELLLIB)
	@if [ '!' -d $(CLASSDIR) ]; then mkdir $(CLASSDIR); fi;
	@$(JAVAC) $(JAVACOPT) -d $(CLASSDIR)  -classpath $(JXTACLASSPATH) $(JAVAFILES)
	@echo Done building all.

run:
	$(JAVA) -classpath $(JXTACLASSPATH)  net.jxta.impl.peergroup.Boot

zip:	pkg
	$(ZIP) $(ZIPFILE) $(PKGJARNAME) PlatformConfig run.sh

pkg:	cleanpkg
	@rm -rf $(PKGDIR)
	@echo Building the JXTA-Standalone local J2SE peer jar file
	@if [ '!' -d $(PKGDIR) ]; then mkdir -p $(PKGDIR); fi;
	@cd $(PKGDIR); \
	$(JAR) xf $(PLATFORMLIB); \
	$(JAR) xf $(JXTASHELLLIB); \
	$(JAR) xf $(LOG4JLIB); \
	$(JAR) xf $(SERVLETLIB); \
	$(JAR) xf $(JETTYLIB); \
	$(JAR) xf $(JXTASECLIB); \
	$(JAR) xf $(JXTAMINIBCLIB); \
	$(JAR) xf $(JXTACRYPTIXASN1LIB); \
	$(JAR) xf $(JXTACRYPTIX32LIB); \
	$(JAR) xf $(JXTATLSLIB); \
	$(JAR) xf $(JXTAPTLSLIB)
	@cp -r classes/* $(PKGDIR)
	@cd $(PKGDIR); \
	$(JAR) cf $(TESTTOP)/$(PKGJARNAME) .
	@echo 
	@echo Pakaging done. Jar file is $(TESTTOP)/$(PKGJARNAME)
	@echo To run it:
	@echo 
	@echo "     make run"
	@echo or
	@echo "    " $(JAVA) " -classpath " $(PKGJARNAME) " net.jxta.impl.peergroup.Boot"
	@echo

clean: cleanpkg
	@rm -rf classes
cleanpkg:
	@rm -rf $(PKGDIR)

clobber: clean

help:
	@echo "# Usage : gnumake [all] [compile] [jar] [run] [doc] [javadoc] [refdoc] [clean] [clobber] [help]"






