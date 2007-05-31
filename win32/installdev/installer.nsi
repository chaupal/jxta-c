;VERSION: 1.00
;INFO: Nullsoft SuperPIMP Install System - Copyright (C)1999-2001 Nullsoft, Inc.
;WARNING ! Please do not modify the header

;-)Application_Name Jxta-c
;-)Application_Version 1.0.0.0
;-)Application_Main_EXE
;-)Add_Publisher "Sun Microsystems"
;-)Add_Contact
;-)Add_Publisher_URL www.sun.com
;-)Add_Support_URL www.jxta.org
;-)Add_Support_Phone
;-)Add_Product_URL www.jxta.org
;-)Add_Comments "OpenSource p2p platform"

Name Jxta-c
Caption Jxta-c
; Default variables
!define VER_MAJOR 1
!define VER_MINOR 0
!define APP_NAME Jxta-c
; ---
LicenseText "You must agree to this license before installing." 
LicenseData license.txt
BGGradient off
InstallColors 00FF00 000000
AutoCloseWindow false
ShowInstDetails show
InstProgressFlags smooth colored 
DirShow show
DirText "Select the directory to install Jxta-c in:"  
InstallDir $PROGRAMFILES\Jxta-c
InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Sun Microsystems\Jxta" ""
SilentInstall normal
OutFile Installer.exe
SubCaption 0 ""
SubCaption 1 ""
SubCaption 2 ""
SubCaption 3 ""
SubCaption 4 ""
UninstallText "This will uninstall Jxta-c from your system" 
;-)Uninstall_Create_Shortcut off
;-)Uninstall_Auto_Section on
UninstallSubCaption 0 ""
UninstallSubCaption 1 ""
UninstallSubCaption 2 ""

Section "";"(default section)"
SetOutPath "$INSTDIR"
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Sun Microsystems\Jxta" "" "$INSTDIR"
Call MkJxta_Directory
; WriteUninstaller "$INSTDIR\uninstaller.exe"
; Copy jxta
Call Copy_Jxta_root_core
Call Copy_Jxta_root_core_doc
Call Copy_Jxta_root_core_include
Call Copy_Jxta_root_core_include_jpr
Call Copy_Jxta_root_core_include_jpr_arch
Call Copy_Jxta_root_core_lib
;

;
;
;
;
SetOutPath "$INSTDIR\core\src"
File "C:\jxta-c\src\adparsegen.sh"
File "C:\jxta-c\src\Advertisement.c"
File "C:\jxta-c\src\DiscoveryQuery.c"
File "C:\jxta-c\src\DiscoveryResponse.c"
File "C:\jxta-c\src\EndpointRouter.c"
File "C:\jxta-c\src\endpoint_address.c"
File "C:\jxta-c\src\endpoint_service.c"
File "C:\jxta-c\src\error.c"
File "C:\jxta-c\src\HTTPTransportAdvertisement.c"
File "C:\jxta-c\src\http_client.c"
File "C:\jxta-c\src\http_client.h"
File "C:\jxta-c\src\http_poller.c"
File "C:\jxta-c\src\http_poller.h"
File "C:\jxta-c\src\http_transport.c"
File "C:\jxta-c\src\jdlist.c"
File "C:\jxta-c\src\jstring.c"
File "C:\jxta-c\src\jxtaid.c"
File "C:\jxta-c\src\jxtaid_jxta.c"
File "C:\jxta-c\src\jxtaid_priv.h"
File "C:\jxta-c\src\jxtaid_uuid.c"
File "C:\jxta-c\src\jxtaobject.c"
File "C:\jxta-c\src\jxta_advertisement.c"
File "C:\jxta-c\src\jxta_advertisement.h"
File "C:\jxta-c\src\jxta_boot.c"
File "C:\jxta-c\src\jxta_peer.c"
File "C:\jxta-c\src\jxta_peergroup.c"
File "C:\jxta-c\src\jxta_private.h"
File "C:\jxta-c\src\Makefile.am"
File "C:\jxta-c\src\message.c"
File "C:\jxta-c\src\mod_jxta.c"
File "C:\jxta-c\src\newads.sh"
File "C:\jxta-c\src\PA.c"
File "C:\jxta-c\src\PeerAdvertisement.c.OLD"
File "C:\jxta-c\src\PeerAdvertisement.h.OLD"
File "C:\jxta-c\src\peerlist.c"
File "C:\jxta-c\src\PGA.c"
File "C:\jxta-c\src\queue.c"
File "C:\jxta-c\src\queue.h"
File "C:\jxta-c\src\rdv_service.c"
File "C:\jxta-c\src\timer.c"
File "C:\jxta-c\src\timer.h"
;
SetOutPath "$INSTDIR\core\src\apache"
File "C:\jxta-c\src\apache\discovery_service.c"
File "C:\jxta-c\src\apache\discovery_service.h"
File "C:\jxta-c\src\apache\hash_query.c"
File "C:\jxta-c\src\apache\hash_query.h"
File "C:\jxta-c\src\apache\http_connection.h"
File "C:\jxta-c\src\apache\makefile"
File "C:\jxta-c\src\apache\Makefile.am"
File "C:\jxta-c\src\apache\mod_jxta.html"
File "C:\jxta-c\src\apache\mod_jxta_private.h"
File "C:\jxta-c\src\apache\NOTES"
File "C:\jxta-c\src\apache\registration.c"
File "C:\jxta-c\src\apache\registration.h"
File "C:\jxta-c\src\apache\relay_service.c"
File "C:\jxta-c\src\apache\router.c"
;
SetOutPath "$INSTDIR\core\src\jpr"
File "C:\jxta-c\src\jpr\jpr_ckcompat.c"
File "C:\jxta-c\src\jpr\jpr_excep.c"
;
SetOutPath "$INSTDIR\test"
File "C:\jxta-c\test\DiscoveryQuery.xml"
File "C:\jxta-c\test\DiscoveryResponse.xml"
File "C:\jxta-c\test\discovery_request_msg"
File "C:\jxta-c\test\dq_test.c"
File "C:\jxta-c\test\dr_test.c"
File "C:\jxta-c\test\error_test.c"
File "C:\jxta-c\test\excep_test.c"
File "C:\jxta-c\test\jstring_test.c"
File "C:\jxta-c\test\jxtaid_test.c"
File "C:\jxta-c\test\jxtaobject_test.c"
File "C:\jxta-c\test\jxtatcptest.c"
File "C:\jxta-c\test\Makefile.am"
File "C:\jxta-c\test\msg_test.c"
File "C:\jxta-c\test\pa_test.c"
File "C:\jxta-c\test\PeerGroupAdvertisement.xml"
File "C:\jxta-c\test\peerlist_test.c"
File "C:\jxta-c\test\PGA.xml"
File "C:\jxta-c\test\pga_test.c"
File "C:\jxta-c\test\PlatformConfig"
File "C:\jxta-c\test\sa_test.c"
File "C:\jxta-c\test\ServiceAdvertisement.xml"
File "C:\jxta-c\test\testall.sh"
File "C:\jxta-c\test\timer_test.c"
;
SetOutPath "$INSTDIR\win32\jxta"
File "C:\jxta-c\win32\jxta\jxta.dsw"
;
SetOutPath "$INSTDIR\win32\jxta\jxtalib"
File "C:\jxta-c\win32\jxta\jxtalib\jxtalib.dsw"
File "C:\jxta-c\win32\jxta\jxtalib\jxtalib.dsp"
;
SetOutPath "$INSTDIR\win32\confgui"       
File "C:\jxta-c\win32\confgui\jxtaconf.dsw"
File "C:\jxta-c\win32\confgui\jxtaconf.dsp"
;                                         
SetOutPath "$INSTDIR\win32\confgui\src"
File "C:\jxta-c\win32\confgui\src\confgui.cpp"
File "C:\jxta-c\win32\confgui\src\filebytes.c"
File "C:\jxta-c\win32\confgui\src\filebytes.h"
File "C:\jxta-c\win32\confgui\src\icon1.ico"
File "C:\jxta-c\win32\confgui\src\icon2.ico"
File "C:\jxta-c\win32\confgui\src\icon3.ico"
File "C:\jxta-c\win32\confgui\src\icon4.ico"
File "C:\jxta-c\win32\confgui\src\jxtaconf.cpp"
File "C:\jxta-c\win32\confgui\src\jxtaconf.h"
File "C:\jxta-c\win32\confgui\src\propsheet.cpp"
File "C:\jxta-c\win32\confgui\src\resource.h"
File "C:\jxta-c\win32\confgui\src\Script1.rc"

SectionEnd ; end of default section"


Section -Uninstall_Info
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "Comments" "OpenSource p2p platform"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "Contact" ""
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "DisplayName" "${APP_NAME} v${VER_MAJOR}.${VER_MINOR}"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "DisplayVersion" "1.0.0.0"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "HelpLink" "www.jxta.org"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "HelpTelephone" ""
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "Publisher" "Sun Microsystems"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "UninstallString" "$INSTDIR\uninst.exe"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "URLInfoAbout" "www.sun.com"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}" "URLUpdateInfo" "www.jxta.org"
WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd

Section Uninstall
DeleteRegKey HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME} v${VER_MAJOR}.${VER_MINOR}"
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Sun Microsystems\Jxta"
Delete "$INSTDIR\uninst.exe"
RMDir "$INSTDIR"
Call Un.RMJxta_Directory
SectionEnd

Function MkJxta_Directory
CreateDirectory "$INSTDIR\core\include"
CreateDirectory "$INSTDIR\core\include\jpr"
CreateDirectory "$INSTDIR\core\include\arch"
CreateDirectory "$INSTDIR\core\include\arch\beos"
CreateDirectory "$INSTDIR\core\include\arch\netware"
CreateDirectory "$INSTDIR\core\include\arch\os2"
CreateDirectory "$INSTDIR\core\include\arch\unix"
CreateDirectory "$INSTDIR\core\include\arch\win32"
CreateDirectory "$INSTDIR\core\lib"
CreateDirectory "$INSTDIR\core\doc"
CreateDirectory "$INSTDIR\core\src"
CreateDirectory "$INSTDIR\core\src\apache"
CreateDirectory "$INSTDIR\core\src\jpr"
CreateDirectory "$INSTDIR\httpd-2.0\include"
CreateDirectory "$INSTDIR\httpd-2.0\lib"
CreateDirectory "$INSTDIR\BEEP\include"
CreateDirectory "$INSTDIR\BEEP\lib"
CreateDirectory "$INSTDIR\test"
CreateDirectory "$INSTDIR\win32"
CreateDirectory "$INSTDIR\win32\jxta"
CreateDirectory "$INSTDIR\win32\jxta\jxtalib"
CreateDirectory "$INSTDIR\win32\confgui"
CreateDirectory "$INSTDIR\win32\confgui\src"
FunctionEnd

Function Un.RMJxta_Directory
RMDir "$INSTDIR\core\include"
RMDir "$INSTDIR\core\include\jpr"
RMDir "$INSTDIR\core\include\arch"
RMDir "$INSTDIR\core\include\arch\beos"
RMDir "$INSTDIR\core\include\arch\netware"
RMDir "$INSTDIR\core\include\arch\os2"
RMDir "$INSTDIR\core\include\arch\unix"
RMDir "$INSTDIR\core\include\arch\win32"
RMDir "$INSTDIR\core\lib"
RMDir "$INSTDIR\core\doc"
RMDir "$INSTDIR\core\src"
RMDir "$INSTDIR\core\src\apache"
RMDir "$INSTDIR\core\src\jpr"
RMDir "$INSTDIR\httpd-2.0\include"
RMDir "$INSTDIR\httpd-2.0\lib"
RMDir "$INSTDIR\BEEP\include"
RMDir "$INSTDIR\BEEP\lib"
RMDir "$INSTDIR\test"
RMDir "$INSTDIR\win32"
RMDir "$INSTDIR\win32\jxta"
RMDir "$INSTDIR\win32\jxta\jxtalib"
RMDir "$INSTDIR\win32\confgui"
RMDir "$INSTDIR\win32\confgui\src"
FunctionEnd

Function InstallJxta_kernel
 Call Copy_Jxta_root_core
FunctionEnd

Function UnInstallJxta_kernel

FunctionEnd

Function Copy_Jxta_root_core
;
SetOutPath "$INSTDIR\core"
File "C:\jxta-c\AUTHORS"
File "C:\jxta-c\BUILD"
File "C:\jxta-c\ChangeLog"
File "C:\jxta-c\configure.in"
File "C:\jxta-c\COPYING"
File "C:\jxta-c\doxconf"
File "C:\jxta-c\INSTALL"
File "C:\jxta-c\jxta-config"
File "C:\jxta-c\libjxta.m4"
File "C:\jxta-c\license.txt"
File "C:\jxta-c\Makefile.am"
File "C:\jxta-c\NEWS"
File "C:\jxta-c\README"
FunctionEnd

Function Copy_Jxta_root_core_doc
;
SetOutPath "$INSTDIR\core\doc"
File "C:\jxta-c\www\arch-intro.xml"
File "C:\jxta-c\www\architecture.html"
File "C:\jxta-c\www\architecture.jpg"
File "C:\jxta-c\www\architecture.xml"
File "C:\jxta-c\www\build.html"
File "C:\jxta-c\www\CApiDoclet.java"
File "C:\jxta-c\www\cbindingmatrix.html"
File "C:\jxta-c\www\cnews.html"
File "C:\jxta-c\www\index.html"
File "C:\jxta-c\www\jxta.h"
File "C:\jxta-c\www\makespec"
File "C:\jxta-c\www\single-html.xsl"
File "C:\jxta-c\www\tasklist.html"
File "C:\jxta-c\www\testing.xml"
File "C:\jxta-c\www\TODO.html"
FunctionEnd

Function Copy_Jxta_root_core_include
;
SetOutPath "$INSTDIR\core\include"
File "C:\jxta-c\include\Advertisement.h"
File "C:\jxta-c\include\DiscoveryQuery.h"
File "C:\jxta-c\include\DiscoveryResponse.h"
File "C:\jxta-c\include\EndpointRouter.h"
File "C:\jxta-c\include\error.h"
File "C:\jxta-c\include\HTTPTransportAdvertisement.h"
File "C:\jxta-c\include\jdlist.h"
File "C:\jxta-c\include\jstring.h"
File "C:\jxta-c\include\jxta.h"
File "C:\jxta-c\include\jxtaapr.h"
File "C:\jxta-c\include\jxtaendpoint.h"
File "C:\jxta-c\include\jxtaid.h"
File "C:\jxta-c\include\jxtamsg.h"
File "C:\jxta-c\include\jxtaobject.h"
File "C:\jxta-c\include\jxtapg.h"
File "C:\jxta-c\include\jxtardvservice.h"
File "C:\jxta-c\include\jxta_errno.h"
File "C:\jxta-c\include\jxta_peer.h"
File "C:\jxta-c\include\jxta_peergroup.h"
File "C:\jxta-c\include\Jxta_resolver_service.h"
File "C:\jxta-c\include\jxta_types.h"
File "C:\jxta-c\include\PA.h"
File "C:\jxta-c\include\peerlist.h"
File "C:\jxta-c\include\PGA.h"
File "C:\jxta-c\include\ServiceAdvertisement.h"
FunctionEnd

Function Copy_Jxta_root_core_include_jpr
SetOutPath "$INSTDIR\core\include\jpr"
File "C:\jxta-c\include\jpr\jpr_errno.h"
File "C:\jxta-c\include\jpr\jpr_excep.h"
File "C:\jxta-c\include\jpr\jpr_threadonce.h"
File "C:\jxta-c\include\jpr\jpr_types.h"
File "C:\jxta-c\include\jpr\jpr_errno.h"
File "C:\jxta-c\include\jpr\jpr_excep.h"
File "C:\jxta-c\include\jpr\jpr_threadonce.h"
File "C:\jxta-c\include\jpr\jpr_types.h"
FunctionEnd

Function Copy_Jxta_root_core_include_jpr_arch
SetOutPath "$INSTDIR\core\include\jpr\arch\win32"
File "C:\jxta-c\include\jpr\arch\win32\jpr_threadonce.h"
FunctionEnd

Function Copy_Jxta_root_core_lib
SetOutPath "$INSTDIR\core\lib"
File "C:\jxta-c\lib\apr-2.0.28-freebsd-386.tar.gz"
File "C:\jxta-c\lib\apr-2.0-linux-386.tar.gz"
File "C:\jxta-c\lib\apr-2.0-solaris-sparc.tgz"
FunctionEnd

; eof - Jxta-c v1.0.0.0 DT: 2002.02.03 14:04
