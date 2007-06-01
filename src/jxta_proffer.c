/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
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
  */
#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxta_advertisement.h"
#include "jxta_proffer.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */ typedef struct _elem_attr Jxta_elemAttr;
static Jxta_status prof_get_element_attribute(const char *elementAttribute, Jxta_elemAttr ** out);
static void prof_add_to_elemHash(Jxta_ProfferAdvertisement * prof, Jxta_elemAttr * el);

struct _jxta_ProfferAdvertisement {
    Jxta_advertisement jxta_advertisement;
    int TTL;
    JString *nameSpace;
    Jxta_hashtable *elemHash;
    const char *advId;
    const char *peerId;
};
void freeProfferAdv(Jxta_object * obj)
{
    Jxta_ProfferAdvertisement *ad = (Jxta_ProfferAdvertisement *) obj;
    if (ad->nameSpace != NULL) {
        JXTA_OBJECT_RELEASE(ad->nameSpace);
    }
    if (ad->elemHash != NULL) {
        JXTA_OBJECT_RELEASE(ad->elemHash);
    }
    free(ad);
}
struct _elem_attr {
    JXTA_OBJECT_HANDLE;
    char *element;
    char *attribute;
    char *value;
};

void freeElemAttr(Jxta_object * obj)
{
    Jxta_elemAttr *el = (Jxta_elemAttr *) obj;

    if (el->element != NULL) {
        free(el->element);
    }
    if (el->value != NULL) {
        free(el->value);
    }
    free(el);
}
static const char *__log_cat = "PROF";


JXTA_DECLARE(Jxta_ProfferAdvertisement *)
    jxta_ProfferAdvertisement_new()
{
    Jxta_ProfferAdvertisement *self;

    self = (Jxta_ProfferAdvertisement *) calloc(1, sizeof(Jxta_ProfferAdvertisement));
    if (self == NULL)
        return NULL;
    JXTA_OBJECT_INIT(self, (JXTA_OBJECT_FREE_FUNC) freeProfferAdv, 0);
    self->elemHash = jxta_hashtable_new(3);

    return self;
}

JXTA_DECLARE(Jxta_status)
    jxta_proffer_adv_add_item(Jxta_ProfferAdvertisement * prof, const char *name, const char *value)
{
    Jxta_elemAttr *elemAttr;
    Jxta_status status;
    status = prof_get_element_attribute(name, &elemAttr);
    if (status != JXTA_SUCCESS) {
        return status;
    }
    elemAttr->value = strdup(value);
    prof_add_to_elemHash(prof, elemAttr);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_set_nameSpace(Jxta_ProfferAdvertisement * prof, const char *ns)
{
    if (prof->nameSpace != NULL) {
        JXTA_OBJECT_RELEASE(prof->nameSpace);
    }
    prof->nameSpace = jstring_new_2(ns);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(JString *) jxta_proffer_adv_get_nameSpace(Jxta_ProfferAdvertisement * prof)
{
    return jstring_clone(prof->nameSpace);
}

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_set_advId(Jxta_ProfferAdvertisement * prof, const char *id)
{
    prof->advId = id;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(const char *) jxta_proffer_adv_get_advId(Jxta_ProfferAdvertisement * prof)
{
    return prof->advId;
}

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_set_peerId(Jxta_ProfferAdvertisement * prof, const char *id)
{
    prof->peerId = id;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(const char *) jxta_proffer_adv_get_peerId(Jxta_ProfferAdvertisement * prof)
{
    return prof->peerId;
}

JXTA_DECLARE(Jxta_status)
    jxta_proffer_adv_print(Jxta_ProfferAdvertisement * prof, JString * out)
{
    unsigned int i, j;
    char *nspace, *ns;
    Jxta_vector *elemVector = NULL;
    Jxta_vector *elements = NULL;
    jstring_reset(out, NULL);
    jstring_append_2(out, "<");
    jstring_append_2(out, jstring_get_string(prof->nameSpace));
    jstring_append_2(out, " xmlns:");
    nspace = strdup(jstring_get_string(prof->nameSpace));
    ns = nspace;
    while (*ns) {
        if (*ns == ':') {
            *ns = '\0';
            break;
        }
        ns++;
    }
    jstring_append_2(out, nspace);
    free(nspace);
    jstring_append_2(out, "=\"http://jxta.org\"");
    jstring_append_2(out, ">\n");
    elements = jxta_hashtable_values_get(prof->elemHash);
    for (j = 0; j < jxta_vector_size(elements); j++) {
        Jxta_elemAttr *elemAttr = NULL;
        JString *attrString = NULL;
        char *element = NULL;
        char *elemValue = NULL;
        jxta_vector_get_object_at(elements, (Jxta_object **) & elemVector, j);
        for (i = 0; i < jxta_vector_size(elemVector); i++) {
            jxta_vector_get_object_at(elemVector, (Jxta_object **) & elemAttr, i);
            element = elemAttr->element;
            if (elemAttr->attribute == NULL) {
                elemValue = elemAttr->value;
            } else {
                if (attrString == NULL) {
                    attrString = jstring_new_0();
                }
                jstring_append_2(attrString, elemAttr->attribute);
                jstring_append_2(attrString, "=\"");
                jstring_append_2(attrString, elemAttr->value);
                jstring_append_2(attrString, "\" ");
            }
        }
        jstring_append_2(out, "<");
        jstring_append_2(out, element);
        if (attrString != NULL) {
            jstring_append_2(out, " ");
            jstring_append_1(out, attrString);
            JXTA_OBJECT_RELEASE(attrString);
        }
        jstring_append_2(out, ">");
        if (elemValue != NULL) {
            jstring_append_2(out, elemValue);
        }
        jstring_append_2(out, "</");
        jstring_append_2(out, element);
        jstring_append_2(out, ">\n");
    }
    jstring_append_2(out, "</");
    jstring_append_2(out, jstring_get_string(prof->nameSpace));
    jstring_append_2(out, ">\n");
    return JXTA_SUCCESS;
}
static Jxta_status prof_get_element_attribute(const char *elementAttribute, Jxta_elemAttr ** out)
{
    Jxta_boolean att = FALSE;
    Jxta_elemAttr *ret = (Jxta_elemAttr *) calloc(1, sizeof(Jxta_elemAttr));
    char *dest = (char *) calloc(1, strlen(elementAttribute) + 1);
    JXTA_OBJECT_INIT(ret, (JXTA_OBJECT_FREE_FUNC) freeElemAttr, 0);
    ret->element = dest;
    ret->attribute = NULL;

    while (*elementAttribute) {
        char c = *elementAttribute;
        if (c == ' ') {
            /* make the element a string */
            *dest = '\0';
            att = TRUE;
            elementAttribute++;
            /* don't go past the end */
            if (*elementAttribute) {
                dest++;
                ret->attribute = dest;
            }
            continue;
        }
        *dest++ = c;
        elementAttribute++;
    }
    *out = ret;
    return JXTA_SUCCESS;
}
static void prof_add_to_elemHash(Jxta_ProfferAdvertisement * prof, Jxta_elemAttr * el)
{
    Jxta_vector *elemVector;
    if (jxta_hashtable_get(prof->elemHash, el->element, strlen(el->element), (Jxta_object **) & elemVector) != JXTA_SUCCESS) {
        elemVector = jxta_vector_new(1);
        jxta_hashtable_put(prof->elemHash, el->element, strlen(el->element), (Jxta_object *) elemVector);
    }
    jxta_vector_add_object_first(elemVector, (Jxta_object *) el);
    JXTA_OBJECT_RELEASE(elemVector);
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
