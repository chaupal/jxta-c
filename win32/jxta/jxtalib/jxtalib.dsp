# Microsoft Developer Studio Project File - Name="jxtalib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=jxtalib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "jxtalib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "jxtalib.mak" CFG="jxtalib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jxtalib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "jxtalib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "jxtalib"
# PROP Scc_LocalPath "..\..\.."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "jxtalib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /O2 /D "WIN32" /D "_NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /GX /Zd /O2 /I "..\..\..\lib\apache2\include" /I "..\..\..\src" /D "WIN32" /D "_NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\jxta.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Release\jxta.lib "c:\Program Files\Microsoft Visual Studio\VC98\Lib"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "jxtalib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "..\..\..\lib\apache2\include" /I "..\..\..\src" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD  /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\jxtad.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\jxtad.lib "c:\Program Files\Microsoft Visual Studio\VC98\Lib"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "jxtalib - Win32 Release"
# Name "jxtalib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\src\jpr\inet_ntop.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jdlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jpr\jpr_ckcompat.c

!IF  "$(CFG)" == "jxtalib - Win32 Release"

!ELSEIF  "$(CFG)" == "jxtalib - Win32 Debug"

# ADD CPP /I "D:\Viewstorage\NDixit_view3\vzshare\src\Client\Windows\Jxta-c\jxta-c\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\src\jpr\jpr_excep.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jpr\jpr_thread.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jstring.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_advertisement.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_apa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_bidipipe.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_builtinmodules.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_builtinmodules_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_bytevector.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_cm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_cred.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_cred_null.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_debug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_defloader.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_defloader_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_discovery_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_discovery_service_ref.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_dq.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_dr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_endpoint_address.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_endpoint_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_hashtable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_hta.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_id.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_id_jxta.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_id_uuid.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_incoming_unicast_server.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_listener.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_log.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_mca.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_membership_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_membership_service_null.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_message.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_mia.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_module.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_module_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_msa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_netpg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_netpg_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_object.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_object_ptrwrapper.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_object_type.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_objecthashtable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_pa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_peer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_peergroup.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_peergroup_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_pga.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_pipe_adv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_pipe_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_piperesolver_impl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_piperesolver_msg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_pm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rdv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rdv_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rdv_service_client.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rdv_service_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rdv_service_provider.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_relay.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_relaya.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_resolver_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_resolver_service_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_resolver_service_ref.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_routea.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_router_client.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rq.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rsrdi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_service_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_socket_tunnel.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_srdi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_stdpg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_string.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_svc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_tcp_message_packet_header.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_tcp_multicast.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_http.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_http_client.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_http_client.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_http_poller.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_http_poller.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_tcp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_tcp_connection.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_tta.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_unipipe_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_vector.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_wire_service.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_wm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_xml_util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxtaid_priv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\queue.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\queue.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\trailing_average.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\include\jpr\jpr_errno.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jpr\jpr_excep.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jpr\jpr_thread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jpr\arch\win32\jpr_threadonce.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jpr\jpr_threadonce.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jpr\jpr_types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jxta_apa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_bidipipe.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_endpoint_messenger.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_incoming_unicast_server.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_log_priv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_rdv_service_provider.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jxta_relaya.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jxta_routea.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jxta_rsrdi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_socket_tunnel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\jxta_srdi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_tcp_message_packet_header.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_tcp_multicast.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_tcp_connection.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\jxta_transport_tcp_private.h
# End Source File
# End Group
# End Target
# End Project
