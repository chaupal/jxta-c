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

static const char *__log_cat = "EA";

#include <stdlib.h>
#include <assert.h>
#include <apr_uri.h>

#include "jxta_types.h"
#include "jxta_log.h"
#include "jxta_object.h"
#include "jxta_endpoint_address.h"
#include "jxta_apr.h"

struct _jxta_endpoint_address {
    JXTA_OBJECT_HANDLE;
    const char *protocol_name;
    const char *protocol_address;
    const char *service_name;
    const char *service_params;
};

typedef struct _jxta_endpoint_address _jxta_endpoint_address;

static void jxta_endpoint_address_free(Jxta_object * obj)
{
    _jxta_endpoint_address *ea = (_jxta_endpoint_address *) obj;

    if (ea->protocol_name) {
        free((void *) ea->protocol_name);
        ea->protocol_name = NULL;
    }
    if (ea->protocol_address) {
        free((void *) ea->protocol_address);
        ea->protocol_name = NULL;
    }
    if (ea->service_name) {
        free((void *) ea->service_name);
        ea->service_name = NULL;
    }
    if (ea->service_params) {
        free((void *) ea->service_params);
        ea->service_params = NULL;
    }
    free(ea);
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_endpoint_address_new_2(const char *protocol_name,
                                                                  const char *protocol_address,
                                                                  const char *service_name, const char *service_params)
{
    _jxta_endpoint_address *ea;

    if ((NULL == protocol_name) || (NULL == protocol_address)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "URI missing scheme or address\n");
        return NULL;
    }

    ea = (_jxta_endpoint_address *) calloc(1, sizeof(_jxta_endpoint_address));

    if (ea != NULL) {
        JXTA_OBJECT_INIT(ea, (JXTA_OBJECT_FREE_FUNC) jxta_endpoint_address_free, 0);

        ea->protocol_name = strdup(protocol_name);
        ea->protocol_address = strdup(protocol_address);
        ea->service_name = service_name == NULL ? NULL : strdup(service_name);
        ea->service_params = service_params == NULL ? NULL : strdup(service_params);
    }

    return (Jxta_endpoint_address *) ea;
}

/**
 * Trim any / or : which might still be before the service name.
 **/
static char *trim_uri(char *uri)
{
    char *pt = uri;

    while (pt && (*pt != 0) && ((*pt == ':') || (*pt == '/')))
        pt++;

    return pt;
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_endpoint_address_new_1(JString * s)
{
    return jxta_endpoint_address_new(jstring_get_string(s));
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_endpoint_address_new(const char *s)
{
    apr_pool_t *pool;   /* (very short lived) */
    apr_uri_t uri;

    _jxta_endpoint_address *ea = NULL;
    char *tmp;
    char *tmp2;
    int tmp_size = 1; /** For the \0 */

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not allocate pool\n");
        return NULL;
    }

    apr_uri_parse(pool, s, &uri);

    if ((NULL == uri.scheme) || (NULL == uri.hostname)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "URI missing scheme or address\n");
        goto Common_exit;
    }

    ea = (_jxta_endpoint_address *) calloc(1, sizeof(_jxta_endpoint_address));

    if (ea == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not allocate Jxta_endpoint_address\n");
        goto Common_exit;
    }

    JXTA_OBJECT_INIT(ea, jxta_endpoint_address_free, NULL);

    ea->protocol_name = strdup(uri.scheme);

    if ((uri.port_str != NULL) && (strlen(uri.port_str) > 0)) {
        int addr_len = strlen(uri.hostname) + strlen(uri.port_str) + 2;
        ea->protocol_address = (const char *) calloc(addr_len, sizeof(char));
        apr_snprintf((char *) ea->protocol_address, addr_len, "%s:%s", uri.hostname, uri.port_str);
    } else {
        ea->protocol_address = strdup(uri.hostname);
    }

    if (NULL != uri.path) {
        tmp_size += strlen(trim_uri(uri.path));
    }
    if (NULL != uri.query) {
        tmp_size += strlen(uri.query) + 1;
    }
    if (NULL != uri.fragment) {
        tmp_size += strlen(uri.fragment) + 1;
    }

    if (tmp_size > 1) {
        tmp = (char *) calloc(1, tmp_size);

        if (NULL != uri.path) {
            strcat(tmp, trim_uri(uri.path));
        }
        if (NULL != uri.query) {
            strcat(tmp + strlen(tmp), "?");
            strcat(tmp, uri.query);
        }
        if (NULL != uri.fragment) {
            strcat(tmp + strlen(tmp), "#");
            strcat(tmp, uri.fragment);
        }

        tmp2 = strchr(tmp, '/');

        if (NULL != tmp2) {
            *tmp2 = '\0';
            ea->service_params = strdup(tmp2 + 1);
        } else {
            ea->service_params = NULL;
        }

        ea->service_name = ((0 == strlen(tmp)) ? NULL : strdup(tmp));

        free(tmp);
    }
  Common_exit:

    /* Done with the pool. We have our private copies on the heap. */
    apr_pool_destroy(pool);

    return (Jxta_endpoint_address *) ea;
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_endpoint_address_new_3(Jxta_id * peer_id, const char *service_name,
                                                                  const char *service_params)
{
    JString *tmp = NULL;
    Jxta_endpoint_address *addr = NULL;
    char *pt;

    JXTA_OBJECT_CHECK_VALID(peer_id);
    jxta_id_get_uniqueportion(peer_id, &tmp);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get the unique portion of the peer id\n");
        return NULL;
    }

    pt = (char *) jstring_get_string(tmp);
    addr = jxta_endpoint_address_new_2((char *) "jxta", pt, service_name, service_params);
    JXTA_OBJECT_CHECK_VALID(addr);

    JXTA_OBJECT_RELEASE(tmp);
    return addr;
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_endpoint_address_new_4(Jxta_endpoint_address* base,
                                                                  const char *service_name, const char *service_params)
{
    _jxta_endpoint_address *ea;

    ea = (_jxta_endpoint_address *) calloc(1, sizeof(_jxta_endpoint_address));

    if (ea != NULL) {
        JXTA_OBJECT_INIT(ea, (JXTA_OBJECT_FREE_FUNC) jxta_endpoint_address_free, 0);

        ea->protocol_name = strdup(base->protocol_name);
        ea->protocol_address = strdup(base->protocol_address);
        ea->service_name = service_name == NULL ? NULL : strdup(service_name);
        ea->service_params = service_params == NULL ? NULL : strdup(service_params);
    }

    return (Jxta_endpoint_address *) ea;
}

JXTA_DECLARE(size_t) jxta_endpoint_address_size(Jxta_endpoint_address * a)
{
    size_t result = strlen(a->protocol_name) + 3 + strlen(a->protocol_address);

    if (NULL != a->service_name) {
        result += 1 + strlen(a->service_name);  /* slash + name */

        if (NULL != a->service_params) {
            result += 1 + strlen(a->service_params);    /* slash + params */
        }
    }

    result++;   /* for the NULL */

    return result;
}

static void to_cstr(Jxta_endpoint_address * me, char * buf)
{
    size_t len;

    len = strlen(me->protocol_name);
    memcpy(buf, me->protocol_name, len);
    buf += len;
    memcpy(buf, "://", 3);
    buf += 3;
    len = strlen(me->protocol_address);
    memcpy(buf, me->protocol_address, len);
    buf += len;

    if (NULL != me->service_name) {
        *buf++ = '/';
        len = strlen(me->service_name);
        memcpy(buf, me->service_name, len);
        buf += len;

        if (NULL != me->service_params) {
            *buf++ = '/';
            len = strlen(me->service_params);
            memcpy(buf, me->service_params, len);
            buf += len;
        }
    }
    *buf = '\0';
}

JXTA_DECLARE(char *) jxta_endpoint_address_to_string(Jxta_endpoint_address * a)
{
    char *str;

    JXTA_OBJECT_CHECK_VALID(a);

    if ((a->protocol_name == NULL) || (a->protocol_address == NULL)) {
        return NULL;
    }

    str = (char *) calloc(jxta_endpoint_address_size(a), sizeof(char));

    if (str == NULL)
        return NULL;

    to_cstr(a, str);
    return str;
}

JXTA_DECLARE(char *) jxta_endpoint_address_to_pstr(Jxta_endpoint_address * me, apr_pool_t * p)
{
    char *buf;

    JXTA_OBJECT_CHECK_VALID(me);
    if ((me->protocol_name == NULL) || (me->protocol_address == NULL)) {
        return NULL;
    }

    buf = apr_pcalloc(p, jxta_endpoint_address_size(me));
    if (buf == NULL)
        return NULL;

    to_cstr(me, buf);
    return buf;
}

JXTA_DECLARE(char *) jxta_endpoint_address_get_transport_addr(Jxta_endpoint_address * me)
{
    char *str;

    JXTA_OBJECT_CHECK_VALID(me);

    if ((me->protocol_name == NULL) || (me->protocol_address == NULL)) {
        return NULL;
    }

    str = calloc(strlen(me->protocol_name) + 3 + strlen(me->protocol_address) + 1, sizeof(char));

    if (str == NULL)
        return NULL;

    strcpy(str, me->protocol_name);
    strcat(str, "://");
    strcat(str, me->protocol_address);

    return str;
}

JXTA_DECLARE(char *) jxta_endpoint_address_get_recipient_cstr(Jxta_endpoint_address * a)
{
    char *str;

    JXTA_OBJECT_CHECK_VALID(a);

    if ((a->service_name == NULL)) {
        return NULL;
    }

    str = (char *) calloc(1 + strlen(a->service_name) + (a->service_params != NULL ? strlen(a->service_params) + 1 : 0) + 1, sizeof(char));
    if (str == NULL)
        return NULL;

    strcpy(str, "/");
    strcat(str, a->service_name);
    if (a->service_params != NULL) {
        strcat(str, "/");
        strcat(str, a->service_params);
    }

    return str;
}

JXTA_DECLARE(const char *) jxta_endpoint_address_get_protocol_name(Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->protocol_name;
}

JXTA_DECLARE(const char *) jxta_endpoint_address_get_protocol_address(Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->protocol_address;
}

JXTA_DECLARE(const char *) jxta_endpoint_address_get_service_name(Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->service_name;
}

JXTA_DECLARE(const char *) jxta_endpoint_address_get_service_params(Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->service_params;
}

static Jxta_boolean string_compare(char const *v1, char const *v2)
{
    if (v1 == v2) {
        return TRUE;
    }
    if ((v1 == NULL) || (v2 == NULL)) {
        return FALSE;
    }
    return (apr_strnatcasecmp(v1, v2) == 0);
}

JXTA_DECLARE(Jxta_boolean) jxta_endpoint_address_equals(Jxta_endpoint_address * addr1, Jxta_endpoint_address * addr2)
{
    JXTA_OBJECT_CHECK_VALID(addr1);
    JXTA_OBJECT_CHECK_VALID(addr2);

    if ((addr1 == NULL) || (addr2 == NULL)) {
        /* Bad arguments. Return FALSE */
        return FALSE;
    }

    /* Optimization: if the addr1 == addr2, they must be the same ! */
    if (addr1 == addr2) {
        return TRUE;
    }
    return (string_compare(addr1->protocol_name, addr2->protocol_name) &&
            string_compare(addr1->protocol_address, addr2->protocol_address) &&
            string_compare(addr1->service_name, addr2->service_name) &&
            string_compare(addr1->service_params, addr2->service_params));
}

JXTA_DECLARE(Jxta_boolean) jxta_endpoint_address_transport_addr_equals(Jxta_endpoint_address * addr1, 
                                                                       Jxta_endpoint_address * addr2)
{
    assert(JXTA_OBJECT_CHECK_VALID(addr1));
    assert(JXTA_OBJECT_CHECK_VALID(addr2));

    if (addr1 == addr2) {
        return TRUE;
    }

    return (0 == strcasecmp(addr1->protocol_name, addr2->protocol_name) &&
            0 == strcasecmp(addr1->protocol_address, addr2->protocol_address));
}


static char *replace_str(const char *str, const char *orig, const char *rep)
{
    char *buffer;
    char *p;

    buffer = calloc(1, strlen(str) + strlen(rep) + 1);

    /* Is 'orig' even in 'str'? */
    if(!(p = strstr(str, orig)))
        return str;

    /* Copy characters from 'str' start to 'orig' str */
    strncpy(buffer, str, p-str);
    buffer[p-str] = '\0';

    sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Replaced %s with %s\n", str, buffer);

    return buffer;
}


JXTA_DECLARE(void) jxta_endpoint_address_replace_variables(Jxta_endpoint_address * addr1, Jxta_hashtable *hash, Jxta_endpoint_address **new_addr)
{
    char **keys=NULL;
    char **keys_save=NULL;
    char *addr_string=NULL;

    addr_string = jxta_endpoint_address_to_string(addr1);

    keys = jxta_hashtable_keys_get(hash);
    keys_save = keys;

    while (NULL != keys && *keys) {
        JString *elem_attribute_j = NULL;
        JString *elem_att_value_j = NULL;
        const char *elem_att_value_c = NULL;
        char *new_str;

        jxta_hashtable_get(hash, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&elem_att_value_j));
        elem_attribute_j = jstring_new_2("%");
        jstring_append_2(elem_attribute_j, *keys);
        jstring_append_2(elem_attribute_j, "%");

        elem_att_value_c = jstring_get_string(elem_att_value_j);

        new_str = replace_str((char *) addr_string, jstring_get_string(elem_attribute_j), (char *) elem_att_value_c);

        if (*new_addr)
            JXTA_OBJECT_RELEASE(*new_addr);
        *new_addr = jxta_endpoint_address_new(new_str);
        if (addr_string) {
            free(addr_string);
            addr_string = NULL;
        }
        addr_string = new_str;
        free(*(keys++));
        JXTA_OBJECT_RELEASE(elem_attribute_j);
        JXTA_OBJECT_RELEASE(elem_att_value_j);
    }
    if (addr_string)
        free(addr_string);
    free(keys_save);
}


/* vim: set ts=4 sw=4 tw=130 et: */
