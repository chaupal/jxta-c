
                             Windows port
                             ============

This directory contains the files required to build this software on the 
native Windows platform. 

General
=======

Currently, we support only Visual Studio .NET 2003 to build JXTA-C. Therefore, 
the old VC6 project files in the current CVS tree are obsolete.

To create JXTA-C on Windows you need following libraries:

 - APR (http://apr.apache.org/)
 - OpenSSL (http://www.openssl.org/)
 - zlib (http://www.zlib.net/)
 - sqlite3 (http://www.sqlite.org/)
 - xml2 (http://xmlsoft.org/index.html)

Beside XML2 you can use the official distribution of these libraries. The 
XML2 library has been modified in order to provide wild card searching 
of text elements with the Enhanced Discovery Service. Therefore, you need to 
create this custom version of XML2. See INSTALL in lib/xml2 for instructions on 
building this version of XML2.


Check out the sources from CVS
==============================

cvs -d :pserver:guest@cvs.jxta.org:/cvs login
cvs -d :pserver:guest@cvs.jxta.org:/cvs checkout jxta-c 


Configuring the sources
=======================

You have to specify the environment variables (APACHE2, XML2, ZLIB, SQLITE3 and OPENSSL) 
to the location of the necessary libraries. It is assumed that 
the headers are located in the subdirectory "include", the DLLs in the subdirtectory 
"bin" and the link libraries in the subdirectory "lib". For instance, zlib.h is located 
at $(ZLIB)/include.


Building the sources
====================

Open with Visual Studio .NET 2003 the project file jxta.sln in the directory "win32".
Afterwards, press F7 to build the JXTA libraries and all examples. 


Copyright notice
================

The Sun Project JXTA(TM) Software License
  
Copyright (c) 2001-2004 Sun Microsystems, Inc. All rights reserved.
  
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
  
  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  
  2. Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
  
  3. The end-user documentation included with the redistribution, if any, must 
     include the following acknowledgment: "This product includes software 
     developed by Sun Microsystems, Inc. for JXTA(TM) technology." 
     Alternately, this acknowledgment may appear in the software itself, if 
     and wherever such third-party acknowledgments normally appear.
  
  4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must 
     not be used to endorse or promote products derived from this software 
     without prior written permission. For written permission, please contact 
     Project JXTA at http://www.jxta.org.
  
  5. Products derived from this software may not be called "JXTA", nor may 
     "JXTA" appear in their name, without prior written permission of Sun.
  
THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SUN 
MICROSYSTEMS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  
JXTA is a registered trademark of Sun Microsystems, Inc. in the United 
States and other countries.
  
Please see the license information page at :
<http://www.jxta.org/project/www/license.html> for instructions on use of 
the license in source files.
  
====================================================================

This software consists of voluntary contributions made by many individuals 
on behalf of Project JXTA. For more information on Project JXTA, please see 
http://www.jxta.org.
  
This license is based on the BSD license adopted by the Apache Foundation. 