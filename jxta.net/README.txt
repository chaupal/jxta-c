JXTA.NET
========

The aim of JXTA.NET is to integrate JXTA in the .NET framework. To simplify this integration, the project did not reimplement the whole JXTA protocols. The integration based on the JXTA-C project, which implements the JXTA protocols in the programming language C. Therefore, the most parts of the JXTA.NET are simple C# wrappers to use JXTA-C in the .NET framework. Therefore, JXTA.NET is integrated in JXTA-C project. The sources are located in the subfolder jxta.net.

General
=======

It is possible to use JXTA.NET under Windows and Mono. JXTA.NET based on the .NET framework 2.0. Therefore, Visual Studio .NET 2005 is necessary to build JXTA.NET on a Windows system. Under Mono gmcs is necessary which is Mono's C# compiler for the .NET framework 2.0. 

To create JXTA.NET you need following libraries:
 - APR (http://apr.apache.org/)
 - OpenSSL (http://www.openssl.org/)
 - zlib (http://www.zlib.net/)
 - sqlite3 (http://www.sqlite.org/)
 - xml2 (http://xmlsoft.org/index.html)

Beside XML2 you can use the official distribution of these libraries. The XML2 library has been modified in order to provide wild card searching of text elements with the Enhanced Discovery Service. Therefore, you need to create this custom version of XML2. See INSTALL in lib/xml2 for instructions to build this version of XML2.


Check out the sources from CVS
==============================

cvs -d :pserver:guest@cvs.jxta.org:/cvs login
cvs -d :pserver:guest@cvs.jxta.org:/cvs checkout jxta-c 


Building JXTA.NET on Windows
============================

You have to specify the environment variables (APACHE2, XML2, ZLIB, SQLITE3 and OPENSSL) to the location of the necessary libraries. It is assumed that the headers are located in the subdirectory "include", the DLLs in the subdirtectory "bin" and the link libraries in the subdirectory "lib". For instance, zlib.h is located at $(ZLIB)/include.

Open with Visual Studio .NET 2005 the project file JxtaNET.sln in the directory "jxta.net". Afterwards, press F7 to build the JXTA-C, JXTA.NET and the JxtaNETShell.


Building JXTA.NET on Mono
========================= 

1. If you want to build a debug version of JXTA.NET, set the following environment variables.
       export MONO_FLAGS=--debug
       export GMCS_FLAGS=-debug
2. JXTA.NET is integrated in the JXTA-C project. Therefore, you have to configure JXTA-C with the option "--enable-mono" to build JXTA-C and JXTA.NET. 
3. Create and install the libraries and examples with the following commands:
      make
      make install
4. Modify your environment variable LD_LIBRARY_PATH in such a way that Mono finds the necessary libraries like APR and JXTA-C.
5. Start the demo application JxtaNETShell.


Copyright notice
================

The Sun Project JXTA(TM) Software License
  
Copyright (c) 2005 Sun Microsystems, Inc. All rights reserved.
  
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
