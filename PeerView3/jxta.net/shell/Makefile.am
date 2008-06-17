scriptdir = $(DESTDIR)$(prefix)/bin
assemblydir = $(DESTDIR)$(prefix)/lib/mono/jxta.net

TARGET_NAME = JxtaNETShell
TARGET = $(TARGET_NAME).exe
MONO = @MONO@
MONO_FLAGS = @MONO_FLAGS@

CS_FILES = RdvControl.cs AssemblyInfo.cs Search.cs Peers.cs RdvStatus.cs Shell.cs ShellApp.cs Talk.cs WhoAmI.cs
		
$(TARGET): $(CS_FILES)
	$(GMCS) $(GMCS_FLAGS) -out:$(TARGET) -reference:$(top_srcdir)/jxta.net/src/JxtaNET.dll $(CS_FILES)

$(TARGET_NAME):
	echo "#!/bin/sh" > $(TARGET_NAME)
	echo -e "export MONO_GAC_PREFIX=\044MONO_GAC_PREFIX:/usr" >> $(TARGET_NAME)
	echo -e "export MONO_PATH=\044MONO_PATH:@prefix@/lib/mono/jxta.net" >> $(TARGET_NAME)
	echo "# export MONO_LOG_LEVEL=debug" >> $(TARGET_NAME)
	echo -e "exec $(MONO) $(MONO_FLAGS) $(prefix)/lib/mono/jxta.net/$(TARGET) \044@" >> $(TARGET_NAME)

all:
	$(MAKE) $(TARGET)

install-data-local: $(TARGET)
	rm -rf $(TARGET_NAME)
	$(MAKE) $(TARGET_NAME)
	$(mkinstalldirs) $(assemblydir)
	$(INSTALL_PROGRAM) $(TARGET) $(assemblydir)
	$(INSTALL_PROGRAM) $(TARGET_NAME) $(scriptdir)
	rm -rf $(TARGET_NAME)

CLEANFILES = $(TARGET) $(TARGET_NAME)

