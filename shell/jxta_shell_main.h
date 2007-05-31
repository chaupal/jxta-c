/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact Project JXTA at http://www.jxta.org.
 *
 * 5. Products derived from this software may not be called "JXTA",
 *    nor may "JXTA" appear in their name, without prior written
 *    permission of Sun.
 *
 * THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of Project JXTA.  For more
 * information on Project JXTA, please see
 * <http://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id: jxta_shell_main.h,v 1.1 2004/12/05 02:16:41 slowhog Exp $
 */
#ifndef __SHELL_MAIN_H__
#define  __SHELL_MAIN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jxta.h"
#include "jxta_peergroup.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef struct _shell_startup_application ShellStartupApplication;

/**
*  Starts up the parent shell application
*  @param pg the peer group in the context of which this
*        application runs
* @param stdout the Jxta_listener on which to send
*        outbound messages
* @param parent  the JxtaShellApplication that spawn this application
* @param terminate the function to call if we are terminating
*/
ShellStartupApplication * ShellStartupApplication_new(Jxta_PG * pg,
                                                      Jxta_listener* standout, 
                                      shell_application_terminate terminate);

/**
 * Process the data that are send to this application
 * @param app the object that 
 *        handles the data received on stdin
 * @param inputLine the data we received on stdin
 **/
void ShellStartupApplication_stdin(Jxta_object  *app, 
                                   JString * inputLine);

/**
 * Function that  starts the actual application
 * @param child the class (derived from JxtaShellApplication) that
 *        we want to start
 * @param argv the number of arguments to pass
 * @param arg the list of arguments
 **/
void   ShellStartupApplication_start(Jxta_object *parent,
                                     int argv, 
                                     char **arg);

/**
 * Function that   prints the help information
 * @param child the class (derived from JxtaShellApplication) that 
 *        we want to print the help information for
 **/
void   ShellStartupApplication_printHelp(Jxta_object *app);


/** 
* The functions that gets clled if the main application finished
 *  @param parent the JxtaShellApplication that needs to be informed
 *        about the termination
 *  @param child the class (derived from JxtaShellApplication) that
 *         terminates 
*/
void ShellStartupApplication_mainFinished(Jxta_object * parent,
                                          Jxta_object *child);

/** 
* The functions that gets clled if a spawned application finished
*  @param parent the JxtaShellApplication that needs to be informed
*        about the termination
*  @param child the class (derived from JxtaShellApplication) that
*         terminates 
*/
void ShellStartupApplication_spawnApplication_terminate(
                                          Jxta_object * parent,
                                          Jxta_object *child);

/**
* Instantiates the application with the indicated name
* @param parent The parent JxtaShellApplication of this  object - the one providing STDIN and STDOUT
* @param parentObject 
* @param app the name of the application to start
*/ 
JxtaShellApplication * ShellStartupApplication_loadApplication(
				 JxtaShellApplication * parent,
                                 Jxta_object * parentObject,
                                 char * app);

#ifdef __cplusplus
}
#endif


#endif /* __SHELL_MAIN_H__ */



