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
 * $Id: jxta_shell_application.h,v 1.3 2005/11/17 04:21:53 slowhog Exp $
 */
#ifndef __JXTA_SHELL_APPLICATION_H__
#define  __JXTA_SHELL_APPLICATION_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta_peergroup.h"
#include "jxta_shell_environment.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
typedef struct _jxta_shell_application JxtaShellApplication;


/**
 * Prototype of the user function that handles the data received on 
 * the input stream of this application
 * @param child object that
 *        handles the data received on stdin
 * @param inputLine the data we received on stdin
 **/
typedef void (*shell_application_stdin) (Jxta_object * child, JString * inputLine);

/**
 * Prototype of the user function that  starts the actual application
 * @param child the object that we want to start
 * @param argv the number of arguments to pass
 * @param arg the list of arguments
 **/
typedef void (*shell_application_start) (Jxta_object * child, int argv, char **arg);

/**
 * Prototype of the user function that   prints the help information
 * @param child the  we want to print the help information for
 **/
typedef void (*shell_application_printHelp) (Jxta_object * child);

/**
 *  Prototype of the user function that  this application calls if it 
 *  terminates
 *  @param parent the object that needs to be informed
 *        about the termination
 *  @param child the object that terminates
 **/
typedef void (*shell_application_terminate) (Jxta_object * parent, Jxta_object * child);


 /**
* Creates a new ShellApplication object
* @param pg the peer group in the context of which this
*        application runs
* @param stdout the Jxta_listener on which to send
*        outbound messages
* @param env the  shell enviroment to use. If not NULL, we expect that it was
*            properly shared. If NULL a new empty environment is created
* @param parent  the JxtaShellApplication that spawn this application
* @param terminate the function to call if we are terminating
*/
JxtaShellApplication *JxtaShellApplication_new(Jxta_PG * pg,
                                               Jxta_listener * standout,
                                               JxtaShellEnvironment * env,
                                               Jxta_object * parent, shell_application_terminate terminate);


typedef JxtaShellApplication *(*shell_application_new) (Jxta_PG * pg,
                                                        Jxta_listener * standout,
                                                        JxtaShellEnvironment * env,
                                                        Jxta_object * parent, shell_application_terminate terminate);

/**
* Starts the application
* This class defers to its child
* @param app the application we want to start
* @param argv the number of arguments
* @param arg the argument array
*/
void JxtaShellApplication_start(JxtaShellApplication * app, int argv, char **arg);

/**
*  Prints  the help information for this child
* @param app the application we want to print help for
*/
void JxtaShellApplication_printHelp(JxtaShellApplication * app);


/** 
* Return the Jxta_listener object on which this application will
* listen, i.e.  where to send data to this application
* @param app the application from which to get the  pipe
*/
Jxta_listener *JxtaShellApplication_getSTDIN(JxtaShellApplication * app);

/**
* Returns the environment that used by this application
* @pram app the application from which to get the environment
*/
JxtaShellEnvironment *JxtaShellApplication_getEnv(JxtaShellApplication * app);

/**
*  Gets the peer group of this application
*  @param app the application from which to get the peergroup
*/
Jxta_PG *JxtaShellApplication_peergroup(JxtaShellApplication * app);


/**
* Terminates this application
* @param app  the application to terminate
*/
void JxtaShellApplication_terminate(JxtaShellApplication * app);

/** 
* Sets the function to use 
* @param a pointer that is passed back in the functions
* @param app the application for which to add  the data
* @param child the application for which to set the functions
* @param help the function that prints out the help menu
* @param help the function that starts the processu
* @param stdin the function that handles stdin
*/
void JxtaShellApplication_setFunctions(JxtaShellApplication * app,
                                       Jxta_object * child,
                                       shell_application_printHelp help,
                                       shell_application_start start, shell_application_stdin standin);


/**
* Prints information to the output of this application
* @param app the application for which to add  the data
* @param inputLine the line to print
* @return JXTA_SUCCESS if sucessfull, JXTA_INVALID_ARGUMENT otherwise
*/
Jxta_status JxtaShellApplication_print(JxtaShellApplication * app, JString * inputLine);

/**
* Prints information to the output of this application after 
* appending a newline character
* @param app the application for which to add  the data
* @param inputLine the line to print
* @return JXTA_SUCCESS if sucessfull, JXTA_INVALID_ARGUMENT otherwise
*/
Jxta_status JxtaShellApplication_println(JxtaShellApplication * app, JString * inputLine);


/**
* The function that process the data received 
* @param  obj the object that gets passed
* @param arg the user pointer
*/
void JXTA_STDCALL JxtaShellApplication_listenerFunction(Jxta_object * obj, void *arg);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_SHELL_APPLICATION_H__ */

/* vim: set ts=4 sw=4 et tw=130 */
