# Microsoft Developer Studio Project File - Name="JxtaShell" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=JxtaShell - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "JxtaShell.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "JxtaShell.mak" CFG="JxtaShell - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "JxtaShell - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "JxtaShell - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "JxtaShell - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\src" /I "..\..\shell" /I "..\..\lib\Apache2\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 jxta.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libapr.lib libaprutil.lib wsock32.lib xml.lib /nologo /subsystem:console /incremental:yes /machine:I386 /libpath:"..\..\lib\Apache2\lib"
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "JxtaShell - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\src" /I "..\..\shell" /I "..\..\lib\Apache2\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 jxtad.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libapr.lib libaprutil.lib wsock32.lib xml.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt" /pdbtype:sept /libpath:"$(BERKELEYDB)\lib" /libpath:"..\..\lib\Apache2\lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "JxtaShell - Win32 Release"
# Name "JxtaShell - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\shell\cat.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\groups.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\join.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\jxta_shell_app_env.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\jxta_shell_application.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\jxta_shell_environment.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\jxta_shell_getopt.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\jxta_shell_main.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\jxta_shell_object.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\jxta_shell_tokenizer.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\kdb.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\leave.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\peers.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\rdvstatus.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\search.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\talk.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\TestApplication.c
# End Source File
# Begin Source File

SOURCE=..\..\shell\whoami.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\shell\include\bin\jxta_shell_app_env.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\jxta_shell_application.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\jxta_shell_environment.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\jxta_shell_getopt.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\jxta_shell_main.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\jxta_shell_object.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\jxta_shell_tokenizer.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\bin\peers.h
# End Source File
# Begin Source File

SOURCE=..\..\shell\include\bin\TestApplication.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
