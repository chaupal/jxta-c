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
 * $Id: jxta_proffer.c,v 1.17 2006/09/19 18:45:47 exocetrick Exp $
 */

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxta_advertisement.h"
#include "jxta_proffer.h"
#include "jxta_apr.h"

/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
typedef struct _element_entry Jxta_elemEntry;
typedef struct _elem_attr Jxta_elemAttr;

static Jxta_status prof_get_element_attribute(const char *elementAttribute, Jxta_elemAttr ** out);
static void prof_add_to_elemHash(Jxta_ProfferAdvertisement * prof, Jxta_elemAttr * el);
static Jxta_boolean prof_is_indexable(Jxta_ProfferAdvertisement * prof, const char *element_attr);
static const char *__log_cat = "PROF";

struct _jxta_ProfferAdvertisement {
    Jxta_advertisement jxta_advertisement;
    int TTL;
    Jxta_vector *indexables;
    JString *nameSpace;
    Jxta_hashtable *elemHash;
    Jxta_vector *elemVector;
    const char *advId;
    const char *peerId;
};

struct _elem_attr {
    JXTA_OBJECT_HANDLE;
    char *element;
    char *attribute;
    char *value;
};

struct _element_entry {
    JXTA_OBJECT_HANDLE;
    Jxta_elemAttr *element;
    Jxta_boolean isIndexable;
    Jxta_vector *attributes;
};

void freeElemEntry(Jxta_object * obj)
{
    Jxta_elemEntry *entry = NULL;
    entry = (Jxta_elemEntry *) obj;
    if (entry->element) {
        JXTA_OBJECT_RELEASE(entry->element);
    }
    if (entry->attributes) {
        JXTA_OBJECT_RELEASE(entry->attributes);
    }
    free(entry);
}

void freeProfferAdv(Jxta_object * obj)
{
    Jxta_ProfferAdvertisement *ad = (Jxta_ProfferAdvertisement *) obj;
    jxta_advertisement_destruct((Jxta_advertisement *) ad);
    if (ad->nameSpace != NULL) {
        JXTA_OBJECT_RELEASE(ad->nameSpace);
    }
    if (ad->elemHash != NULL) {
        JXTA_OBJECT_RELEASE(ad->elemHash);
    }
    if (ad->elemVector != NULL) {
        JXTA_OBJECT_RELEASE(ad->elemVector);
    }
    if (ad->indexables != NULL) {
        JXTA_OBJECT_RELEASE(ad->indexables);
    }
    free(ad);
}

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

static void handleElement(void *userdata, const XML_Char * cd, int len)
{
    const char **atts = NULL;
    JString *pass = NULL;
    JString *passe = NULL;
    JString *passAttr = NULL;
    Jxta_elemAttr *elemAttr = NULL;
    Jxta_elemEntry *elemEntry = NULL;
    Jxta_status status;
    Jxta_vector *attrVector = NULL;
    Jxta_ProfferAdvertisement *ad = (Jxta_ProfferAdvertisement *) userdata;
    Jxta_advertisement *adv = (Jxta_advertisement *) userdata;
    atts = adv->atts;
    if (0 == len)
        return;
    pass = jstring_new_0();
    jstring_append_0(pass, cd, len);
    jstring_trim(pass);
    while (NULL != atts && NULL != *atts) {
        passAttr = jstring_new_2(adv->currElement);
        jstring_append_2(passAttr, " ");
        jstring_append_2(passAttr, *atts);
        passe = jstring_new_2(*(atts + 1));
        jstring_trim(passe);
        if (jstring_length(passe) > 0) {
            if (prof_is_indexable(ad, jstring_get_string(passAttr))) {
                Jxta_elemAttr *elemAttr = NULL;

                if (NULL == attrVector) {
                    attrVector = jxta_vector_new(0);
                }
                status = prof_get_element_attribute(jstring_get_string(passAttr), &elemAttr);
                elemAttr->value = strdup(jstring_get_string(passe));
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,
                                "handleElement -- element:%s  attribute:%s value:%s\n", elemAttr->element, elemAttr->attribute,
                                elemAttr->value);
                jxta_vector_add_object_last(attrVector, (Jxta_object *) elemAttr);
                JXTA_OBJECT_RELEASE(elemAttr);
            }
        }
        atts += 2;
        JXTA_OBJECT_RELEASE(passe);
        JXTA_OBJECT_RELEASE(passAttr);
    }
    adv->atts = NULL;
    if (adv->currElement != NULL) {
        status = prof_get_element_attribute(adv->currElement, &elemAttr);
        if (status != JXTA_SUCCESS) {
            adv->currElement = NULL;
            goto CommonExit;
        }
    } else {
        goto CommonExit;
    }
    elemAttr->value = strdup(jstring_get_string(pass));
    elemEntry = calloc(1, sizeof(Jxta_elemEntry));
    JXTA_OBJECT_INIT(elemEntry, (JXTA_OBJECT_FREE_FUNC) freeElemEntry, 0);
    elemEntry->element = JXTA_OBJECT_SHARE(elemAttr);
    elemEntry->attributes = JXTA_OBJECT_SHARE(attrVector);

    if (NULL == ad->elemVector) {
        ad->elemVector = jxta_vector_new(0);
    }
    elemEntry->isIndexable = FALSE;
    if (prof_is_indexable(ad, adv->currElement) || attrVector != NULL) {
        elemEntry->isIndexable = TRUE;
        jxta_vector_add_object_last(ad->elemVector, (Jxta_object *) elemEntry);
    }

    adv->currElement = NULL;
  CommonExit:
    if (attrVector)
        JXTA_OBJECT_RELEASE(attrVector);
    if (elemAttr)
        JXTA_OBJECT_RELEASE(elemAttr);
    if (elemEntry)
        JXTA_OBJECT_RELEASE(elemEntry);
    if (pass)
        JXTA_OBJECT_RELEASE(pass);
}

JXTA_DECLARE(Jxta_ProfferAdvertisement *) jxta_ProfferAdvertisement_new()
{
    Jxta_ProfferAdvertisement *me;

    me = (Jxta_ProfferAdvertisement *) calloc(1, sizeof(Jxta_ProfferAdvertisement));
    if (me == NULL)
        return NULL;
    JXTA_OBJECT_INIT(me, (JXTA_OBJECT_FREE_FUNC) freeProfferAdv, 0);
    return me;
}

JXTA_DECLARE(Jxta_ProfferAdvertisement *) jxta_ProfferAdvertisement_new_1(const char *ns, const char * advid, const char *peerid)
{
    Jxta_ProfferAdvertisement *me;

    me = jxta_ProfferAdvertisement_new();
    if (NULL == me) return NULL;
    jxta_proffer_adv_set_nameSpace(me, ns);
    jxta_proffer_adv_set_advId(me, advid);
    jxta_proffer_adv_set_peerId(me, peerid);
    return me;
}

static const Kwdtab ProfferAdvertisement_tags[] = {
    {"*", 0, *handleElement, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_add_item(Jxta_ProfferAdvertisement * prof, const char *name, const char *value)
{
    Jxta_elemAttr *elemAttr;
    Jxta_status status;
    status = prof_get_element_attribute(name, &elemAttr);
    if (status != JXTA_SUCCESS) {
        return status;
    }
    elemAttr->value = strdup(value);
    prof_add_to_elemHash(prof, elemAttr);

    JXTA_OBJECT_RELEASE(elemAttr);
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
    if (prof->nameSpace) return jstring_clone(prof->nameSpace);
    return NULL;
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

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_print(Jxta_ProfferAdvertisement * prof, JString * out)
{
    unsigned int i, j;
    char *nspace, *ns;
    Jxta_boolean isMultiple = FALSE;
    Jxta_vector *elemv = NULL;
    Jxta_vector *elements = NULL;
    Jxta_elemEntry *elemEntry = NULL;
    jstring_reset(out, NULL);
    jstring_append_2(out, "<");
    jstring_append_1(out, prof->nameSpace);
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
    jstring_append_2(out, " xmlns:range=\"http://jxta.org\"");
    jstring_append_2(out, ">\n");
    if (NULL != prof->elemHash) {
        elements = jxta_hashtable_values_get(prof->elemHash);
    } else if (NULL != prof->elemVector) {
        isMultiple = TRUE;
        elements = JXTA_OBJECT_SHARE(prof->elemVector);
    } else {
        jstring_append_2(out, "</");
        jstring_append_1(out, prof->nameSpace);
        jstring_append_2(out, ">");
        return JXTA_SUCCESS;
    }
    for (j = 0; NULL != elements && j < jxta_vector_size(elements); j++) {
        Jxta_elemAttr *elemAttr = NULL;
        JString *attrString = NULL;
        char *element = NULL;
        char *elemValue = NULL;
        if (isMultiple) {
            Jxta_elemAttr *elemAttr;
            jxta_vector_get_object_at(elements, JXTA_OBJECT_PPTR(&elemEntry), j);
            elemv = elemEntry->attributes;
            if (elemv)
                JXTA_OBJECT_SHARE(elemv);
            elemAttr = elemEntry->element;
            element = elemAttr->element;
            elemValue = elemAttr->value;
            if ('#' == *elemValue)
                elemValue = elemValue + 1;
        } else {
            jxta_vector_get_object_at(elements, JXTA_OBJECT_PPTR(&elemv), j);
        }
        for (i = 0; NULL != elemv && i < jxta_vector_size(elemv); i++) {
            jxta_vector_get_object_at(elemv, JXTA_OBJECT_PPTR(&elemAttr), i);
            element = elemAttr->element;
            if (elemAttr->attribute == NULL) {
                if ('#' == *elemAttr->value) {
                    elemValue = elemAttr->value + 1;
                } else {
                    elemValue = elemAttr->value;
                }
            } else {
                if (attrString == NULL) {
                    attrString = jstring_new_0();
                }
                jstring_append_2(attrString, elemAttr->attribute);
                jstring_append_2(attrString, "=\"");
                if ('#' == *elemAttr->value) {
                    jstring_append_2(attrString, elemAttr->value + 1);
                } else {
                    jstring_append_2(attrString, elemAttr->value);
                }
                jstring_append_2(attrString, "\" ");
            }
            JXTA_OBJECT_RELEASE(elemAttr);
        }
        if (element != NULL) {
            jstring_append_2(out, "<");
            jstring_append_2(out, element);
        }
        if (attrString != NULL) {
            jstring_append_2(out, " ");
            jstring_append_1(out, attrString);
            JXTA_OBJECT_RELEASE(attrString);
        }
        if (element != NULL) {
            jstring_append_2(out, ">");
            if (elemValue != NULL) {
                jstring_append_2(out, elemValue);
            }
            jstring_append_2(out, "</");
            jstring_append_2(out, element);
            jstring_append_2(out, ">\n");
        }
        if (elemv)
            JXTA_OBJECT_RELEASE(elemv);
        if (elemEntry)
            JXTA_OBJECT_RELEASE(elemEntry);
    }
    jstring_append_2(out, "</");
    jstring_append_2(out, jstring_get_string(prof->nameSpace));
    jstring_append_2(out, ">\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Proffer \n%s\n", jstring_get_string(out));
    if (elements)
        JXTA_OBJECT_RELEASE(elements);
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
    Jxta_vector *elemv;
    if (NULL == prof->elemHash) {
        prof->elemHash = jxta_hashtable_new(3);
    }
    if (jxta_hashtable_get(prof->elemHash, el->element, strlen(el->element), JXTA_OBJECT_PPTR(&elemv)) != JXTA_SUCCESS) {
        elemv = jxta_vector_new(1);
        jxta_hashtable_putnoreplace(prof->elemHash, el->element, strlen(el->element), (Jxta_object *) elemv);
    }
    jxta_vector_add_object_first(elemv, (Jxta_object *) el);
    JXTA_OBJECT_RELEASE(elemv);
}

static Jxta_boolean prof_is_indexable(Jxta_ProfferAdvertisement * prof, const char *element_attr)
{
    unsigned int i;
    Jxta_boolean ret = FALSE;
    if (NULL == prof->indexables)
        return FALSE;
    for (i = 0; i < jxta_vector_size(prof->indexables); i++) {
        Jxta_index *ji = NULL;
        Jxta_status status;
        char *full_index_name;
        status = jxta_vector_get_object_at(prof->indexables, JXTA_OBJECT_PPTR(&ji), i);
        if (status == JXTA_SUCCESS) {
            char *element = NULL;
            char *attrib = NULL;
            int len = 0;
            element = (char *) jstring_get_string(ji->element);
            if (ji->attribute != NULL) {
                attrib = (char *) jstring_get_string(ji->attribute);
                len = strlen(attrib) + strlen(element) + 2;
                full_index_name = calloc(1, len);
                apr_snprintf(full_index_name, len, "%s %s", element, attrib);
            } else {
                len = strlen(element) + 1;
                full_index_name = calloc(1, len);
                apr_snprintf(full_index_name, len, "%s", element);
            }
            if (!strcmp(full_index_name, element_attr)) {
                ret = TRUE;
            }
            free(full_index_name);
        }
        if (NULL != ji)
            JXTA_OBJECT_RELEASE(ji);
        if (TRUE == ret)
            break;
    }
    return ret;
}

/* vi: set ts=4 sw=4 tw=130 et: */
