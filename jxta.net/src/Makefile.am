assemblydir = $(DESTDIR)$(prefix)/lib/mono/jxta.net

TARGET = JxtaNET.dll

CS_FILES =	Advertisement.cs \
		JxtaObject.cs \
		PeerGroupAdvertisement.cs \
		JxtaString.cs \
		PeerGroup.cs \
		AssemblyInfo.cs \
		JxtaVector.cs \
		ByteStream.cs \
		Listener.cs \
		Credential.cs \
		MembershipService.cs \
		ID.cs \
		DiscoveryResponse.cs \
		Message.cs \
		PeerView.cs \
		DiscoveryService.cs \
		MessageElement.cs \
		PipeAdvertisement.cs \
		Errors.cs \
		ModuleClassAdvertisement.cs \
		Pipe.cs \
		OutputPipe.cs \
		PipeService.cs \
		InputPipe.cs \
		PeerAdvertisement.cs \
		RendezVousService.cs \
		jxtaDll.cs \
		Peer.cs \
		Log.cs


$(TARGET): $(CS_FILES)
	$(GMCS) $(GMCS_FLAGS) -target:library -out:$(TARGET) $(CS_FILES)

all:
	$(MAKE) $(TARGET)

install-data-local: $(TARGET)
	$(mkinstalldirs) $(assemblydir)
	$(INSTALL_PROGRAM) $(TARGET) $(assemblydir)
	$(INSTALL_DATA) $(TARGET).config $(assemblydir)

CLEANFILES = $(TARGET)

