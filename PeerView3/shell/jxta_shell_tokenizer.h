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
 * $Id$
 */
#ifndef __JXTA_SHELL_TOKENIZER_H__
#define  __JXTA_SHELL_TOKENIZER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jxta.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
typedef struct _jxta_shell_tokenizer JxtaShellTokenizer;


/**
* Creates a new  Tokenizer
* @param line the line which we want to tokenize
* @param delim the characters on which we split the line
* @param returnDelim if 1 we do not return  delimiters, otherwise we do
*/
JxtaShellTokenizer *JxtaShellTokenizer_new(const char *line, const char *delim, int returnDelim);

/** 
*  Returns 0 if we have more tokens to return and -1 otherwise
*  @param opt the ShellTokenizer object to use 
* @return 1 if we have more tokens to return and 0 otherwise
*/
int JxtaShellTokenizer_hasMoreTokens(JxtaShellTokenizer * tok);

/** 
*  Returns the next token or 0 if no more tokens are available.
*  The token is a newly created malloced character array. The 
*  calling function is responsible for freeing it.
*  @param opt the ShellTokenizer object to use 
* @return  the next token or 0 if no more tokens are available
*/
char *JxtaShellTokenizer_nextToken(JxtaShellTokenizer * tok);

/**
* Returns the number of tokens still available in this tokenizer
*  @param opt the ShellTokenizer object to use 
* @return the number of tokens still available in this tokenizer
*/
int JxtaShellTokenizer_countTokens(JxtaShellTokenizer * tok);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_SHELL_TOKENIZER_H__ */

/* vim: set ts=4 sw=4 et tw=130 */
