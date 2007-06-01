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
 * $Id: jxta_message.c,v 1.64 2006/09/12 02:53:40 bondolo Exp $
 */

static const char *__log_cat = "MESSAGE";

#include <stddef.h>
#include <ctype.h>      /* tolower */
#include <string.h>     /* for strncmp */
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/param.h>
#include <netinet/in.h> /* for hton & ntoh byte-order swapping funcs */
#endif

#include <assert.h>

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_debug.h"
#include "jxta_message.h"

apr_uint64_t ntohll(apr_uint64_t swap);
apr_uint64_t htonll(apr_uint64_t swap);

#define ntohll(x) ((((apr_uint64_t)ntohl((apr_uint32_t)(x))) << 32) | (apr_uint64_t)ntohl((apr_uint32_t)((x) >> 32)))
#define htonll(x) ((((apr_uint64_t)htonl((apr_uint32_t)(x))) << 32) | (apr_uint64_t)htonl((apr_uint32_t)((x) >> 32)))

static const char *MESSAGE_EMPTYNS = "";
static const char *MESSAGE_JXTANS = "jxta";
static const char *MESSAGE_SOURCE_NS = "jxta";
static const char *MESSAGE_DESTINATION_NS = "jxta";
static const char *MESSAGE_SOURCE_NAME = "EndpointSourceAddress";
static const char *MESSAGE_DESTINATION_NAME = "EndpointDestinationAddress";
static const char *MESSAGE_QOS_SETTING_NAME = "QoS_Setting";
static const char *MESSAGE_JXTABINARYWIRE_MIME = "application/x-jxta-msg";

static const char JXTAMSG_MSGMAGICSIG[] = { 'j', 'x', 'm', 'g' };

static const char JXTAMSG_ELMMAGICSIG[] = { 'j', 'x', 'e', 'l' };

static const apr_byte_t JXTAMSG_MSGVERS1 = 0;
static const apr_byte_t JXTAMSG_MSGVERS2 = 1;

const apr_byte_t JXTAMSG_MSGVERS = 0;

static const apr_byte_t JXTAMSG1_ELMFLAG_TYPE = 1 << 0;
static const apr_byte_t JXTAMSG1_ELMFLAG_ENCODING = 1 << 1;
static const apr_byte_t JXTAMSG1_ELMFLAG_SIGNATURE = 1 << 2;

static const apr_byte_t JXTAMSG2_MSGFLAG_UTF16BE_STRINGS = 1 << 0;
static const apr_byte_t JXTAMSG2_MSGFLAG_UTF32BE_STRINGS = 1 << 1;

static const apr_byte_t JXTAMSG2_ELMFLAG_UINT64_LENS = 1 << 0;
static const apr_byte_t JXTAMSG2_ELMFLAG_NAME_LITERAL = 1 << 1;
static const apr_byte_t JXTAMSG2_ELMFLAG_TYPE = 1 << 2;
static const apr_byte_t JXTAMSG2_ELMFLAG_SIGNATURE = 1 << 3;
static const apr_byte_t JXTAMSG2_ELMFLAG_ENCODINGS = 1 << 4;
static const apr_byte_t JXTAMSG2_ELMFLAG_ENCODED_SIGNED = 1 << 5;

static const int JXTAMSG_NAMES_EMPTY_IDX = 0;
static const int JXTAMSG_NAMES_JXTA_IDX = 1;
static const int JXTAMSG_NAMES_LITERAL_IDX = 65535;

struct _Jxta_message {
    JXTA_OBJECT_HANDLE;
    struct {
        Jxta_vector *elements;
    } usr;
    const Jxta_qos * qos;
    apr_pool_t * pool;
};

typedef struct _Jxta_message Jxta_message_mutable;

struct _Jxta_message_element {
    JXTA_OBJECT_HANDLE;
    struct {
        char const *ns;
        char const *name;
        char const *mime_type;
        Jxta_bytevector *value;
        Jxta_message_element *sig;
    } usr;
};

typedef struct _Jxta_message_element Jxta_message_element_mutable;

static void jxta_message_delete(Jxta_object * ptr);
static void Jxta_message_element_delete(Jxta_object * ptr);

static char *string_read(ReadFunc read_func, void *stream);
static char *string_read4(ReadFunc read_func, void *stream);
static char *name_read(ReadFunc read_func, void *stream, char const **names_table, int names_count);
static Jxta_status string_write(WriteFunc write_func, void *stream, const char *str);
static Jxta_status string_write4(WriteFunc write_func, void *stream, const char *str);
static char *parseqname(char const *srcqname, char **ns, char **ncname);

static Jxta_status JXTA_STDCALL get_message_to_jstring(void *stream, char const *buf, size_t len);

static Jxta_status message_write_v2(Jxta_message * msg, char const *mime_type, WriteFunc write_func, void *stream);
static int build_v2_names_table(Jxta_message * msg, char const ***names_table);
static apr_uint16_t lookup_name_token(char const *name, char const **names_table, int names_count);
static char const *lookup_name_string(apr_uint16_t token, char const **names_table, int names_count);
static Jxta_status read_v1_element(Jxta_message_element ** anElement, ReadFunc read_func, void *stream,
                                   char const **names_table, int names_count);
static Jxta_status read_v2_element(Jxta_message_element ** anElement, ReadFunc read_func, void *stream,
                                   char const **names_table, int names_count);
static Jxta_status write_v2_element(Jxta_message_element * anElement, WriteFunc write_func, void *stream,
                                    char const **names_table, int names_count);

static Jxta_status add_qos_element(Jxta_message * me);
static Jxta_status extract_qos_element(Jxta_message * me, Jxta_message_element * el);

static apr_status_t msg_cleanup(void *me)
{
    Jxta_message * myself = me;

    JXTA_OBJECT_RELEASE(myself->usr.elements);
    return APR_SUCCESS;
}

static void msg_nop_release(Jxta_object * me)
{
}

static void jxta_message_delete(Jxta_object * ptr)
{
    Jxta_message_mutable *msg = (Jxta_message_mutable *) ptr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Deleting message [%pp]\n", msg);
    JXTA_OBJECT_RELEASE(msg->usr.elements);
    msg->usr.elements = NULL;
    apr_pool_destroy(msg->pool);

    memset(msg, 0xDD, sizeof(Jxta_message_mutable));
    free(msg);
}

JXTA_DECLARE(Jxta_status) jxta_message_create(Jxta_message **me, apr_pool_t *pool)
{
    *me = apr_pcalloc(pool, sizeof(Jxta_message));
    if (NULL == *me) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return JXTA_NOMEM;
    }

    JXTA_OBJECT_INIT(*me, msg_nop_release, NULL);

    (*me)->pool = pool;
    (*me)->usr.elements = jxta_vector_new(1);
    
    if ((*me)->usr.elements == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        *me = NULL;
        return JXTA_NOMEM;
    }
    (*me)->qos = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Created new message [%pp]\n", *me);

    apr_pool_cleanup_register(pool, *me, msg_cleanup, apr_pool_cleanup_null);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_message_destroy(Jxta_message * me)
{
    return apr_pool_cleanup_run(me->pool, me, msg_cleanup);
}

JXTA_DECLARE(Jxta_message *) jxta_message_new(void)
{
    Jxta_message_mutable *msg = (Jxta_message_mutable *) calloc(1, sizeof(Jxta_message_mutable));

    if (NULL == msg) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not malloc message\n");
        return NULL;
    }

    apr_pool_create(&msg->pool, NULL);
    if (NULL == msg->pool) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        free(msg);
        return NULL;
    }

    JXTA_OBJECT_INIT(msg, jxta_message_delete, NULL);

    msg->usr.elements = jxta_vector_new(1);
    if (msg->usr.elements == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        return NULL;
    }

    msg->qos = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Created new message [%pp]\n", msg);

    return msg;
}

JXTA_DECLARE(Jxta_message *) jxta_message_clone(Jxta_message * old)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_message *msg;
    
    JXTA_OBJECT_CHECK_VALID(old);

    msg = jxta_message_new();

    if (NULL == msg)
        return NULL;

    jxta_vector_addall_objects_last( msg->usr.elements, old->usr.elements );

    if (old->qos) {
        jxta_qos_clone( (Jxta_qos**) &msg->qos, old->qos, msg->pool);
    } else {
        msg->qos = NULL;
    }

    return msg;
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_message_get_source(Jxta_message * msg)
{
    Jxta_status err;
    Jxta_endpoint_address *result = NULL;
    Jxta_message_element *el = NULL;
    size_t length;
    char *msg_tmp = NULL;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return NULL;

    err = jxta_message_get_element_2(msg, MESSAGE_SOURCE_NS, MESSAGE_SOURCE_NAME, &el);

    if (err != JXTA_SUCCESS)
        return NULL;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    length = jxta_bytevector_size(el->usr.value);

    msg_tmp = malloc(length + 1);

    if (NULL == msg_tmp)
        goto FINAL_EXIT;

    err = jxta_bytevector_get_bytes_at(el->usr.value, (unsigned char *) msg_tmp, 0, length);

    if (JXTA_SUCCESS != err)
        goto FINAL_EXIT;

    msg_tmp[length] = 0;

    result = jxta_endpoint_address_new(msg_tmp);

  FINAL_EXIT:
    if (NULL != msg_tmp)
        free(msg_tmp);

    if (NULL != el)
        JXTA_OBJECT_RELEASE(el);
    el = NULL;

    return result;
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_message_get_destination(Jxta_message * msg)
{
    Jxta_status err;
    Jxta_endpoint_address *result = NULL;
    Jxta_message_element *el = NULL;
    size_t length;
    char *msg_tmp = NULL;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return NULL;

    err = jxta_message_get_element_2(msg, MESSAGE_DESTINATION_NS, MESSAGE_DESTINATION_NAME, &el);

    if (err != JXTA_SUCCESS)
        return NULL;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    length = jxta_bytevector_size(el->usr.value);

    msg_tmp = malloc(length + 1);

    if (NULL == msg_tmp)
        goto FINAL_EXIT;

    err = jxta_bytevector_get_bytes_at(el->usr.value, (unsigned char *) msg_tmp, 0, length);

    if (JXTA_SUCCESS != err)
        goto FINAL_EXIT;

    msg_tmp[length] = 0;

    result = jxta_endpoint_address_new(msg_tmp);

  FINAL_EXIT:
    if (NULL != msg_tmp)
        free(msg_tmp);

    if (NULL != el)
        JXTA_OBJECT_RELEASE(el);
    el = NULL;

    return result;
}

JXTA_DECLARE(Jxta_status) jxta_message_set_source(Jxta_message * msg, Jxta_endpoint_address * src_addr)
{
    Jxta_status res;
    char *el_value;
    Jxta_message_element *el;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(src_addr))
        return JXTA_INVALID_ARGUMENT;

    el_value = jxta_endpoint_address_to_string(src_addr);

    if (NULL == el_value)
        return JXTA_FAILED;

    el = jxta_message_element_new_2(MESSAGE_SOURCE_NS, MESSAGE_SOURCE_NAME, "text/plain", el_value, strlen(el_value), NULL);

    if (NULL != el) {
        jxta_message_remove_element_2(msg, MESSAGE_SOURCE_NS, MESSAGE_SOURCE_NAME);

        res = jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
        el = NULL;      /* release local */
    } else
        res = JXTA_FAILED;

    free(el_value);
    el_value = NULL;

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_message_set_destination(Jxta_message * msg, Jxta_endpoint_address * dst_addr)
{
    Jxta_status res;
    char *el_value;
    Jxta_message_element *el;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(dst_addr))
        return JXTA_INVALID_ARGUMENT;

    el_value = jxta_endpoint_address_to_string(dst_addr);

    if (NULL == el_value)
        return JXTA_FAILED;

    el = jxta_message_element_new_2(MESSAGE_DESTINATION_NS, MESSAGE_DESTINATION_NAME,
                                    "text/plain", el_value, strlen(el_value), NULL);

    if (NULL != el) {
        (void) jxta_message_remove_element_2(msg, MESSAGE_DESTINATION_NS, MESSAGE_DESTINATION_NAME);

        res = jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
        el = NULL;      /* release local */
    } else
        res = JXTA_FAILED;

    free(el_value);
    el_value = NULL;

    return res;
}

JXTA_DECLARE(Jxta_vector *) jxta_message_get_elements_of_namespace(Jxta_message * msg, char const *ns)
{
    int eachElement = 0;
    Jxta_vector *result = jxta_vector_new(jxta_vector_size(msg->usr.elements));

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return NULL;

    if (NULL == result)
        return NULL;

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_status res;
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(msg->usr.elements, JXTA_OBJECT_PPTR(&anElement), eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        if (0 == strcmp(anElement->usr.ns, ns)) {
            (void) jxta_vector_add_object_last(result, (Jxta_object *) anElement);
        }

        JXTA_OBJECT_RELEASE(anElement); /* release local */
    }

    return result;
}

JXTA_DECLARE(Jxta_vector *) jxta_message_get_elements(Jxta_message * msg)
{
    Jxta_vector *result = msg->usr.elements;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return NULL;

    if (!JXTA_OBJECT_CHECK_VALID(result))
        return NULL;

    JXTA_OBJECT_SHARE(result);

    return result;
}

JXTA_DECLARE(Jxta_status) jxta_message_get_element_1(Jxta_message * msg, char const *element_qname, Jxta_message_element ** el)
{
    Jxta_status res;
    char *tmp;
    char *ns;
    char *name;

    tmp = parseqname(element_qname, &ns, &name);

    if (NULL == tmp)
        return JXTA_NOMEM;

    res = jxta_message_get_element_2(msg, ns, name, el);

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    if (!JXTA_OBJECT_CHECK_VALID(*el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid element will not be be returned\n");
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    res = JXTA_SUCCESS;

  FINAL_EXIT:
    free(tmp);
    tmp = NULL;
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_message_get_element_2(Jxta_message * msg, char const *element_ns, char const *element_ncname,
                                                     Jxta_message_element ** el)
{
    Jxta_status res;
    int eachElement = 0;

    if (NULL == el)
        return JXTA_INVALID_ARGUMENT;

    *el = NULL;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(msg->usr.elements, JXTA_OBJECT_PPTR(&anElement), eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        if ((0 == strcmp(anElement->usr.ns, element_ns)) && (0 == strcmp(anElement->usr.name, element_ncname))) {
            *el = anElement;
            return JXTA_SUCCESS;
        }

        JXTA_OBJECT_RELEASE(anElement);
    }

    return JXTA_ITEM_NOTFOUND;
}

JXTA_DECLARE(Jxta_status) jxta_message_add_element(Jxta_message * msg, Jxta_message_element * el)
{
    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(msg->usr.elements))
        return JXTA_INVALID_ARGUMENT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Add element [msg=%pp] ns='%s' name='%s' len=%d\n", msg, el->usr.ns,
                    el->usr.name, jxta_bytevector_size(el->usr.value));

    return jxta_vector_add_object_last(msg->usr.elements, (Jxta_object *) el);
}

JXTA_DECLARE(Jxta_status) jxta_message_remove_element(Jxta_message * msg, Jxta_message_element * el)
{
    Jxta_status res;
    int eachElement;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return JXTA_INVALID_ARGUMENT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remove element [msg=%pp] ns='%s' name='%s' len=%d\n", msg, el->usr.ns,
                    el->usr.name, jxta_bytevector_size(el->usr.value));

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(msg->usr.elements, JXTA_OBJECT_PPTR(&anElement), eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        if (el == anElement) {
            res = jxta_vector_remove_object_at(msg->usr.elements, NULL, eachElement);   /* RLSE from msg */
            JXTA_OBJECT_RELEASE(anElement);     /* RLSE local */
            return res;
        }

        JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
    }

    return JXTA_ITEM_NOTFOUND;
}

JXTA_DECLARE(Jxta_status) jxta_message_remove_element_1(Jxta_message * msg, char const *element_qname)
{
    Jxta_status res;
    char *ns;
    char *name;
    char *tmp;

    if (NULL == element_qname)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    tmp = parseqname(element_qname, &ns, &name);

    if (NULL == tmp)
        return JXTA_NOMEM;

    res = jxta_message_remove_element_2(msg, ns, name);

    free(tmp);
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_message_remove_element_2(Jxta_message * msg, char const *ns, char const *name)
{
    int eachElement;

    if ((NULL == ns) || (NULL == name))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_status res;
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(msg->usr.elements, JXTA_OBJECT_PPTR(&anElement), eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        if ((0 == strcmp(anElement->usr.ns, ns)) && (0 == strcmp(anElement->usr.name, name))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remove element [msg=%pp] ns='%s' name='%s' len=%d\n", msg,
                            anElement->usr.ns, anElement->usr.name, jxta_bytevector_size(anElement->usr.value));

            res = jxta_vector_remove_object_at(msg->usr.elements, NULL, eachElement);   /* RLSE from msg */
            JXTA_OBJECT_RELEASE(anElement);     /* RLSE local */
            return res;
        }

        JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
    }

    return JXTA_ITEM_NOTFOUND;
}

static Jxta_status string_write4(WriteFunc write_func, void *stream, const char *str)
{
    Jxta_status res;

    apr_uint32_t len = htonl(strlen(str));

    res = (write_func) (stream, (const char *) &len, sizeof(apr_uint32_t));

    if (JXTA_SUCCESS != res)
        return res;

    res = (write_func) (stream, str, strlen(str));

    return res;
}

static Jxta_status string_write(WriteFunc write_func, void *stream, const char *str)
{
    Jxta_status res;
    apr_uint16_t len = htons((apr_uint16_t) strlen(str));

    res = write_func(stream, (const char *) &len, sizeof(apr_uint16_t));

    if (JXTA_SUCCESS != res)
        return res;

    res = write_func(stream, str, strlen(str));

    return res;
}

static char *string_read4(ReadFunc read_func, void *stream)
{
    Jxta_status res;
    apr_uint32_t len = 0;
    char *data;

    res = read_func(stream, (char *) &len, 4);

    if (JXTA_SUCCESS != res)
        return NULL;

    len = ntohl(len);

    data = (char *) malloc(len + 1);

    if (NULL == data)
        return NULL;

    res = read_func(stream, data, len);

    if (JXTA_SUCCESS != res) {
        free(data);
        return NULL;
    }

    data[len] = '\0';

    return data;
}

static char *string_read(ReadFunc read_func, void *stream)
{
    Jxta_status res;
    apr_uint16_t len = 0;
    char *data;

    res = read_func(stream, (char *) &len, sizeof(apr_uint16_t));

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "string_read: could not read length\n");
        return NULL;
    }

    len = ntohs(len);

    data = (char *) malloc(len + 1);

    if (NULL == data) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "string_read: could not alloc string\n");
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "string_read: reading %d bytes to %pp\n", len, data);

    res = read_func(stream, data, len);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "string_read: could not read data\n");
        free(data);
        return NULL;
    }

    data[len] = '\0';

    return data;
}

static char *name_read(ReadFunc read_func, void *stream, char const **names_table, int names_count)
{
    Jxta_status res = JXTA_SUCCESS;
    apr_uint16_t name_idx = 0;
    char *name = NULL;

    res = read_func(stream, (char *) &name_idx, sizeof(name_idx));
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "name_read: Could not read name\n");
        goto FINAL_EXIT;
    }

    name_idx = ntohs(name_idx);

    if (JXTAMSG_NAMES_LITERAL_IDX == name_idx) {
        name = string_read(read_func, stream);
    } else {
        const char *lookup = lookup_name_string(name_idx, names_table, names_count);

        if (NULL != lookup) {
            name = strdup(lookup);
        }
    }

    if (NULL == name) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "name_read: Could not read name\n");
        res = JXTA_IOERR;
        goto FINAL_EXIT;
    }

  FINAL_EXIT:
    return name;
}

static Jxta_status extract_qos_element(Jxta_message * me, Jxta_message_element * el)
{
    const char * data;
    Jxta_qos * qos;
    Jxta_status rv;
    long val;

    data = jxta_bytevector_content_ptr(el->usr.value);
    rv = jxta_qos_create_1(&qos, data, me->pool);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    me->qos = qos;

    rv = jxta_qos_get_long(me->qos, "jxtaMsg:TTL", &val);
    if (JXTA_SUCCESS == rv) {
        if (0 == val) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Message[%pp] received should not have TTL of 0.\n", me);
            return JXTA_INVALID_ARGUMENT;
        }
        --val;
        jxta_qos_set_long((Jxta_qos*) me->qos, "jxtaMsg:TTL", val);
    }

    rv = jxta_qos_get_long(me->qos, "jxtaMsg:lifespan", &val);
    if (JXTA_SUCCESS == rv && time(NULL) > val) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Message[%pp] is discarded with lifespan %ld\n", me, val);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_message_read(Jxta_message * msg, char const *mime_type, ReadFunc read_func, void *stream)
{
    Jxta_status res = JXTA_SUCCESS;
    apr_byte_t message_magic_bytes[sizeof(JXTAMSG_MSGMAGICSIG)];
    apr_byte_t message_format_version;
    apr_byte_t message_flags = 0;
    apr_uint16_t names_count = 0;
    int eachName;
    char const **names_table = NULL;

    apr_uint16_t element_count;
    int each_element;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == mime_type)
        mime_type = MESSAGE_JXTABINARYWIRE_MIME;
    else {
        /* FIXME 20020327 bondolo@jxta.org This test does not contemplate mime
           parameters. It should match only on type and sub-type
         */
        if (0 != strcmp(MESSAGE_JXTABINARYWIRE_MIME, mime_type))
            return JXTA_INVALID_ARGUMENT;
    }

    res = jxta_vector_clear(msg->usr.elements);

    if (JXTA_SUCCESS != res)
        return res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Reading msg [%pp] of type : %s\n", msg, mime_type);

    res = read_func(stream, (char *) message_magic_bytes, sizeof(message_magic_bytes));

    if (JXTA_SUCCESS != res)
        return res;

    if (0 != memcmp(message_magic_bytes, JXTAMSG_MSGMAGICSIG, sizeof(JXTAMSG_MSGMAGICSIG))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ill formed JXTA message--bad magic bytes [%4.4s]\n",
                        message_magic_bytes);
        return JXTA_IOERR;
    }

    res = read_func(stream, (char *) &message_format_version, sizeof(message_format_version));

    if (JXTA_SUCCESS != res)
        goto IO_ERROR_EXIT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "jxta_message_read: Reading message [%pp] from version %d\n",
                    msg, message_format_version);

    if ((JXTAMSG_MSGVERS1 != message_format_version) && (JXTAMSG_MSGVERS2 != message_format_version)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Malformed JXTA message--bad version [%d]\n", message_format_version);
        return JXTA_IOERR;
    }

    if (JXTAMSG_MSGVERS2 == message_format_version) {
        res = read_func(stream, (char *) &message_flags, sizeof(message_flags));

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failure reading message flags.\n");
            return res;
        }

        if (0 != message_flags) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unsupported Message flags.\n");
            return JXTA_INVALID_ARGUMENT;
        }
    }

    res = read_func(stream, (char *) &names_count, sizeof(names_count));

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not read names count\n");
        goto IO_ERROR_EXIT;
    }

    names_count = ntohs(names_count);

    names_count += 2;

    names_table = (char const **) calloc((names_count), sizeof(char const *));

    if (names_table == NULL) {
        res = JXTA_NOMEM;
        goto IO_ERROR_EXIT;
    }

    names_table[JXTAMSG_NAMES_EMPTY_IDX] = MESSAGE_EMPTYNS;

    names_table[JXTAMSG_NAMES_JXTA_IDX] = MESSAGE_JXTANS;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta_message_read: Reading %d names for [msg= %pp]\n", names_count, msg);

    for (eachName = 2; eachName < names_count; eachName++) {
        names_table[eachName] = string_read(read_func, stream);
        if (NULL == names_table[eachName]) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            FILEANDLINE "jxta_message_read: Could not read names table name\n");
            goto IO_ERROR_EXIT;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, FILEANDLINE "[msg= %pp] Read name #%d =[%s]\n", msg,
                        eachName, names_table[eachName]);
    }

    res = read_func(stream, (char *) &element_count, sizeof(element_count));

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read element count\n");
        goto IO_ERROR_EXIT;
    }

    element_count = ntohs(element_count);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta_message_read: Reading %d elements [msg= %pp]\n", element_count, msg);

    for (each_element = 0; each_element < element_count; each_element++) {
        Jxta_message_element *el = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta_message_read: Message [%pp] reading element #%d\n",
                        msg, each_element);

        if (JXTAMSG_MSGVERS1 == message_format_version) {
            res = read_v1_element(&el, read_func, stream, names_table, names_count);
        } else if (JXTAMSG_MSGVERS2 == message_format_version) {
            res = read_v2_element(&el, read_func, stream, names_table, names_count);
        } else {
            assert(FALSE);
        }

        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }

        if (!strcmp(el->usr.ns, MESSAGE_JXTANS) && !strcmp(el->usr.name, MESSAGE_QOS_SETTING_NAME)) {
            res = extract_qos_element(msg, el);
        } else {
            res = jxta_message_add_element(msg, el);
        }

        JXTA_OBJECT_RELEASE(el);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            FILEANDLINE "jxta_message_read: Failed to add element to message.\n");
            goto FINAL_EXIT;
        }
    }

    res = JXTA_SUCCESS;

    goto FINAL_EXIT;

  IO_ERROR_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "IO Error reading from message\n");
    goto FINAL_EXIT;

  FINAL_EXIT:
    if (NULL != names_table) {
        for (eachName = 2; eachName < names_count; eachName++) {
            if (NULL != names_table[eachName]) {
                free((char *) names_table[eachName]);
                names_table[eachName] = NULL;
            }
        }

        free(names_table);
    }

    names_table = NULL;
    return res;
}

static Jxta_status message_write_v1(Jxta_message * msg, char const *mime_type, WriteFunc write_func, void *stream)
{
    Jxta_status res;
    size_t names_count = 2;     /* start with the two default names */
    apr_uint16_t msg_ns_count;
    apr_uint16_t element_count = htons((apr_uint16_t) jxta_vector_size(msg->usr.elements));
    char const **names_table = NULL;

    int eachElement;
    unsigned int eachName;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == mime_type)
        mime_type = MESSAGE_JXTABINARYWIRE_MIME;
    else {
        /* FIXME 20020327 bondolo@jxta.org This test does not contemplate mime
           parameters. It should match only on type and sub-type
         */
        if (0 != strcmp(MESSAGE_JXTABINARYWIRE_MIME, mime_type))
            return JXTA_NOTIMP;
    }

    /*
       Build a table of all of the names present in this message.
     */
    names_table = (char const **) calloc(sizeof(char const *), (names_count + 1));

    if (NULL == names_table) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    names_table[JXTAMSG_NAMES_EMPTY_IDX] = MESSAGE_EMPTYNS;

    names_table[JXTAMSG_NAMES_JXTA_IDX] = MESSAGE_JXTANS;

    names_table[names_count] = NULL;

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(msg->usr.elements, JXTA_OBJECT_PPTR(&anElement), eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        /*
           See if the element's namespace matchs any of the existing names, if not add this
           namespace. 
         */
        for (eachName = 0; eachName <= names_count; eachName++) {
            if (NULL == names_table[eachName]) {
                names_count++;
                names_table = realloc(names_table, sizeof(char *) * (names_count + 1));
                if (NULL == names_table) {
                    res = JXTA_NOMEM;
                    goto FINAL_EXIT;
                }

                names_table[eachName] = strdup(jxta_message_element_get_namespace(anElement));
                if (NULL == names_table[eachName]) {
                    res = JXTA_NOMEM;
                    goto FINAL_EXIT;
                }
                names_table[names_count] = NULL;
                break;
            }

            if (0 == strcmp(jxta_message_element_get_namespace(anElement), names_table[eachName]))
                break;
        }
        JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
    }

    /*
       Begin writing the message!
     */
    res = write_func(stream, JXTAMSG_MSGMAGICSIG, sizeof(JXTAMSG_MSGMAGICSIG));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    res = write_func(stream, (char *) &JXTAMSG_MSGVERS, sizeof(JXTAMSG_MSGVERS));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Writing [msg= %pp] names_count = %d element count = %d \n",
                    msg, names_count, jxta_vector_size(msg->usr.elements));

    msg_ns_count = htons((apr_uint16_t) (names_count - 2));

    res = write_func(stream, (char *) &msg_ns_count, sizeof(msg_ns_count));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    for (eachName = 2; eachName < names_count; eachName++) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, FILEANDLINE "Writing namespace %s [msg= %pp]\n",
                        names_table[eachName], msg);
        res = string_write(write_func, stream, names_table[eachName]);

        if (JXTA_SUCCESS != res)
            goto FINAL_EXIT;
    }

    res = write_func(stream, (char *) &element_count, sizeof(element_count));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    for (eachName = 0; eachName < names_count; eachName++) {
        Jxta_vector *elementsOfNS = jxta_message_get_elements_of_namespace(msg, names_table[eachName]);

        if (NULL == elementsOfNS) {
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }

        for (eachElement = jxta_vector_size(elementsOfNS) - 1; eachElement >= 0; eachElement--) {
            Jxta_message_element *anElement = NULL;
            apr_byte_t el_flags;
            size_t length;
            apr_uint32_t el_length;
            apr_byte_t el_nsid = (apr_byte_t) eachName;

            res = jxta_vector_get_object_at(elementsOfNS, JXTA_OBJECT_PPTR(&anElement), eachElement);

            if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
                continue;
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Writing element [msg=%pp] ns='%s' name='%s' len=%d\n", msg,
                            anElement->usr.ns, anElement->usr.name, jxta_bytevector_size(anElement->usr.value));

            el_flags = 0;
            if (anElement->usr.mime_type)
                el_flags |= JXTAMSG1_ELMFLAG_TYPE;

            res = write_func(stream, JXTAMSG_ELMMAGICSIG, sizeof(JXTAMSG_ELMMAGICSIG));
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            res = write_func(stream, (char *) &el_nsid, sizeof(el_nsid));
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            res = write_func(stream, (char *) &el_flags, sizeof(el_flags));
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            res = string_write(write_func, stream, anElement->usr.name);
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            if (anElement->usr.mime_type) {
                res = string_write(write_func, stream, anElement->usr.mime_type);
                if (JXTA_SUCCESS != res)
                    goto ELEMENT_IO_ERROR;
            }

            length = jxta_bytevector_size(anElement->usr.value);
            el_length = htonl(length);

            res = write_func(stream, (char *) &el_length, sizeof(el_length));
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            if (length == 0) {
                JXTA_OBJECT_RELEASE(anElement);
                continue;
            }

            res = jxta_bytevector_write(anElement->usr.value, write_func, stream, 0, length);
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            /* FIXME 20060526 bondolo Write signature element here */

            JXTA_OBJECT_RELEASE(anElement);
            continue;

          ELEMENT_IO_ERROR:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error writing element.\n");
            JXTA_OBJECT_RELEASE(anElement);
            JXTA_OBJECT_RELEASE(elementsOfNS);
            elementsOfNS = NULL;

            goto FINAL_EXIT;
        }

        JXTA_OBJECT_RELEASE(elementsOfNS);
        elementsOfNS = NULL;
    }

    res = JXTA_SUCCESS;

  FINAL_EXIT:
    if (NULL != names_table) {

        for (eachName = 2; eachName < names_count; eachName++) {
            if (NULL != names_table[eachName]) {
                free((char *) names_table[eachName]);
                names_table[eachName] = NULL;
            }
        }

        free(names_table);
    }
    names_table = NULL;

    return res;
}

static Jxta_status add_qos_element(Jxta_message * me)
{
    Jxta_status rv;
    Jxta_message_element *el;
    char *el_value;

    if (! me->qos) {
        return JXTA_SUCCESS;
    }

    rv = jxta_qos_setting_to_xml(me->qos, &el_value, NULL);
    if (rv != JXTA_SUCCESS) {
        return rv;
    }

    el = jxta_message_element_new_2(MESSAGE_JXTANS, MESSAGE_QOS_SETTING_NAME, "text/xml", el_value, strlen(el_value), NULL);
    if (!el) {
        return JXTA_FAILED;
    }

    jxta_message_remove_element_2(me, MESSAGE_JXTANS, MESSAGE_QOS_SETTING_NAME);
    rv = jxta_message_add_element(me, el);
    JXTA_OBJECT_RELEASE(el);

    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_message_write(Jxta_message * msg, char const *mime_type, WriteFunc write_func, void *stream)
{
    add_qos_element(msg);
    return message_write_v1(msg, mime_type, write_func, stream);
}

JXTA_DECLARE(Jxta_status) jxta_message_write_1(Jxta_message * msg, char const *mime_type, int version, WriteFunc write_func,
                                               void *stream)
{
    add_qos_element(msg);
    if (0 == version) {
        return message_write_v1(msg, mime_type, write_func, stream);
    } else if (1 == version) {
        return message_write_v2(msg, mime_type, write_func, stream);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error writing element.\n");
        return JXTA_INVALID_ARGUMENT;
    }
}

static Jxta_status message_write_v2(Jxta_message * msg, char const *mime_type, WriteFunc write_func, void *stream)
{
    Jxta_status res;
    size_t names_count = 2;     /* start with the two default names */
    apr_uint16_t msg_names_count;
    char const **names_table = NULL;
    Jxta_vector *elements = jxta_message_get_elements(msg);
    apr_uint16_t element_count = htons((apr_uint16_t) jxta_vector_size(elements));
    apr_byte_t flags;
    unsigned int eachElement;
    unsigned int eachName;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == mime_type)
        mime_type = MESSAGE_JXTABINARYWIRE_MIME;
    else {
        /* FIXME 20020327 bondolo@jxta.org This test does not contemplate mime
           parameters. It should match only on type and sub-type
         */
        if (0 != strcmp(MESSAGE_JXTABINARYWIRE_MIME, mime_type))
            return JXTA_NOTIMP;
    }

    /*
       Build a table of all of the names present in this message.
     */
    names_count = build_v2_names_table(msg, &names_table);

    /*
       Begin writing the message!
     */
    res = write_func(stream, JXTAMSG_MSGMAGICSIG, sizeof(JXTAMSG_MSGMAGICSIG));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    res = write_func(stream, (char *) &JXTAMSG_MSGVERS2, sizeof(JXTAMSG_MSGVERS2));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    /* Write flags (We don't currently support any) */
    flags = 0;
    res = write_func(stream, (char *) &flags, sizeof(apr_byte_t));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Writing [msg=%pp] names_count = %d element count = %d \n",
                    msg, (names_count - 2), jxta_vector_size(msg->usr.elements));

    msg_names_count = htons((apr_uint16_t) (names_count - 2));
    res = write_func(stream, (char *) &msg_names_count, sizeof(msg_names_count));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    for (eachName = 2; eachName < names_count; eachName++) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "[msg= %pp] Writing name %d : %s \n", msg, eachName,
                        names_table[eachName]);
        res = string_write(write_func, stream, names_table[eachName]);

        if (JXTA_SUCCESS != res)
            goto FINAL_EXIT;
    }

    res = write_func(stream, (char *) &element_count, sizeof(element_count));

    if (JXTA_SUCCESS != res)
        goto FINAL_EXIT;

    /* Write the message elements */
    for (eachElement = 0; eachElement < jxta_vector_size(elements); eachElement++) {
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(elements, JXTA_OBJECT_PPTR(&anElement), eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Bad message element : %d \n", eachElement);
            goto FINAL_EXIT;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "[msg= %pp] #%d Writing element [%pp]  -- %s:%s \n", msg, eachElement,
                        anElement, anElement->usr.ns, anElement->usr.name);

        res = write_v2_element(anElement, write_func, stream, names_table, names_count);

        JXTA_OBJECT_RELEASE(anElement);

        if (res != JXTA_SUCCESS) {
            goto FINAL_EXIT;
        }
    }

    res = JXTA_SUCCESS;

  FINAL_EXIT:
    if (NULL != elements) {
        JXTA_OBJECT_RELEASE(elements);
        elements = NULL;
    }

    if (NULL != names_table) {
        for (eachName = 2; eachName < names_count; eachName++) {
            if (NULL != names_table[eachName]) {
                free((char *) names_table[eachName]);
                names_table[eachName] = NULL;
            }
        }

        free(names_table);
    }
    names_table = NULL;

    return res;
}

/**
* This could include a more sophisticated handling for the element name field
* in order to add it to the names table if it is used multiple times.
**/
static int build_v2_names_table(Jxta_message * msg, char const ***names_table)
{
    Jxta_status res;
    int names_count = 2;        /* start with the two default names */
    int eachElement;
    int eachElementName;

    *names_table = (char const **) calloc(sizeof(char const *), names_count);

    (*names_table)[JXTAMSG_NAMES_EMPTY_IDX] = MESSAGE_EMPTYNS;

    (*names_table)[JXTAMSG_NAMES_JXTA_IDX] = MESSAGE_JXTANS;

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_message_element *anElement = NULL;
        Jxta_message_element *sigElement = NULL;
        int eachName;

        res = jxta_vector_get_object_at(msg->usr.elements, JXTA_OBJECT_PPTR(&anElement), eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        sigElement = jxta_message_element_get_signature(anElement);

        /*
           See if the elements names match any of the existing names, if not add this name. 
         */
        for (eachElementName = 0; eachElementName < 4; eachElementName++) {
            const char *name = NULL;

            if ((eachElementName > 1) && (NULL == sigElement)) {
                continue;
            }

            switch (eachElementName) {
            case 0:
                name = jxta_message_element_get_namespace(anElement);
                break;

            case 1:
                name = jxta_message_element_get_mime_type(anElement);
                break;

            case 2:
                name = jxta_message_element_get_namespace(sigElement);
                break;

            case 3:
                name = jxta_message_element_get_mime_type(sigElement);
                break;

            default:
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Illegal String index.\n");
                names_count = 0;
                goto FINAL_EXIT;
            }

            if (NULL == name) {
                continue;
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Adding name %s idx=%d \n", name, eachElementName);


            for (eachName = 0; eachName <= names_count; eachName++) {
                if (eachName == names_count) {
                    names_count++;
                    *names_table = realloc(*names_table, sizeof(char const *) * (names_count));
                    if (NULL == names_table) {
                        names_count = 0;
                        goto FINAL_EXIT;
                    }

                    (*names_table)[eachName] = (char const *) strdup(name);
                    if (NULL == (*names_table)[eachName]) {
                        names_count = 0;
                        goto FINAL_EXIT;
                    }

                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Added name %s at %d.\n", name, eachName);
                    break;
                }

                if (0 == strcmp(name, (*names_table)[eachName]))
                    break;
            }
        }

        JXTA_OBJECT_RELEASE(anElement);
        anElement = NULL;

        if (NULL != sigElement) {
            JXTA_OBJECT_RELEASE(sigElement);
            sigElement = NULL;
        }
    }

  FINAL_EXIT:

    return names_count;
}


static apr_uint16_t lookup_name_token(char const *name, char const **names_table, int names_count)
{
    apr_uint16_t eachName = 0;

    while (eachName < names_count) {
        if (0 == strcmp(name, names_table[eachName])) {
            return eachName;
        }

        eachName++;
    }

    return (apr_uint16_t) JXTAMSG_NAMES_LITERAL_IDX;
}


static char const *lookup_name_string(apr_uint16_t token, char const **names_table, int names_count)
{
    if (token >= names_count) {
        return NULL;
    }

    return (names_table[token]);
}


static Jxta_status write_v2_element(Jxta_message_element * anElement, WriteFunc write_func,
                                    void *stream, char const **names_table, int names_count)
{
    Jxta_status res;
    apr_byte_t el_flags;
    size_t length;
    apr_uint32_t el_length;
    apr_uint16_t name;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Writing element [%pp] ns='%s' name='%s' len=%d sig=[%pp]\n", anElement,
                    anElement->usr.ns, anElement->usr.name, jxta_bytevector_size(anElement->usr.value), anElement->usr.sig);

    /* signature */
    res = write_func(stream, JXTAMSG_ELMMAGICSIG, sizeof(JXTAMSG_ELMMAGICSIG));
    if (JXTA_SUCCESS != res)
        goto ELEMENT_IO_ERROR;

    el_flags = 0;
    if (anElement->usr.name) {
        el_flags |= JXTAMSG2_ELMFLAG_NAME_LITERAL;
    }

    if (anElement->usr.mime_type) {
        el_flags |= JXTAMSG2_ELMFLAG_TYPE;
    }

    if (anElement->usr.sig) {
        el_flags |= JXTAMSG2_ELMFLAG_SIGNATURE;
    }

    /* flags */
    res = write_func(stream, (char *) &el_flags, sizeof(el_flags));
    if (JXTA_SUCCESS != res)
        goto ELEMENT_IO_ERROR;

    /* namespace */
    name = htons(lookup_name_token(anElement->usr.ns, names_table, names_count));

    if (JXTAMSG_NAMES_LITERAL_IDX == name) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp] Namespace not found : %s.\n", anElement,
                        anElement->usr.ns);
        res = JXTA_IOERR;
        goto ELEMENT_IO_ERROR;
    }

    res = write_func(stream, (char *) &name, sizeof(apr_uint16_t));
    if (JXTA_SUCCESS != res) {
        goto ELEMENT_IO_ERROR;
    }

    /* name */
    if (anElement->usr.name) {
        res = string_write(write_func, stream, anElement->usr.name);
        if (JXTA_SUCCESS != res) {
            goto ELEMENT_IO_ERROR;
        }
    } else {
        name = htons(JXTAMSG_NAMES_EMPTY_IDX);

        res = write_func(stream, (char *) &name, sizeof(apr_uint16_t));
        if (JXTA_SUCCESS != res) {
            goto ELEMENT_IO_ERROR;
        }
    }

    /* mime type */
    if (anElement->usr.mime_type) {
        name = htons(lookup_name_token(anElement->usr.mime_type, names_table, names_count));

        if (JXTAMSG_NAMES_LITERAL_IDX == name) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp] mime not found : %s.\n", anElement,
                            anElement->usr.mime_type);
            res = JXTA_IOERR;
            goto ELEMENT_IO_ERROR;
        }

        res = write_func(stream, (char *) &name, sizeof(apr_uint16_t));
        if (JXTA_SUCCESS != res) {
            goto ELEMENT_IO_ERROR;
        }
    }

    /* body length */
    length = jxta_bytevector_size(anElement->usr.value);
    el_length = htonl(length);

    res = write_func(stream, (char *) &el_length, sizeof(el_length));
    if (JXTA_SUCCESS != res)
        goto ELEMENT_IO_ERROR;

    if (length > 0) {
        /* body */
        res = jxta_bytevector_write(anElement->usr.value, write_func, stream, 0, length);
        if (JXTA_SUCCESS != res) {
            goto ELEMENT_IO_ERROR;
        }
    }

    if (NULL != anElement->usr.sig) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "[%pp] writing signature element [%pp].\n", anElement,
                        anElement->usr.sig);

        res = write_v2_element(anElement->usr.sig, write_func, stream, names_table, names_count);

        if (res != JXTA_SUCCESS) {
            goto FINAL_EXIT;
        }
    }

    res = JXTA_SUCCESS;
    goto FINAL_EXIT;

  ELEMENT_IO_ERROR:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Error writing element [%pp].\n", anElement);

  FINAL_EXIT:
    return res;
}

static Jxta_status read_v1_element(Jxta_message_element ** anElement, ReadFunc read_func, void *stream, char const **names_table,
                                   int names_count)
{
    Jxta_status res;
    apr_byte_t element_magic_bytes[sizeof(JXTAMSG_ELMMAGICSIG)];
    apr_byte_t element_namespace;
    apr_byte_t element_flags;
    apr_uint32_t el_length;
    char *el_name = NULL;
    char *el_mime_type = NULL;
    Jxta_bytevector *el_value = NULL;

    res = read_func(stream, (char *) element_magic_bytes, sizeof(element_magic_bytes));
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read element magic bytes\n");
        goto ELEMENT_IO_ERROR;
    }

    if (0 != memcmp(element_magic_bytes, JXTAMSG_ELMMAGICSIG, sizeof(JXTAMSG_ELMMAGICSIG))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                        FILEANDLINE "jxta_message_read: Ill formed JXTA element--bad element magic bytes [%4.4s]\n",
                        element_magic_bytes);
        res = JXTA_IOERR;
        goto ELEMENT_IO_ERROR;
    }

    res = read_func(stream, (char *) &element_namespace, sizeof(element_namespace));
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read element namespace\n");
        goto ELEMENT_IO_ERROR;
    }

    res = read_func(stream, (char *) &element_flags, sizeof(element_flags));
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read element flags \n");
        goto ELEMENT_IO_ERROR;
    }

    el_name = string_read(read_func, stream);
    if (NULL == el_name) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read element name\n");
        res = JXTA_IOERR;
        goto ELEMENT_IO_ERROR;
    }

    if (element_flags & JXTAMSG1_ELMFLAG_TYPE) {
        el_mime_type = string_read(read_func, stream);
        if (NULL == el_mime_type) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read mimetype\n");
            res = JXTA_IOERR;
            goto ELEMENT_IO_ERROR;
        }
    }

    res = read_func(stream, (char *) &el_length, sizeof(el_length));

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read element length\n");
        goto ELEMENT_IO_ERROR;
    }

    el_length = ntohl(el_length);

    el_value = jxta_bytevector_new_1((size_t) el_length);
    if (el_value == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        res = JXTA_NOMEM;
        goto ELEMENT_IO_ERROR;
    }
    res = jxta_bytevector_add_from_stream_at(el_value, read_func, stream, el_length, 0);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not read bytes from stream\n");
        goto ELEMENT_IO_ERROR;
    }

    if (el_length != jxta_bytevector_size(el_value)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                        FILEANDLINE "jxta_message_read: value doesn't have all the bytes specified.\n");
        res = JXTA_IOERR;
        goto ELEMENT_IO_ERROR;
    }

    /* FIXME 20060526 bondolo Read message signature here. */

    *anElement = jxta_message_element_new_3(names_table[(int) element_namespace], el_name, el_mime_type, el_value, NULL);

    if (*anElement == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_message_read: Could not create element\n");
        res = JXTA_NOMEM;
        goto ELEMENT_IO_ERROR;
    }

    res = JXTA_SUCCESS;

  ELEMENT_IO_ERROR:

    if (NULL != el_name)
        free(el_name);
    el_name = NULL;

    if (NULL != el_mime_type)
        free(el_mime_type);
    el_mime_type = NULL;

    if (NULL != el_value)
        JXTA_OBJECT_RELEASE(el_value);
    el_value = NULL;

    return res;
}

static Jxta_status read_v2_element(Jxta_message_element ** anElement, ReadFunc read_func, void *stream, char const **names_table,
                                   int names_count)
{
    Jxta_status res;
    apr_byte_t element_magic_bytes[sizeof(JXTAMSG_ELMMAGICSIG)];
    apr_byte_t element_flags;
    apr_uint32_t element_length32;      /* for reading in lengths as 32 bit quantities. */
    apr_uint64_t element_length;
    char *el_namespace = NULL;
    char *el_name = NULL;
    char *el_mime_type = NULL;
    Jxta_bytevector *el_value = NULL;
    Jxta_message_element *sigElement = NULL;

    res = read_func(stream, (char *) element_magic_bytes, sizeof(element_magic_bytes));
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read element magic bytes\n");
        goto FINAL_EXIT;
    }

    if (0 != memcmp(element_magic_bytes, JXTAMSG_ELMMAGICSIG, sizeof(JXTAMSG_ELMMAGICSIG))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                        FILEANDLINE "read_v2_element: Ill formed JXTA message--bad element magic bytes [%4.4s]\n",
                        element_magic_bytes);
        res = JXTA_IOERR;
        goto FINAL_EXIT;
    }

    res = read_func(stream, (char *) &element_flags, sizeof(element_flags));
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read element flags \n");
        goto FINAL_EXIT;
    }

    if (element_flags & (JXTAMSG2_ELMFLAG_ENCODINGS | JXTAMSG2_ELMFLAG_ENCODED_SIGNED)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "read_v2_element: Unsupported element flags\n");
        goto FINAL_EXIT;
    }

    el_namespace = name_read(read_func, stream, names_table, names_count);

    if (NULL == el_namespace) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read element namespace\n");
        res = JXTA_IOERR;
        goto FINAL_EXIT;
    }

    if (0 != (element_flags & JXTAMSG2_ELMFLAG_NAME_LITERAL)) {
        el_name = string_read(read_func, stream);
    } else {
        el_name = name_read(read_func, stream, names_table, names_count);

        if (NULL == el_name) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read element name\n");
            res = JXTA_IOERR;
            goto FINAL_EXIT;
        }
    }

    if (0 != (element_flags & JXTAMSG2_ELMFLAG_TYPE)) {
        el_mime_type = name_read(read_func, stream, names_table, names_count);

        if (NULL == el_mime_type) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read element mime\n");
            res = JXTA_IOERR;
            goto FINAL_EXIT;
        }
    }

    if (0 != (JXTAMSG2_ELMFLAG_UINT64_LENS & element_flags)) {
        res = read_func(stream, (char *) &element_length, sizeof(element_length));

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read element length\n");
            goto FINAL_EXIT;
        }

        element_length = ntohll(element_length);
    } else {
        res = read_func(stream, (char *) &element_length32, sizeof(element_length32));

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read element length\n");
            goto FINAL_EXIT;
        }

        element_length = (apr_uint64_t) ntohl(element_length32);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "read_v2_element: Reading body %" APR_INT64_T_FMT " bytes.\n",
                    element_length);

    el_value = jxta_bytevector_new_1((size_t)element_length);
    if (el_value == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Out of memory\n");
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    res = jxta_bytevector_add_from_stream_at(el_value, read_func, stream, (size_t)element_length, 0);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not read bytes from stream\n");
        goto FINAL_EXIT;
    }

    if (element_length != jxta_bytevector_size(el_value)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                        FILEANDLINE "read_v2_element: value doesn't have all the bytes specified.\n");
        res = JXTA_IOERR;
        goto FINAL_EXIT;
    }

    if (0 != (JXTAMSG2_ELMFLAG_SIGNATURE & element_flags)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "read_v2_element: Reading signature.\n");

        res = read_v2_element(&sigElement, read_func, stream, names_table, names_count);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "read_v2_element: Failed reading signature element.\n");
            goto FINAL_EXIT;
        }
    }

    *anElement = jxta_message_element_new_3(el_namespace, el_name, el_mime_type, el_value, sigElement);

    if (*anElement == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "read_v2_element: Could not create element\n");
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    res = JXTA_SUCCESS;

  FINAL_EXIT:
    if (NULL != el_namespace)
        free(el_namespace);
    el_namespace = NULL;

    if (NULL != el_name)
        free(el_name);
    el_name = NULL;

    if (NULL != el_mime_type)
        free(el_mime_type);
    el_mime_type = NULL;

    if (NULL != sigElement)
        JXTA_OBJECT_RELEASE(sigElement);
    sigElement = NULL;

    if (NULL != el_value)
        JXTA_OBJECT_RELEASE(el_value);
    el_value = NULL;

    return res;
}

static Jxta_status JXTA_STDCALL get_message_to_jstring(void *stream, char const *buf, size_t len)
{
    JString *string = (JString *) stream;

    if (!JXTA_OBJECT_CHECK_VALID(string))
        return JXTA_INVALID_ARGUMENT;

    jstring_append_0(string, buf, len);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_message_to_jstring(Jxta_message * msg, char const *mime, JString * string)
{

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    return jxta_message_write(msg, mime, get_message_to_jstring, string);
}

JXTA_DECLARE(Jxta_status) jxta_message_print(Jxta_message * msg)
{
    Jxta_status res;
    JString *string;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    string = jstring_new_0();

    if (NULL == string)
        return JXTA_NOMEM;

    res = jxta_message_to_jstring(msg, NULL, string);

    if (JXTA_SUCCESS != res)
        return res;

    printf("\n===================== Message ======================\n");
    jxta_buffer_display((char *) jstring_get_string(string), jstring_length(string));
    printf("\n================= End of Message ===================\n");

    JXTA_OBJECT_RELEASE(string);
    string = NULL;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_1(char const *qname,
                                                                char const *mime_type,
                                                                char const *value, size_t length, Jxta_message_element * sig)
{
    Jxta_message_element *el = NULL;
    char *tmp = NULL;
    char *ns;
    char *ncname;

    if (NULL == qname)
        return NULL;

    tmp = parseqname(qname, &ns, &ncname);

    if (NULL == tmp)
        return NULL;

    el = jxta_message_element_new_2(ns, ncname, mime_type, value, length, sig);

    free(tmp);
    tmp = NULL;

    return el;
}

JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_2(char const *ns, char const *ncname,
                                                                char const *mime_type,
                                                                char const *value, size_t length, Jxta_message_element * sig)
{
    Jxta_message_element *el = NULL;
    Jxta_bytevector *value_vector = NULL;

    if ((NULL == value) && (0 != length))
        return NULL;

    value_vector = jxta_bytevector_new_1(length);

    if (NULL == value_vector) {
        return NULL;
    }

    if (NULL != value) {
        if (JXTA_SUCCESS != jxta_bytevector_add_bytes_at(value_vector, (unsigned char const *) value, 0, length)) {
            JXTA_OBJECT_RELEASE(value_vector);
            value_vector = NULL;        /* release local */
            return NULL;
        }
    }

    el = jxta_message_element_new_3(ns, ncname, mime_type, value_vector, sig);

    JXTA_OBJECT_RELEASE(value_vector);
    value_vector = NULL;        /* release local */

    return el;
}

JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_3(char const *ns, char const *ncname,
                                                                char const *mime_type, Jxta_bytevector * value,
                                                                Jxta_message_element * sig)
{
    Jxta_message_element_mutable *el = NULL;

    if ((NULL == ns) || (NULL == ncname)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Missing name or namespace\n");
        return NULL;
    }

    if (!JXTA_OBJECT_CHECK_VALID(value)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "bad value parameter\n");
        return NULL;
    }

    if ((NULL != sig) && !JXTA_OBJECT_CHECK_VALID(sig)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "bad signature parameter\n");
        return NULL;
    }

    el = (Jxta_message_element_mutable *) calloc(1, sizeof(Jxta_message_element_mutable));

    if (NULL == el) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        goto ERROR_EXIT;
    }

    JXTA_OBJECT_INIT(el, Jxta_message_element_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "New Element : %s:%s [%s] len=%d signature=[%pp]\n",
                    ns, ncname, mime_type, jxta_bytevector_size(value), sig);

    el->usr.ns = NULL;
    el->usr.name = NULL;
    el->usr.mime_type = NULL;
    el->usr.value = NULL;
    el->usr.sig = NULL;

    el->usr.ns = strdup(ns);
    if (NULL == el->usr.ns) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        goto ERROR_EXIT;
    }

    el->usr.name = strdup(ncname);
    if (NULL == el->usr.name) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        goto ERROR_EXIT;
    }

    if (NULL != mime_type) {
        el->usr.mime_type = strdup(mime_type);
        if (NULL == el->usr.mime_type) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
            goto ERROR_EXIT;
        }
    } else
        el->usr.mime_type = NULL;

    el->usr.value = value;
    JXTA_OBJECT_SHARE(el->usr.value);

    if (NULL != sig) {
        el->usr.sig = JXTA_OBJECT_SHARE(sig);
    }

    goto FINAL_EXIT;

  ERROR_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Something bad happened making element\n");

    if (NULL != el) {
        if (NULL != el->usr.ns)
            free((char *) el->usr.ns);
        el->usr.ns = NULL;

        if (NULL != el->usr.name)
            free((char *) el->usr.name);
        el->usr.name = NULL;

        if (NULL != el->usr.mime_type)
            free((char *) el->usr.mime_type);
        el->usr.mime_type = NULL;

        JXTA_OBJECT_RELEASE(el);
    }
    el = NULL;

  FINAL_EXIT:

    return el;
}

static void Jxta_message_element_delete(Jxta_object * ptr)
{
    Jxta_message_element_mutable *el = (Jxta_message_element_mutable *) ptr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Message element delete [el=%pp] ns='%s' name='%s' len=%d\n", el,
                    el->usr.ns, el->usr.name, jxta_bytevector_size(el->usr.value));

    if (el->usr.ns != NULL)
        free((char *) el->usr.ns);
    el->usr.ns = NULL;

    if (el->usr.name != NULL)
        free((char *) el->usr.name);
    el->usr.name = NULL;

    if (el->usr.mime_type != NULL)
        free((char *) el->usr.mime_type);
    el->usr.mime_type = NULL;

    JXTA_OBJECT_RELEASE(el->usr.value);
    el->usr.value = NULL;

    if (el->usr.sig != NULL)
        JXTA_OBJECT_RELEASE(el->usr.sig);
    el->usr.sig = NULL;

    free(el);
}

JXTA_DECLARE(char const *) jxta_message_element_get_namespace(Jxta_message_element * el)
{
    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    return el->usr.ns;
}

JXTA_DECLARE(char const *) jxta_message_element_get_name(Jxta_message_element * el)
{
    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    return el->usr.name;
}

JXTA_DECLARE(char const *) jxta_message_element_get_mime_type(Jxta_message_element * el)
{

    char const *result = NULL;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    if (el->usr.mime_type == NULL) {
        result = "application/octet-stream";
    } else {
        result = el->usr.mime_type;
    }

    return result;
}

JXTA_DECLARE(Jxta_bytevector *) jxta_message_element_get_value(Jxta_message_element * el)
{

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    if (!JXTA_OBJECT_CHECK_VALID(el->usr.value))
        return NULL;

    return JXTA_OBJECT_SHARE(el->usr.value);
}

JXTA_DECLARE(Jxta_message_element *) jxta_message_element_get_signature(Jxta_message_element * el)
{
    Jxta_message_element *result;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    result = el->usr.sig;

    if (NULL != result)
        JXTA_OBJECT_SHARE(result);

    return result;
}

static char *parseqname(char const *srcqname, char **ns, char **ncname)
{
    char *tmp = NULL;
    char *pt;
    char *nsAt;
    char *nameAt;

    if (NULL == srcqname)
        return NULL;

    pt = strchr(srcqname, ':');

    if (NULL == pt) {
        size_t len = strlen(srcqname);
        tmp = (char *) malloc(len + strlen(MESSAGE_EMPTYNS) + 2);
        if (NULL == tmp)
            return NULL;

        strcpy(tmp, srcqname);
        nsAt = &tmp[len + 1];
        strcpy(nsAt, MESSAGE_EMPTYNS);
        nameAt = tmp;
    } else {
        tmp = strdup(srcqname);
        if (NULL == tmp)
            return NULL;

        pt = tmp + (pt - srcqname);
        *pt = 0;
        nsAt = tmp;
        nameAt = pt + 1;
    }

    if (NULL != ns)
        *ns = nsAt;

    if (NULL != ncname)
        *ncname = nameAt;

    return tmp;
}

JXTA_DECLARE(void) jxta_message_attach_qos(Jxta_message * me, const Jxta_qos * qos)
{
    me->qos = qos;
}

JXTA_DECLARE(const Jxta_qos*) jxta_message_qos(Jxta_message * me)
{
    return me->qos;
}

JXTA_DECLARE(Jxta_status) jxta_message_get_priority(Jxta_message * me, apr_uint16_t * priority)
{
    Jxta_status rv;
    long val;

    if (!me->qos) {
        return JXTA_ITEM_NOTFOUND;
    }

    rv = jxta_qos_get_long(me->qos, "jxtaMsg:priority", &val);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    *priority = (apr_uint16_t)val;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_message_get_ttl(Jxta_message * me, apr_uint16_t * ttl)
{
    Jxta_status rv;
    long val;

    if (!me->qos) {
        return JXTA_ITEM_NOTFOUND;
    }

    rv = jxta_qos_get_long(me->qos, "jxtaMsg:TTL", &val);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    *ttl = (apr_uint16_t)val;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_message_get_lifespan(Jxta_message * me, time_t * lifespan)
{
    Jxta_status rv;
    long val;

    if (!me->qos) {
        return JXTA_ITEM_NOTFOUND;
    }

    rv = jxta_qos_get_long(me->qos, "jxtaMsg:lifespan", &val);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    *lifespan = val;
    return JXTA_SUCCESS;
}

/* vim: set ts=4 sw=4 tw=130 et: */
