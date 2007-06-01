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
 * $Id: jxta_shell_getopt.h,v 1.3.4.1 2005/05/21 01:03:44 slowhog Exp $
 */
#ifndef __JXTA_SHELL_GETOPT_H__
#define  __JXTA_SHELL_GETOPT_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jstring.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef struct _jxta_shell_getopt JxtaShellGetopt;

enum JxtaShellGetOptErrors {
    OPT_NO_ERROR        = 0,
    NO_MORE_ARGS    =-1,
    MISSING_OPTION  =-2,
    MISSING_ARG     =-3,
    INVALID_OPTION  =-4,
    UNKNOWN_OPTION  =-5,
    MISC_ERROR      =-6
};

typedef enum JxtaShellGetOptErrors JxtaShellGetOptError;

/**
* Creates a new JxtaShellGetOpt object
* @param argc the number of command line arguments
* @param argv the command line arguments
* @param optionChars a list (of length argv) contaning 
*        the command line arguments
*/
JxtaShellGetopt *JxtaShellGetopt_new(int argc, const char **argv, const char *optionChars);

/**
* Retrieves  the next option. If there are no more options left or an error 
* occurred then -1 is returned the selected option is returned. Errors can be
* retrieved using ShellGetOpt_getError
*
* @param opt the JxtaShellGetOpt object to use 
* @return the value of the next option or -1 if an error occured
*         or there are no more options
*/
int JxtaShellGetopt_getNext(JxtaShellGetopt * opt);


/**
* If an error occured this function returns a value less that 0,
*  otherwise it returns 0
* @param opt the JxtaShellGetopt object to use 
* @return  a value smaller than 0  if an error ocurred, 0 otherwise
*/
JxtaShellGetOptError JxtaShellGetopt_getErrorCode(const JxtaShellGetopt * opt);

/**
* If an error occured this function returns a suitable error code
* @param opt the JxtaShellGetopt object to use 
* @return  a error description
*/
JString *JxtaShellGetopt_getError(const JxtaShellGetopt * opt);

JString *JxtaShellGetopt_OptionArgument(const JxtaShellGetopt * opt);

/** 
* If the previous option had an option argument,
* we return it, otherwise, NULL is returned
* @param opt the JxtaShellGetOpt object to use 
* @return the option argument of the previous option or 0
*/
JString *JxtaShellGetopt_getOptionArgument(const JxtaShellGetopt * opt);

/** 
* Returns the index of the current argument  or -1 if no more arguments
* are left
*  @param opt the JxtaShellGetOpt object to use 
*  @return  the index of the current argument  or -1 if no more arguments
*            are left
*/
int JxtaShellGetopt_getCurrentArgument(const JxtaShellGetopt * opt);

/** 
* Returns the next character in the current argument we would process
* in the next call to  JxtaShellGetOpt_getNext
*  @param opt the ShellGetOpt object to use 
*  @return  the index of the next characer in the current argument
*            are left
*/
int JxtaShellGetopt_getNextCharacter(const JxtaShellGetopt * opt);

/**
*  Frees the resources taken up by this object and frees the memory pointed 
* to by the opt pointer
*  @param opt the ShellGetOpt object to use 
*/
void JxtaShellGetopt_delete(JxtaShellGetopt * opt);

#ifdef __cplusplus
}
#endif


#endif /* __JXTA_SHELL_GETOPT_H__ */
