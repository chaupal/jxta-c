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
 * $Id: jxta_shell_environment.c,v 1.7 2005/11/15 18:41:30 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_objecthashtable.h"
#include "jxta_shell_object.h"
#include "jxta_shell_environment.h"

static int _jxta_shell_environement_counter = 0;

struct _jxta_shell_environement {
    JXTA_OBJECT_HANDLE;
    Jxta_objecthashtable *list;
};

void JxtaShellEnvironment_free(Jxta_object * obj);

/**
*  Create a new name for the environment variable
*/
JString *JxtaShellEnvironment_createName();

JxtaShellEnvironment *JxtaShellEnvironment_new(JxtaShellEnvironment * parent)
{
    JxtaShellEnvironment *env = 0;

    env = (JxtaShellEnvironment *) malloc(sizeof(JxtaShellEnvironment));

    if (env == NULL)
        return NULL;

    memset(env, 0, sizeof(JxtaShellEnvironment));
    JXTA_OBJECT_INIT(env, JxtaShellEnvironment_free, 0);

    env->list = jxta_objecthashtable_new(0, JxtaShellObject_hashValue, JxtaShellObject_equals);

    if (parent != NULL) {   /** add the parent data */
        Jxta_vector *list = JxtaShellEnvironment_contains_values(parent);
        JxtaShellObject *obj;

        if (list != NULL) {
            int i, max = jxta_vector_size(list);

            for (i = 0; i < max; i++) {
                obj = 0;
                jxta_vector_get_object_at(list, JXTA_OBJECT_PPTR(&obj), i);
                if (obj != 0) {
                    JxtaShellEnvironment_add_0(env, obj);
                    JXTA_OBJECT_RELEASE(obj);
                }
            }
            JXTA_OBJECT_RELEASE(list);
            list = NULL;
        }
    }
    return env;
}

void JxtaShellEnvironment_free(Jxta_object * obj)
{
    JxtaShellEnvironment *app = (JxtaShellEnvironment *) obj;

    if (app != NULL) {
        if (app->list != NULL)
            JXTA_OBJECT_RELEASE(app->list);
        free(app);
    }
}

JString *JxtaShellEnvironment_createName()
{
    char result[100];
    JString *name;

    result[0] = '\0';
    sprintf(result, "Env-%d", _jxta_shell_environement_counter++);
    name = jstring_new_2(result);
    return name;
}

void JxtaShellEnvironment_add_0(JxtaShellEnvironment * env, JxtaShellObject * obj)
{
    if (env != 0 && env->list != 0) {
        JString *name = JxtaShellObject_name(obj);
        jxta_objecthashtable_del(env->list, name, NULL);
        if (name == 0)
            name = JxtaShellEnvironment_createName();
        jxta_objecthashtable_put(env->list, (Jxta_object *) name, (Jxta_object *) obj);
        JXTA_OBJECT_RELEASE(name);
    }
}

void JxtaShellEnvironment_add_1(JxtaShellEnvironment * env, Jxta_object * obj)
{
    JxtaShellEnvironment_add_2(env, 0, obj);
}


void JxtaShellEnvironment_add_2(JxtaShellEnvironment * env, JString * name, Jxta_object * obj)
{
    if (env != 0 && env->list != 0) {
        JString *key = name;
        JxtaShellObject *value;

        if (key == NULL) {
            key = JxtaShellEnvironment_createName();    /* gives us a share */
            if (key == 0)
                return;
        } else {
            JXTA_OBJECT_SHARE(key); /* we need a share */
        }

        value = JxtaShellObject_new(key, obj, 0);   /* does not share for itself */
        if (value == 0) {
            JXTA_OBJECT_RELEASE(key);
            return;
        }
        JXTA_OBJECT_SHARE(obj); /* needs a share too */

        JxtaShellEnvironment_add_0(env, value);
        JXTA_OBJECT_RELEASE(value);
    }
}


JxtaShellObject *JxtaShellEnvironment_get(JxtaShellEnvironment * env, JString * name)
{
    JxtaShellObject *found = 0;

    if (env == 0 || env->list == 0 || name == 0)
        return found;

    jxta_objecthashtable_get(env->list, (Jxta_object *) name, JXTA_OBJECT_PPTR(&found));
    return found;
}

void JxtaShellEnvironment_delete(JxtaShellEnvironment * env, JString * name)
{
    if (env == 0 || env->list == 0 || name == 0)
        return;

    jxta_objecthashtable_del(env->list, (Jxta_object *) name, NULL);
}

Jxta_boolean JxtaShellEnvironment_contains_key(JxtaShellEnvironment * env, JString * name)
{
    Jxta_boolean result = FALSE;

    if (env != 0 && env->list != 0 && name != 0) {
        JxtaShellObject *found;

        found = JxtaShellEnvironment_get(env, name);
        if (found != 0) {
            result = TRUE;
            JXTA_OBJECT_RELEASE(found);
        }
    }

    return result;
}

Jxta_vector *JxtaShellEnvironment_contains_values(JxtaShellEnvironment * env)
{
    Jxta_vector *list = 0;

    if (env != 0 && env->list != 0) {
        list = jxta_objecthashtable_values_get(env->list);
    }
    return list;
}

void JxtaShellEnvironment_delete_type(JxtaShellEnvironment * env, JString * type)
{
    if (env != NULL && env->list != NULL) {
        int eachItem;
        Jxta_vector *list = NULL;

        list = jxta_objecthashtable_values_get(env->list);
        for (eachItem = 0; eachItem < jxta_vector_size(list); eachItem++) {
            JxtaShellObject *object = NULL;

            jxta_vector_get_object_at(list, JXTA_OBJECT_PPTR(&object), eachItem);

            if ((NULL != object)) {
                JString *itsType = JxtaShellObject_type(object);

                if (0 == strcmp(jstring_get_string(type), jstring_get_string(itsType))) {
                    JString *itsName = JxtaShellObject_name(object);
                    char *name_cstr = jstring_get_string(itsName);
                    if (strcmp("currGroupAdv", name_cstr) != 0 && strcmp("thisPeer", name_cstr) != 0) {
                        JxtaShellEnvironment_delete(env, itsName);
                        JXTA_OBJECT_RELEASE(itsName);
                    }
                }
                JXTA_OBJECT_RELEASE(itsType);
                itsType = NULL;
            }
            JXTA_OBJECT_RELEASE(object);
            object = NULL;
        }
        JXTA_OBJECT_RELEASE(list);
        list = NULL;
    }

}

Jxta_vector *JxtaShellEnvironment_get_of_type(JxtaShellEnvironment * env, JString * type)
{
    Jxta_vector *reslist = jxta_vector_new(0);

    if (env != NULL && env->list != NULL) {
        int eachItem;
        Jxta_vector *list = NULL;

        list = jxta_objecthashtable_values_get(env->list);
        for (eachItem = 0; eachItem < jxta_vector_size(list); eachItem++) {
            JxtaShellObject *object = NULL;

            jxta_vector_get_object_at(list, JXTA_OBJECT_PPTR(&object), eachItem);

            if ((NULL != object)) {
                JString *itsType = JxtaShellObject_type(object);

                if (0 == strcmp(jstring_get_string(type), jstring_get_string(itsType)))
                    jxta_vector_add_object_last(reslist, (Jxta_object *) object);

                JXTA_OBJECT_RELEASE(itsType);
                itsType = NULL;
            }
            JXTA_OBJECT_RELEASE(object);
            object = NULL;
        }
        JXTA_OBJECT_RELEASE(list);
        list = NULL;
    }

    return reslist;
}

Jxta_status JxtaShellEnvironment_set_current_group(JxtaShellEnvironment * env, Jxta_PG * pg)
{
    Jxta_PGA *pga = NULL;
    JString *name = NULL;
    JString *type = NULL;
    JxtaShellObject *var = NULL;

    jxta_PG_get_PGA(pg, &pga);
    name = jstring_new_2("currGroupAdv");
    type = jstring_new_2("GroupAdvertisement");
    var = JxtaShellObject_new(name, (Jxta_object *) pga, type);
    JXTA_OBJECT_RELEASE(name);
    JXTA_OBJECT_RELEASE(type);

    JxtaShellEnvironment_add_0(env, var);
    JXTA_OBJECT_RELEASE(var);   /* release local */
    JXTA_OBJECT_RELEASE(pga);
    pga = NULL; /* release local */

    name = jstring_new_2("currGroup");
    type = jstring_new_2("Group");
    var = JxtaShellObject_new(name, (Jxta_object *) pg, type);
    JXTA_OBJECT_RELEASE(name);
    JXTA_OBJECT_RELEASE(type);

    JxtaShellEnvironment_add_0(env, var);
    JXTA_OBJECT_RELEASE(var);   /* release local */

    return JXTA_SUCCESS;
}

/* vi: set ts=4 sw=4 tw=130 et: */
