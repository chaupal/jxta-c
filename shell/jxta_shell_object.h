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
 * $Id: jxta_shell_object.h,v 1.3 2005/08/24 01:21:21 slowhog Exp $
 */

#ifndef __JXTA_SHELL_OBJECT_H__
#define  __JXTA_SHELL_OBJECT_H__

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
typedef struct _jxta_shell_object JxtaShellObject;

/**
* Creates a new shell object. This function initializes the object by 
* calling JXTA_OBJECT_INIT.
* @param name the name of the shell object to create, which cannot be NULL. If NULL, 
*             we return a NULL pointer. It is expected that this object was
*             properly shared by the calling function.
* @param object the object to store under the given name. It cannot be NULL. If NULL, 
*             we return a NULL pointer. It is expected that this object was
*             properly shared by the calling function.
* @param type an optional description of the type of object stored. It is just saved 
*              and passed on as is. If not NULL it is expected that this object was
*             properly shared by the calling function.
*/
JxtaShellObject *JxtaShellObject_new(JString * name, Jxta_object * object, JString * type);

/**
*  Gets the name of this JxtaShellObject. If the calling function is done with
*  the JString object it should release it.
*  @param obj the JxtaShellObject whose name to extract
*/
JString *JxtaShellObject_name(JxtaShellObject * obj);

/**
*  Gets the object encapsulated by this JxtaShellObject. If the calling function is done with
*  the object it should release it.
*  @param obj the JxtaShellObject whose name to extract
*/
Jxta_object *JxtaShellObject_object(JxtaShellObject * obj);

/**
*  Gets the type associated with  this JxtaShellObject. If the calling function is done with
*  the JString and it is not NULL, it should release it.
*  @param obj the JxtaShellObject whose name to extract
*/
JString *JxtaShellObject_type(JxtaShellObject * obj);

/**
* Compares two JxtaShellObject types.
* They are equal it the names are equal
*/
Jxta_boolean JXTA_STDCALL JxtaShellObject_equals(Jxta_object * obj1, Jxta_object * obj2);

/**
* Calculates the hash value of the object - it just calls the hash function
* in jxta_objecthashtable.h with the correct size
* @param obj the object for which to determine the hash value
*/
unsigned int JXTA_STDCALL JxtaShellObject_hashValue(Jxta_object * obj);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_SHELL_OBJECT_H__ */

/* vim: set ts=4 sw=4 et tw=130 */
