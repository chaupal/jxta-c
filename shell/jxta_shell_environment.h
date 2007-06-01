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
 * $Id: jxta_shell_environment.h,v 1.4 2005/08/24 01:21:20 slowhog Exp $
 */

#ifndef __JXTA_SHELL_ENVIRONMENT_H__
#define  __JXTA_SHELL_ENVIRONMENT_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_shell_object.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
typedef struct _jxta_shell_environement JxtaShellEnvironment;

/**
* Creates a new shell environement. This function initializes the object by 
* calling JXTA_OBJECT_INIT.
* @param parent the parent environment, if any. If not NULL it is expected that 
*               this object was
*               properly shared by the calling function.
*/
JxtaShellEnvironment *JxtaShellEnvironment_new(JxtaShellEnvironment * parent);


/************************************************************************
 * Inserts a new  JxtaShellObject.  The name of the JxtaShellObject is
 * used as the key. If no name exists, a name is generated.
 * 
 * @param env the environment to which to add the  object
 * @param obj the object to add. It is shared automatically
 */
void JxtaShellEnvironment_add_0(JxtaShellEnvironment * env, JxtaShellObject * obj);


/************************************************************************
 * Inserts a new  Jxta_object. A name is generated automtically
 * @param env the environment to which to add the  object
 * @param obj the object to add. It is shared automatically
 */
void JxtaShellEnvironment_add_1(JxtaShellEnvironment * env, Jxta_object * obj);



/************************************************************************
 * Inserts a new  Jxta_object. 
 * @param env the environment to which to add the  object
 * @param name the name under which to store the object. It is shared automatically
 * @param obj the object to add. It is shared automatically
 */
void JxtaShellEnvironment_add_2(JxtaShellEnvironment * env, JString * name, Jxta_object * obj);

/*
* Returns the JxtaShellObject stored under the indicated name
* @param env the environment  from which to get the name
* @param name the name of the variable to find
*/
JxtaShellObject *JxtaShellEnvironment_get(JxtaShellEnvironment * env, JString * name);

/*
* If an item under the name exists, it is deleted. It is not deleted, if 
* the parameter is in the  parent environment
* @param env the environment  from which to get the name
* @param name the name of the variable to delete
*/
void JxtaShellEnvironment_delete(JxtaShellEnvironment * env, JString * name);

/*
 *If an item with this type exists, it is deleted. It is not deleted, if 
 *the parameter is in the parent enviroment
 *@param env the environment from which to get the type
 *@param type the type of the variable to delete
 */
void JxtaShellEnvironment_delete_type(JxtaShellEnvironment * env, JString * type);

/**
* Does the indicated environment contain the indicted key
* @param env the environment  to query
* @param name the key  for which to search
*/
Jxta_boolean JxtaShellEnvironment_contains_key(JxtaShellEnvironment * env, JString * name);

/**
* Gets a list of all the values in this environment.
* The values and returned vector are properly shared. So the only
* responsibility of the invoker upon return is to release the vector
* when no-longer needed. 
* @param env the environment  to query
*/
Jxta_vector *JxtaShellEnvironment_contains_values(JxtaShellEnvironment * env);

/**
* Gets a list of the values in this environment of the specified type.
* The values and returned vector are properly shared. So the only
* responsibility of the invoker upon return is to release the vector
* when no-longer needed. 
* @param env the environment  to query
*/
Jxta_vector *JxtaShellEnvironment_get_of_type(JxtaShellEnvironment * env, JString * type);

/**
*   sets the current group variables to the provided group in the specified env.
*   @param  env the environment to modify.
*   @param  pg  the group which will become the current group.
*   @return the result.
*
**/
Jxta_status JxtaShellEnvironment_set_current_group(JxtaShellEnvironment * env, Jxta_PG * pg);


JString *JxtaShellEnvironment_createName(void);

#endif /* __JXTA_SHELL_ENVIRONMENT_H__ */
