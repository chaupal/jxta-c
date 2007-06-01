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
 * $Id: jxta_message.c,v 1.44 2005/09/10 01:04:55 slowhog Exp $
 */

static const char *__log_cat = "MESSAGE";

#include <stddef.h>
#include <ctype.h>  /* tolower */
#include <string.h> /* for strncmp */
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
typedef UCHAR uint8_t;
typedef USHORT uint16_t;
typedef UINT uint32_t;
#else
#include <stdint.h>
#include <sys/param.h>
#include <netinet/in.h> /* for hton & ntoh byte-order swapping funcs */
#endif

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_debug.h"
#include "jxta_message.h"

/*************************************************************************
 **
 *************************************************************************/
static const char *MESSAGE_EMPTYNS = "";
static const char *MESSAGE_JXTANS = "jxta";
static const char *MESSAGE_SOURCE_NS = "jxta";
static const char *MESSAGE_DESTINATION_NS = "jxta";
static const char *MESSAGE_SOURCE_NAME = "EndpointSourceAddress";
static const char *MESSAGE_DESTINATION_NAME = "EndpointDestinationAddress";
static const char *MESSAGE_JXTABINARYWIRE_MIME = "application/x-jxta-msg";

static const char JXTAMSG_MSGMAGICSIG[] = { 'j', 'x', 'm', 'g'
};
static const char JXTAMSG_ELMMAGICSIG[] = { 'j', 'x', 'e', 'l'
};
static const uint8_t JXTAMSG_MSGVERS = 0;

static const int JXTAMSG_ELMFLAG_TYPE = 1 << 0;

struct _Jxta_message {
    JXTA_OBJECT_HANDLE;
    struct {
        Jxta_vector *elements;
    } usr;
};

typedef struct _Jxta_message Jxta_message_mutable;

struct _Jxta_message_element {
    JXTA_OBJECT_HANDLE;
    struct {
        char *ns;
        char *name;
        char *mime_type;
        Jxta_bytevector *value;
        Jxta_message_element *sig;
    } usr;
};

typedef struct _Jxta_message_element Jxta_message_element_mutable;

/*************************************************************************
 **
 *************************************************************************/
static void jxta_message_delete(Jxta_object * ptr);
static void Jxta_message_element_delete(Jxta_object * ptr);
static char *jxta_string_read(ReadFunc read_func, void *stream);
static char *jxta_string_read4(ReadFunc read_func, void *stream);
static Jxta_status jxta_string_write(WriteFunc write_func, void *stream, const char *str);
static Jxta_status jxta_string_write4(WriteFunc write_func, void *stream, const char *str);
static char *parseqname(char const *srcqname, char **ns, char **ncname);
static Jxta_status JXTA_STDCALL get_message_to_jstring(void *stream, char const *buf, size_t len);

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_message *) jxta_message_new(void)
{
    Jxta_message_mutable *msg = (Jxta_message_mutable *) calloc(1, sizeof(Jxta_message_mutable));

    if (NULL == msg) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not malloc message");
        return NULL;
    }

    JXTA_OBJECT_INIT(msg, jxta_message_delete, NULL);

    msg->usr.elements = jxta_vector_new(0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Created new message [%p]\n", msg);

    return msg;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_message *) jxta_message_clone(Jxta_message * old)
{
    Jxta_message *msg;
    int eachElement = 0;

    if (!JXTA_OBJECT_CHECK_VALID(old))
        return NULL;

    msg = jxta_message_new();

    if (NULL == msg)
        return NULL;

    for (eachElement = jxta_vector_size(old->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_message_element *anElement = NULL;
        if (JXTA_SUCCESS != jxta_vector_get_object_at(old->usr.elements, (Jxta_object **) & anElement, eachElement))
            continue;

        if ((NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            /* could not add the element, we abort */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Encountered bad element in clone");
            JXTA_OBJECT_RELEASE(anElement);
            anElement = NULL;   /* release local */
            JXTA_OBJECT_RELEASE(msg);
            msg = NULL;
            break;
        }

        if (JXTA_SUCCESS != jxta_message_add_element(msg, anElement)) {
            /* could not add the element, we abort */
            JXTA_OBJECT_RELEASE(anElement);
            anElement = NULL;   /* release local */
            JXTA_OBJECT_RELEASE(msg);
            msg = NULL;
            break;
        }

        JXTA_OBJECT_RELEASE(anElement);
        anElement = NULL;   /* release local */
    }

    return msg;
}

/*************************************************************************
 **
 *************************************************************************/
static void jxta_message_delete(Jxta_object * ptr)
{
    Jxta_message_mutable *msg = (Jxta_message_mutable *) ptr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Deleting message [%p]\n", ptr);
    JXTA_OBJECT_RELEASE(msg->usr.elements);
    msg->usr.elements = NULL;

    free(msg);
}

/*************************************************************************
 **
 *************************************************************************/
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
        goto Common_Exit;

    err = jxta_bytevector_get_bytes_at(el->usr.value, (unsigned char *) msg_tmp, 0, length);

    if (JXTA_SUCCESS != err)
        goto Common_Exit;

    msg_tmp[length] = 0;

    result = jxta_endpoint_address_new(msg_tmp);

  Common_Exit:
    if (NULL != msg_tmp)
        free(msg_tmp);

    if (NULL != el)
        JXTA_OBJECT_RELEASE(el);
    el = NULL;

    return result;
}

/*************************************************************************
 **
 *************************************************************************/
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
        goto Common_Exit;

    err = jxta_bytevector_get_bytes_at(el->usr.value, (unsigned char *) msg_tmp, 0, length);

    if (JXTA_SUCCESS != err)
        goto Common_Exit;

    msg_tmp[length] = 0;

    result = jxta_endpoint_address_new(msg_tmp);

  Common_Exit:
    if (NULL != msg_tmp)
        free(msg_tmp);

    if (NULL != el)
        JXTA_OBJECT_RELEASE(el);
    el = NULL;

    return result;
}

/*************************************************************************
 **
 *************************************************************************/
/**
 ** FIXME 2/13/2002 lomax@jxta.org
 ** This function is not thread-safe (and should be)
 **/
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
        (void) jxta_message_remove_element_2(msg, MESSAGE_SOURCE_NS, MESSAGE_SOURCE_NAME);

        res = jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
        el = NULL;  /* release local */
    } else
        res = JXTA_FAILED;

    free(el_value);
    el_value = NULL;

    return res;
}

/*************************************************************************
 **
 *************************************************************************/
/**
 ** FIXME 2/13/2002 lomax@jxta.org
 ** This function is not thread-safe (and should be)
 **/
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
        el = NULL;  /* release local */
    } else
        res = JXTA_FAILED;

    free(el_value);
    el_value = NULL;

    return res;
}

/*************************************************************************
 **
 *************************************************************************/
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

        res = jxta_vector_get_object_at(msg->usr.elements, (Jxta_object **) & anElement, eachElement);

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

/*************************************************************************
 **
 *************************************************************************/
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

/*************************************************************************
 **
 *************************************************************************/
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
        goto Common_Exit;

    if (!JXTA_OBJECT_CHECK_VALID(*el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid element will not be be returned\n");
        res = JXTA_FAILED;
        goto Common_Exit;
    }

    res = JXTA_SUCCESS;

  Common_Exit:
    free(tmp);
    tmp = NULL;
    return res;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_message_get_element_2(Jxta_message * msg, char const *element_ns, char const *element_ncname, Jxta_message_element ** el)
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

        res = jxta_vector_get_object_at(msg->usr.elements, (Jxta_object **) & anElement, eachElement);

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

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_message_add_element(Jxta_message * msg, Jxta_message_element * el)
{
    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(msg->usr.elements))
        return JXTA_INVALID_ARGUMENT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Add element [msg=%p] ns='%s' name='%s' len=%d\n", msg, el->usr.ns,
                    el->usr.name, jxta_bytevector_size(el->usr.value));

    return jxta_vector_add_object_last(msg->usr.elements, (Jxta_object *) el);
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_message_remove_element(Jxta_message * msg, Jxta_message_element * el)
{
    Jxta_status res;
    int eachElement;

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return JXTA_INVALID_ARGUMENT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remove element [msg=%p] ns='%s' name='%s' len=%d\n", msg, el->usr.ns,
                    el->usr.name, jxta_bytevector_size(el->usr.value));

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(msg->usr.elements, (Jxta_object **) & anElement, eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        if (el == anElement) {
            res = jxta_vector_remove_object_at(msg->usr.elements, NULL, eachElement);   /* RLSE from msg */
            JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
            return res;
        }

        JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
    }

    return JXTA_ITEM_NOTFOUND;
}

/*************************************************************************
 **
 *************************************************************************/
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

/*************************************************************************
 **
 *************************************************************************/
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

        res = jxta_vector_get_object_at(msg->usr.elements, (Jxta_object **) & anElement, eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        if ((0 == strcmp(anElement->usr.ns, ns)) && (0 == strcmp(anElement->usr.name, name))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remove element [msg=%p] ns='%s' name='%s' len=%d\n", msg,
                            anElement->usr.ns, anElement->usr.name, jxta_bytevector_size(anElement->usr.value));

            res = jxta_vector_remove_object_at(msg->usr.elements, NULL, eachElement);   /* RLSE from msg */
            JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
            return res;
        }

        JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
    }

    return JXTA_ITEM_NOTFOUND;
}

/*************************************************************************
 **
 *************************************************************************/
static Jxta_status jxta_string_write4(WriteFunc write_func, void *stream, const char *str)
{
    Jxta_status res;

    uint32_t len = htonl(strlen(str));

    res = (write_func) (stream, (const char *) &len, sizeof(uint32_t));

    if (JXTA_SUCCESS != res)
        return res;

    res = (write_func) (stream, str, strlen(str));

    return res;
}

/*************************************************************************
 **
 *************************************************************************/
static
Jxta_status jxta_string_write(WriteFunc write_func, void *stream, const char *str)
{
    Jxta_status res;
    uint16_t len = htons((uint16_t) strlen(str));

    res = write_func(stream, (const char *) &len, sizeof(uint16_t));

    if (JXTA_SUCCESS != res)
        return res;

    res = write_func(stream, str, strlen(str));

    return res;
}

/*************************************************************************
 **
 *************************************************************************/
static char *jxta_string_read4(ReadFunc read_func, void *stream)
{
    Jxta_status res;
    uint32_t len = 0;
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

/*************************************************************************
 **
 *************************************************************************/
static char *jxta_string_read(ReadFunc read_func, void *stream)
{
    Jxta_status res;
    uint16_t len = 0;
    char *data;

    res = read_func(stream, (char *) &len, sizeof(uint16_t));

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_string_read: could not read length\n");
        return NULL;
    }

    len = ntohs(len);

    data = (char *) malloc(len + 1);

    if (NULL == data) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_string_read: could not alloc string\n");
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "jxta_string_read: reading %d bytes to %p\n", len, data);

    res = read_func(stream, data, len);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "jxta_string_read: could not read data\n");
        free(data);
        return NULL;
    }

    data[len] = '\0';

    return data;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_message_read(Jxta_message * msg, char const *mime_type, ReadFunc read_func, void *stream)
{
    Jxta_status res = JXTA_SUCCESS;
    uint8_t message_magic_bytes[sizeof(JXTAMSG_MSGMAGICSIG)];
    uint8_t message_format_version;
    uint16_t namespace_count = 0;

    uint16_t element_count;

    char **namespaces = NULL;
    int i;

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
        goto ERROR_EXIT;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Reading msg [%p]\n", msg);

    res = read_func(stream, (char *) message_magic_bytes, sizeof(message_magic_bytes));

    if (JXTA_SUCCESS != res)
        goto IO_ERROR_EXIT;

    if (0 != memcmp(message_magic_bytes, JXTAMSG_MSGMAGICSIG, sizeof(JXTAMSG_MSGMAGICSIG))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Ill formed JXTA message--bad magic bytes [%4.4s]\n",
                        message_magic_bytes);
        return JXTA_IOERR;
    }

    res = read_func(stream, (char *) &message_format_version, sizeof(message_format_version));

    if (JXTA_SUCCESS != res)
        goto IO_ERROR_EXIT;

    if (JXTAMSG_MSGVERS != message_format_version) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Ill formed JXTA message--bad version [%d]\n",
                        message_format_version);
        return JXTA_IOERR;
    }

    res = read_func(stream, (char *) &namespace_count, sizeof(namespace_count));

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not read namespace count\n");
        goto IO_ERROR_EXIT;
    }

    namespace_count = ntohs(namespace_count);

    namespaces = (char **) malloc((namespace_count + 2) * sizeof(char *));

    if (namespaces == NULL) {
        res = JXTA_NOMEM;
        goto IO_ERROR_EXIT;
    }

    memset(namespaces, 0, (namespace_count + 2) * sizeof(char *));

    namespaces[0] = strdup(MESSAGE_EMPTYNS);

    namespaces[1] = strdup(MESSAGE_JXTANS);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta_message_read: Reading %d namespaces for [msg= %p]\n", namespace_count,
                    msg);

    for (i = 2; i < namespace_count + 2; i++) {
        namespaces[i] = jxta_string_read(read_func, stream);
        if (NULL == namespaces[i]) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "jxta_message_read: Could not read namespace name\n");
            goto IO_ERROR_EXIT;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "jxta_message_read: Read namespace [msg= %p] name=[%s]\n", msg,
                        namespaces[i]);
    }

    res = read_func(stream, (char *) &element_count, sizeof(element_count));

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "jxta_message_read: Could not read element count\n");
        goto IO_ERROR_EXIT;
    }

    element_count = ntohs(element_count);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, FILEANDLINE "jxta_message_read: Reading %d elements [msg= %p]\n",
                    element_count, msg);

    for (i = 0; i < element_count; i++) {
        Jxta_message_element *el = NULL;
        uint8_t element_magic_bytes[sizeof(JXTAMSG_ELMMAGICSIG)];
        uint8_t element_namespace;
        uint8_t element_flags;
        uint32_t el_length;
        char *el_name = NULL;
        char *el_mime_type = NULL;
        Jxta_bytevector *el_value = NULL;

        res = read_func(stream, (char *) element_magic_bytes, sizeof(element_magic_bytes));
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "jxta_message_read: Could not read element magic bytes\n");
            goto ELEMENT_IO_ERROR;
        }

        if (0 != memcmp(element_magic_bytes, JXTAMSG_ELMMAGICSIG, sizeof(JXTAMSG_ELMMAGICSIG))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "jxta_message_read: Ill formed JXTA message--bad element magic bytes [%4.4s]\n",
                            element_magic_bytes);
            res = JXTA_IOERR;
            goto ELEMENT_IO_ERROR;
        }

        res = read_func(stream, (char *) &element_namespace, sizeof(element_namespace));
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "jxta_message_read: Could not read element namespace\n");
            goto ELEMENT_IO_ERROR;
        }

        res = read_func(stream, (char *) &element_flags, sizeof(element_flags));
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "jxta_message_read: Could not read element flags \n");
            goto ELEMENT_IO_ERROR;
        }

        el_name = jxta_string_read(read_func, stream);
        if (NULL == el_name) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "jxta_message_read: Could not read element name\n");
            res = JXTA_IOERR;
            goto ELEMENT_IO_ERROR;
        }

        if (element_flags & JXTAMSG_ELMFLAG_TYPE) {
            el_mime_type = jxta_string_read(read_func, stream);
            if (NULL == el_mime_type) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "jxta_message_read: Could not read mimetype\n");
                res = JXTA_IOERR;
                goto ELEMENT_IO_ERROR;
            }
        }

        res = read_func(stream, (char *) &el_length, sizeof(el_length));

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "jxta_message_read: Could not read element length\n");
            goto ELEMENT_IO_ERROR;
        }

        el_length = ntohl(el_length);

        el_value = jxta_bytevector_new_1(el_length);

        res = jxta_bytevector_add_from_stream_at(el_value, read_func, stream, el_length, 0);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "jxta_message_read: Could not read bytes from stream\n");
            goto ELEMENT_IO_ERROR;
        }

        if (el_length != jxta_bytevector_size(el_value)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "jxta_message_read: value doesn't have all the bytes specified.\n");
            res = JXTA_IOERR;
            goto ELEMENT_IO_ERROR;
        }

        el = jxta_message_element_new_3(namespaces[(int) element_namespace], el_name, el_mime_type, el_value, NULL);

        if (el == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "jxta_message_read: Could not create element\n");
            res = JXTA_NOMEM;
            goto ELEMENT_IO_ERROR;
        }

        res = jxta_message_add_element(msg, el);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "jxta_message_read: Failed to add element to message.\n");
        }

        JXTA_OBJECT_RELEASE(el_value);
        el_value = NULL;    /* release local */

        JXTA_OBJECT_RELEASE(el);
        el = NULL;  /* release local */

        free(el_name);
        el_name = NULL;

        if (NULL != el_mime_type)
            free(el_mime_type);
        el_mime_type = NULL;

        continue;

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

        goto IO_ERROR_EXIT;
    }

    res = JXTA_SUCCESS;

    goto COMMON_EXIT;

  IO_ERROR_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "IO Error reading from message\n");
    goto COMMON_EXIT;

  ERROR_EXIT:

  COMMON_EXIT:
    if (NULL != namespaces) {
        for (i = 0; i < namespace_count + 2; i++) {
            if (NULL != namespaces[i]) {
                free(namespaces[i]);
                namespaces[i] = NULL;
            }
        }

        free(namespaces);
    }

    namespaces = NULL;
    return res;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_message_write(Jxta_message * msg, char const *mime_type, WriteFunc write_func, void *stream)
{
    Jxta_status res;
    size_t namespace_count = 2; /* start with the two default namespaces */
    uint16_t msg_ns_count;
    uint16_t element_count = htons((uint16_t) jxta_vector_size(msg->usr.elements));
    char **namespaces = NULL;

    int eachElement;
    unsigned int eachNS;

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
       Build a table of all of the namespaces present in this message.
     */
    namespaces = (char **) malloc(sizeof(char *) * (namespace_count + 1));

    if (NULL == namespaces) {
        res = JXTA_NOMEM;
        goto Common_Exit;
    }

    namespaces[0] = strdup(MESSAGE_EMPTYNS);

    if (NULL == namespaces[0]) {
        res = JXTA_NOMEM;
        goto Common_Exit;
    }

    namespaces[1] = strdup(MESSAGE_JXTANS);

    if (NULL == namespaces[0]) {
        res = JXTA_NOMEM;
        goto Common_Exit;
    }

    namespaces[namespace_count] = NULL;

    for (eachElement = jxta_vector_size(msg->usr.elements) - 1; eachElement >= 0; eachElement--) {
        Jxta_message_element *anElement = NULL;

        res = jxta_vector_get_object_at(msg->usr.elements, (Jxta_object **) & anElement, eachElement);

        if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Ignoring bad message element\n");
            continue;
        }

        /*
           See if the elements namespace matchs any of the existing namespaces, if not add this
           namespace. 
         */
        for (eachNS = 0; eachNS <= namespace_count; eachNS++) {
            if (NULL == namespaces[eachNS]) {
                namespace_count++;
                namespaces = realloc(namespaces, sizeof(char *) * (namespace_count + 1));
                if (NULL == namespaces) {
                    res = JXTA_NOMEM;
                    goto Common_Exit;
                }

                namespaces[eachNS] = strdup(jxta_message_element_get_namespace(anElement));
                if (NULL == namespaces[eachNS]) {
                    res = JXTA_NOMEM;
                    goto Common_Exit;
                }
                namespaces[namespace_count] = NULL;
                break;
            }

            if (0 == strcmp(jxta_message_element_get_namespace(anElement), namespaces[eachNS]))
                break;
        }
        JXTA_OBJECT_RELEASE(anElement); /* RLSE local */
    }

    /*
       Begin writing the message!
     */
    res = write_func(stream, JXTAMSG_MSGMAGICSIG, sizeof(JXTAMSG_MSGMAGICSIG));

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    res = write_func(stream, (char *) &JXTAMSG_MSGVERS, sizeof(JXTAMSG_MSGVERS));

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Writing [msg= %p] namespace_count = %d element count = %d \n",
                    msg, namespace_count, jxta_vector_size(msg->usr.elements));

    msg_ns_count = htons((uint16_t) (namespace_count - 2));

    res = write_func(stream, (char *) &msg_ns_count, sizeof(msg_ns_count));

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    for (eachNS = 2; eachNS < namespace_count; eachNS++) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, FILEANDLINE "Writing namespace %s [msg= %p]\n", namespaces[eachNS],
                        msg);
        res = jxta_string_write(write_func, stream, namespaces[eachNS]);

        if (JXTA_SUCCESS != res)
            goto Common_Exit;
    }

    res = write_func(stream, (char *) &element_count, sizeof(element_count));

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    for (eachNS = 0; eachNS < namespace_count; eachNS++) {
        Jxta_vector *elementsOfNS = jxta_message_get_elements_of_namespace(msg, namespaces[eachNS]);

        if (NULL == elementsOfNS) {
            res = JXTA_NOMEM;
            goto Common_Exit;
        }

        for (eachElement = jxta_vector_size(elementsOfNS) - 1; eachElement >= 0; eachElement--) {
            Jxta_message_element *anElement = NULL;
            uint8_t el_flags;
            size_t length;
            uint32_t el_length;
            uint8_t el_nsid = (uint8_t) eachNS;

            res = jxta_vector_get_object_at(elementsOfNS, (Jxta_object **) & anElement, eachElement);

            if ((JXTA_SUCCESS != res) || (NULL == anElement) || !JXTA_OBJECT_CHECK_VALID(anElement)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Ignoring bad message element\n");
                continue;
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Writing element [msg=%p] ns='%s' name='%s' len=%d\n", msg,
                            anElement->usr.ns, anElement->usr.name, jxta_bytevector_size(anElement->usr.value));

            el_flags = 0;
            if (anElement->usr.mime_type)
                el_flags |= JXTAMSG_ELMFLAG_TYPE;

            res = write_func(stream, JXTAMSG_ELMMAGICSIG, sizeof(JXTAMSG_ELMMAGICSIG));
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            res = write_func(stream, (char *) &el_nsid, sizeof(el_nsid));
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            res = write_func(stream, (char *) &el_flags, sizeof(el_flags));
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            res = jxta_string_write(write_func, stream, anElement->usr.name);
            if (JXTA_SUCCESS != res)
                goto ELEMENT_IO_ERROR;

            if (anElement->usr.mime_type) {
                res = jxta_string_write(write_func, stream, anElement->usr.mime_type);
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

            JXTA_OBJECT_RELEASE(anElement);
            continue;

          ELEMENT_IO_ERROR:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error writing element.\n");
            JXTA_OBJECT_RELEASE(anElement);
            JXTA_OBJECT_RELEASE(elementsOfNS);
            elementsOfNS = NULL;

            goto Common_Exit;
        }

        JXTA_OBJECT_RELEASE(elementsOfNS);
        elementsOfNS = NULL;
    }

    res = JXTA_SUCCESS;

  Common_Exit:
    if (NULL != namespaces) {

        for (eachNS = 0; eachNS < namespace_count; eachNS++) {
            if (NULL != namespaces[eachNS]) {
                free(namespaces[eachNS]);
                namespaces[eachNS] = NULL;
            }
        }

        free(namespaces);
    }
    namespaces = NULL;

    return res;
}

/*************************************************************************
 **
 *************************************************************************/
static Jxta_status JXTA_STDCALL get_message_to_jstring(void *stream, char const *buf, size_t len)
{
    JString *string = (JString *) stream;

    if (!JXTA_OBJECT_CHECK_VALID(string))
        return JXTA_INVALID_ARGUMENT;

    jstring_append_0(string, buf, len);

    return JXTA_SUCCESS;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_message_to_jstring(Jxta_message * msg, char const *mime, JString * string)
{

    if (!JXTA_OBJECT_CHECK_VALID(msg))
        return JXTA_INVALID_ARGUMENT;

    return jxta_message_write(msg, mime, get_message_to_jstring, string);
}

/*************************************************************************
 **
 *************************************************************************/
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

/*************************************************************************
 **
 *************************************************************************/
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

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_2(char const *ns, char const *ncname,
                                                                char const *mime_type,
                                                                char const *value, size_t length, Jxta_message_element * sig)
{

    Jxta_message_element *el = NULL;
    Jxta_bytevector *value_vector = NULL;

    if ((NULL == value) && (0 != length))
        return NULL;

    value_vector = jxta_bytevector_new_1(length);

    if (NULL == value_vector)
        return NULL;

    if (NULL != value) {
        if (JXTA_SUCCESS != jxta_bytevector_add_bytes_at(value_vector, (unsigned char const *) value, 0, length)) {
            JXTA_OBJECT_RELEASE(value_vector);
            value_vector = NULL;    /* release local */
            return NULL;
        }
    }

    el = jxta_message_element_new_3(ns, ncname, mime_type, value_vector, sig);

    JXTA_OBJECT_RELEASE(value_vector);
    value_vector = NULL;    /* release local */

    return el;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_3(char const *ns, char const *ncname,
                                                                char const *mime_type, Jxta_bytevector * value,
                                                                Jxta_message_element * sig)
{
    Jxta_message_element_mutable *el = NULL;

    if ((NULL == ns) || (NULL == ncname)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Missing name or namespace\n");
        return NULL;
    }

    if (!JXTA_OBJECT_CHECK_VALID(value)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "bad value parameter\n");
        return NULL;
    }

    if ((NULL != sig) && !JXTA_OBJECT_CHECK_VALID(sig)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "bad signature parameter\n");
        return NULL;
    }

    el = (Jxta_message_element_mutable *) calloc(1, sizeof(Jxta_message_element_mutable));

    if (NULL == el) {
        goto Error_Exit;
    }

    JXTA_OBJECT_INIT(el, Jxta_message_element_delete, NULL);

    el->usr.ns = NULL;
    el->usr.name = NULL;
    el->usr.mime_type = NULL;
    el->usr.value = NULL;
    el->usr.sig = NULL;

    el->usr.ns = strdup(ns);
    if (NULL == el->usr.ns)
        goto Error_Exit;

    el->usr.name = strdup(ncname);
    if (NULL == el->usr.name)
        goto Error_Exit;

    if (NULL != mime_type) {
        el->usr.mime_type = strdup(mime_type);
        if (NULL == el->usr.mime_type)
            goto Error_Exit;
    } else
        el->usr.mime_type = NULL;

    el->usr.value = value;
    JXTA_OBJECT_SHARE(el->usr.value);

    if (NULL != sig) {
        el->usr.sig = sig;
        JXTA_OBJECT_SHARE(el->usr.sig);
    }

    goto Common_Exit;

  Error_Exit:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Something bad happened making element\n");

    if (NULL != el) {
        if (NULL != el->usr.ns)
            free(el->usr.ns);
        el->usr.ns = NULL;

        if (NULL != el->usr.name)
            free(el->usr.name);
        el->usr.name = NULL;

        if (NULL != el->usr.mime_type)
            free(el->usr.mime_type);
        el->usr.mime_type = NULL;

        JXTA_OBJECT_RELEASE(el);
    }
    el = NULL;

  Common_Exit:

    return el;
}

/*************************************************************************
 **
 *************************************************************************/
static
void Jxta_message_element_delete(Jxta_object * ptr)
{
    Jxta_message_element_mutable *el = (Jxta_message_element_mutable *) ptr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Message element delete [el=%p] ns='%s' name='%s' len=%d\n", el,
                    el->usr.ns, el->usr.name, jxta_bytevector_size(el->usr.value));

    if (el->usr.ns != NULL)
        free(el->usr.ns);
    el->usr.ns = NULL;

    if (el->usr.name != NULL)
        free(el->usr.name);
    el->usr.name = NULL;

    if (el->usr.mime_type != NULL)
        free(el->usr.mime_type);
    el->usr.mime_type = NULL;

    JXTA_OBJECT_RELEASE(el->usr.value);
    el->usr.value = NULL;

    if (el->usr.sig != NULL)
        JXTA_OBJECT_RELEASE(el->usr.sig);
    el->usr.sig = NULL;

    free(el);
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(char const *) jxta_message_element_get_namespace(Jxta_message_element * el)
{
    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    return el->usr.ns;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(char const *) jxta_message_element_get_name(Jxta_message_element * el)
{
    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    return el->usr.name;
}

/*************************************************************************
 **
 *************************************************************************/
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

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_bytevector *) jxta_message_element_get_value(Jxta_message_element * el)
{

    if (!JXTA_OBJECT_CHECK_VALID(el))
        return NULL;

    if (!JXTA_OBJECT_CHECK_VALID(el->usr.value))
        return NULL;

    return JXTA_OBJECT_SHARE(el->usr.value);
}

/*************************************************************************
 **
 *************************************************************************/
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

/*************************************************************************
 **
 *************************************************************************/
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
