/*
 * Copyright (c) 2005-2006 Sun Microsystems, Inc.  All rights reserved.
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
#include <stdlib.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_types.h"
#include "jxta_debug.h"
#include "jxta_log.h"
#include "jxta_hashtable.h"
#include "jxta_range.h"
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/globals.h>

#define __log_cat "Range"

struct _jxta_range {
    JXTA_OBJECT_HANDLE;
    char *nameSpace;
    char *element;
    char *attribute;
    char *range;
    double high;
    double low;
};

static void range_get_elem_attr(const char * elemAttr, char **element, char **attribute);
static Jxta_hashtable *global_range_table = NULL;

static void range_delete(Jxta_object * obj)
{
    Jxta_range *rge;

    rge = (Jxta_range *) obj;
    if (rge->nameSpace)
        free(rge->nameSpace);
    free(rge->element);
    if (rge->attribute)
        free(rge->attribute);
    if (rge->range) 
        free(rge->range);
    free(rge);
}

JXTA_DECLARE(double) jxta_range_cast_to_number(const char *sNum)
{
    double ret = 0;

    ret = xmlXPathCastStringToNumber((const xmlChar *) sNum);
    return ret;
}

/* TODO make range hashtable associate to individual peer group ?? */
JXTA_DECLARE(void) jxta_range_init()
{
    global_range_table = jxta_hashtable_new(32);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Global range hash table initialized\n");
}

JXTA_DECLARE(void) jxta_range_destroy()
{
    if (global_range_table) {
        Jxta_object *dummy;
        Jxta_vector *ranges;
        unsigned int i;

        ranges = jxta_hashtable_values_get(global_range_table);
        for (i = 0; i < jxta_vector_size(ranges); i++) {
            jxta_vector_get_object_at(ranges, &dummy, i);
            JXTA_OBJECT_RELEASE(dummy);
        }
        JXTA_OBJECT_RELEASE(ranges);
        JXTA_OBJECT_RELEASE(global_range_table);
        global_range_table = NULL;
    }
}

/**
* 
**/
JXTA_DECLARE(Jxta_range *)
    jxta_range_new()
{
    Jxta_range *rge;

    rge = calloc(1, sizeof(Jxta_range));
    JXTA_OBJECT_INIT(rge, (JXTA_OBJECT_FREE_FUNC) range_delete, 0);

    return rge;
}

JXTA_DECLARE(Jxta_range *)
    jxta_range_new_1(const char *nameSpace, const char *elemAttr)
{
    Jxta_range *rge = jxta_range_new();

    jxta_range_set_name_space(rge, nameSpace);
    jxta_range_set_element_attr(rge, elemAttr);

    return rge;
}

JXTA_DECLARE(Jxta_status)
    jxta_range_add(Jxta_range * rge, const char *nspace, const char *element, const char *attribute, const char *range,
               Jxta_boolean force)
{
    Jxta_status ret;
    const char *tmp;
    char *rangeSave;
    char low[32];
    char high[32];
    int i;
    int len=0;
    char *full_index_name = NULL;

    if (attribute != NULL) {
        len = strlen(nspace) + strlen(attribute) + strlen(element) + 3;
        full_index_name = (char *) calloc(1, len);
        apr_snprintf(full_index_name, len, "%s %s %s", nspace, element, attribute);
    } else {
        len = strlen(nspace) + strlen(element) + 2;
        full_index_name = (char *) calloc(1, len);
        apr_snprintf(full_index_name, len, "%s %s", nspace, element);
    }

    tmp = attribute;
    rangeSave = (char *) range;
    memset(low, '\0', 32);
    memset(high, '\0', 32);
    ret = JXTA_INVALID_ARGUMENT;
    while (*range) {
        if ('(' == *range++) {
            i = 0;
            while (':' != *range && *range) {
                low[i++] = *range++;
            }
            if (':' != *range) {
                break;
            }
            range++;
            if (':' == *range) {
                range++;
                i = 0;
                while (')' != *range && *range) {
                    high[i++] = *range++;
                }
                if (')' == *range) {
                    ret = JXTA_SUCCESS;
                }
                break;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    if (JXTA_SUCCESS == ret) {
        if (NULL == tmp) {
            tmp = "";
        }
        rge->nameSpace = strdup(nspace);
        rge->element = strdup(element);
        if (NULL != attribute) {
            rge->attribute = strdup(attribute);
        } else {
            rge->attribute = NULL;
        }
        rge->low = jxta_range_cast_to_number(low);
        rge->high = jxta_range_cast_to_number(high);
        rge->range = strdup(rangeSave);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Global Range fullname: %s nspace: %s element:%s attribute:%s range:%s low:%f high:%f \n",
                        full_index_name, rge->nameSpace, rge->element, rge->attribute, rge->range, rge->low, rge->high);
        jxta_hashtable_putnoreplace(global_range_table, full_index_name, strlen(full_index_name), (Jxta_object *) rge);

    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Bad Range specified %s\n", rangeSave);
        ret = JXTA_INVALID_ARGUMENT;
    }
    if (full_index_name)
        free(full_index_name);

    return ret;

}

JXTA_DECLARE(void)
    jxta_range_set_range(Jxta_range *rge, const char *nSpace, const char *elemAttr, const char *range)
{
    char * element;
    char * attribute;

    range_get_elem_attr(elemAttr, &element, &attribute);
    if (NULL != range) {
        jxta_range_add(rge, nSpace, element, attribute, range, TRUE);
    }
    if (element) free(element);
    if (attribute) free(attribute);
}

JXTA_DECLARE(Jxta_status)
    jxta_range_remove(Jxta_range * rge, const char *nspace, const char *element, const char *attribute, const char *range)
{
    Jxta_status res = JXTA_SUCCESS;

    return res;
}

JXTA_DECLARE(void)
    jxta_range_set_name_space(Jxta_range *rge, const char *nspace)
{
    if (rge->nameSpace) free(rge->nameSpace);
    rge->nameSpace = strdup(nspace);
}

JXTA_DECLARE(const char *)
    jxta_range_get_name_space(Jxta_range *rge)
{
    return rge->nameSpace;
}

JXTA_DECLARE(void)
    jxta_range_set_element_attr(Jxta_range *rge, const char *elementAttr)
{
    char * element=NULL;
    char * attribute=NULL;

    range_get_elem_attr(elementAttr, &element, &attribute);
    if (rge->element) free(rge->element);
    rge->element = element;
    if (rge->attribute) free(rge->attribute);
    rge->attribute = attribute;
}

JXTA_DECLARE(void)
    jxta_range_set_element(Jxta_range *rge, const char *element)
{
    if (rge->element) free(rge->element);
    rge->element = strdup(element);
}

JXTA_DECLARE(const char *)
    jxta_range_get_element(Jxta_range * rge)
{
    return rge->element;
}

JXTA_DECLARE(void)
    jxta_range_set_attribute(Jxta_range *rge, const char *attribute)
{
    if (rge->attribute) free(rge->attribute);
    rge->attribute = strdup(attribute);
}

JXTA_DECLARE(const char *)
    jxta_range_get_attribute(Jxta_range * rge)
{
    return rge->attribute;
}

JXTA_DECLARE(double)
    jxta_range_get_high(Jxta_range * rge)
{
    return rge->high;
}

JXTA_DECLARE(double)
    jxta_range_get_low(Jxta_range * rge)
{
    return rge->low;
}

JXTA_DECLARE(int)
    jxta_range_get_position(Jxta_range * rge, const char *value, int segments)
{
    unsigned int i;
    int ret;
    double high=0;
    double low=0;
    double dValue=0;
    double segment_length=0;
    double segment_work=0;
    double range_length=0;

    if (segments < 2) {
        ret = -2;
        goto CommonExit;
    }
    if (NULL == rge->range) {
        ret = -1;
        goto CommonExit;
    }
    dValue = jxta_range_cast_to_number(value);
    high = jxta_range_get_high(rge);
    low = jxta_range_get_low(rge);
    if (dValue < low) {
        ret = 0;
        goto CommonExit;
    }
    if (dValue > high) {
        ret = segments -1;
        goto CommonExit;
    }
    range_length = high - low;
    segment_length = (range_length / segments);
    segment_work = low + segment_length;
    for (i = 0; i < (unsigned int)segments - 1; i++) {
        if (dValue < segment_work) {
            ret = (int) i;
            break;
        }
        segment_work += segment_length;
    }
    ret = (int) i;

CommonExit:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Get Pos: %d  dV: %10.5f segs: %d  val: %s  rge: %s el:%s att:%s\n",
                                 ret, dValue, segments, value, rge->range, rge->element, rge->attribute); 
    return ret;
}

JXTA_DECLARE(char *)
    jxta_range_get_range_string(Jxta_range * rge)
{
    if (rge->range) {
        return strdup(rge->range);
    }
    return NULL;
}

JXTA_DECLARE(Jxta_vector *)
    jxta_range_get_ranges(void)
{
    return jxta_hashtable_values_get(global_range_table);

}

JXTA_DECLARE(Jxta_status)
    jxta_range_item_added(Jxta_range * rge, double value)
{

    return JXTA_NOTIMP;
}

JXTA_DECLARE(Jxta_status)
    jxta_range_item_replicated(Jxta_range * rge, double value)
{

    return JXTA_NOTIMP;
}

static void range_get_elem_attr(const char * elemAttr, char **element, char **attribute)
{
   char *elem=NULL;
   char *attr=NULL;
   const char *tmp;
   char *dest;

   elem = calloc(1, 128);
   tmp = elemAttr;
   dest = elem;
   while (*tmp) {
       if (' ' == *tmp) { 
           *dest++ = '\0';
           attr = calloc(1, 128);
           dest = attr;
       } else { 
           *dest++ = *tmp;
       }
       tmp++;
   }
   *element = elem;
   *attribute = attr;
}

/* vim: set ts=4 sw=4 et tw=130: */
