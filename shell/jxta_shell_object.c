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
 * $Id: jxta_shell_object.c,v 1.3 2005/08/24 01:21:21 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta_objecthashtable.h"
#include "jxta.h"
#include "jxta_shell_object.h"

struct _jxta_shell_object {
    JXTA_OBJECT_HANDLE;
    JString *name;
    Jxta_object *object;
    JString *type;
};

void JxtaShellObject_delete(Jxta_object * obj);

JxtaShellObject *JxtaShellObject_new(JString * name, Jxta_object * object, JString * type)
{
    JxtaShellObject *obj = NULL;

    if (name == NULL || object == NULL)
        return obj;
    obj = (JxtaShellObject *) malloc(sizeof(JxtaShellObject));
    if (obj == NULL)
        return obj;
    memset(obj, 0, sizeof(*obj));
    JXTA_OBJECT_INIT(obj, JxtaShellObject_delete, 0);

    obj->name = JXTA_OBJECT_SHARE(name);
    obj->object = JXTA_OBJECT_SHARE(object);
    obj->type = JXTA_OBJECT_SHARE(type);

    return obj;
}

void JxtaShellObject_delete(Jxta_object * obj)
{
    JxtaShellObject *app = (JxtaShellObject *) obj;

    if (app != NULL) {
        if (app->name != NULL)
            JXTA_OBJECT_RELEASE(app->name);
        if (app->object != NULL)
            JXTA_OBJECT_RELEASE(app->object);
        if (app->type != NULL)
            JXTA_OBJECT_RELEASE(app->type);
        free(app);
    }
}

JString *JxtaShellObject_name(JxtaShellObject * obj)
{
    JString *result = NULL;

    if (obj != NULL && obj->name != NULL) {
        JXTA_OBJECT_SHARE(obj->name);
        result = obj->name;
    }
    return result;
}

Jxta_object *JxtaShellObject_object(JxtaShellObject * obj)
{
    Jxta_object *result = NULL;

    if (obj != NULL && obj->object != NULL) {
        JXTA_OBJECT_SHARE(obj->object);
        result = obj->object;
    }
    return result;
}


JString *JxtaShellObject_type(JxtaShellObject * obj)
{
    JString *result = NULL;

    if (obj != NULL && obj->type != NULL) {
        JXTA_OBJECT_SHARE(obj->type);
        result = obj->type;
    }
    return result;
}

Jxta_boolean JXTA_STDCALL JxtaShellObject_equals(Jxta_object * obj1, Jxta_object * obj2)
{
    JString *s1 = (JString *) obj1;
    JString *s2 = (JString *) obj2;
    Jxta_boolean result = FALSE;

    if (s1 != NULL && s2 != NULL) {
        if (strcmp(jstring_get_string(s1), jstring_get_string(s1)) == 0) {
            result = TRUE;
        }
    }
    return result;
}

unsigned int JXTA_STDCALL JxtaShellObject_hashValue(Jxta_object * obj)
{
    unsigned int result = 0;
    const char *string = jstring_get_string((JString *) obj);

    if (string != 0) {
        result = jxta_objecthashtable_simplehash(string, sizeof(char) * (strlen(string) + 1));
    }
    return result;
}
