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
 * $Id: jxta_endpoint_address.c,v 1.22 2005/02/08 01:04:46 bondolo Exp $
 */
#include <stdlib.h>
#include "jxta_types.h"
#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxtaapr.h"
#include <apr_uri.h>
#include "jxta_endpoint_address.h"


struct _jxta_endpoint_address {
    JXTA_OBJECT_HANDLE;
    char *protocol_name;
    char *protocol_address;
    char *service_name;
    char *service_params;
};

static void jxta_endpoint_address_free(Jxta_object * obj)
{
    Jxta_endpoint_address *ea = (Jxta_endpoint_address *) obj;

    if (ea->protocol_name) {
        free(ea->protocol_name);
        ea->protocol_name = NULL;
    }
    if (ea->protocol_address) {
        free(ea->protocol_address);
        ea->protocol_name = NULL;
    }
    if (ea->service_name) {
        free(ea->service_name);
        ea->service_name = NULL;
    }
    if (ea->service_params) {
        free(ea->service_params);
        ea->service_params = NULL;
    }
    free(ea);
}


Jxta_endpoint_address *jxta_endpoint_address_new2(const char *protocol_name,
                                                  const char *protocol_address,
                                                  const char *service_name,
                                                  const char *service_params)
{
    Jxta_endpoint_address *ea = (Jxta_endpoint_address *)
        malloc(sizeof(Jxta_endpoint_address));

    if (ea == NULL)
        return NULL;

    memset(ea, 0, sizeof(Jxta_endpoint_address));
    JXTA_OBJECT_INIT(ea, (JXTA_OBJECT_FREE_FUNC) jxta_endpoint_address_free, 0);

    ea->protocol_name = protocol_name == NULL ? NULL : strdup(protocol_name);
    ea->protocol_address = protocol_address == NULL ? NULL : strdup(protocol_address);
    ea->service_name = service_name == NULL ? NULL : strdup(service_name);
    ea->service_params = service_params == NULL ? NULL : strdup(service_params);

    return ea;
}

/**
 ** Trim any / or : which might still be before the service name.
 ** This is needed when the Endpoint Address is "incomplete", i.e.,
 ** the protocol name is not present.
 **/
static const char *trim_uri(const char *uri)
{

    const char *pt = uri;

    while (pt && (*pt != 0) && ((*pt == ':') || (*pt == '/')))
        ++pt;

    return pt;
}

Jxta_endpoint_address *jxta_endpoint_address_new(const char *s)
{
    apr_pool_t *pool;   /* (very short lived) */
    apr_uri_t uri;

    Jxta_endpoint_address *ea;
    char *tmp1;
    char *tmp2;

    ea = (Jxta_endpoint_address *)
        malloc(sizeof(Jxta_endpoint_address));
    memset(ea, 0, sizeof(Jxta_endpoint_address));

    if (ea == NULL) {
        JXTA_LOG("Could not allocate Jxta_endpoint_address\n");
        return NULL;
    }

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        free(ea);
        JXTA_LOG("Could not allocate pool\n");
        return NULL;
    }

    JXTA_OBJECT_INIT(ea, jxta_endpoint_address_free, NULL);

    apr_uri_parse(pool, s, &uri);

    if (uri.scheme) {
        ea->protocol_name = strdup(uri.scheme);
    } else {
        ea->protocol_name = NULL;
    }

    if (uri.hostname) {
        if ((uri.port_str != NULL) && (strlen(uri.port_str) > 0)) {
            ea->protocol_address = malloc(strlen(uri.hostname) + strlen(uri.port_str) + 2);
            sprintf(ea->protocol_address, "%s:%s", uri.hostname, uri.port_str);
        } else {
            ea->protocol_address = strdup(uri.hostname);
        }
    } else {
        ea->protocol_address = NULL;
    }

    if (uri.path != NULL) {
        /* Strip of initial / if any */
        tmp1 = trim_uri(uri.path);
    } else {
        tmp1 = uri.path;
    }

    tmp2 = tmp1;

    while (tmp2 != NULL && *tmp2 != '/' && *tmp2 != '\0')
        tmp2++;

    if (tmp2 != NULL && *tmp2 != '\0') {
        *tmp2 = '\0';
        ea->service_params = strdup(tmp2 + 1);
    } else {
        ea->service_params = NULL;
    }
    ea->service_name = (tmp1 == NULL ? NULL : strdup(tmp1));

    /* Done with the pool. We have our private copies on the heap. */
    apr_pool_destroy(pool);

    return ea;
}

int jxta_endpoint_address_size(const Jxta_endpoint_address * a)
{
    return ((a->service_name == NULL ? 0 : strlen(a->service_name))
            + (a->service_params == NULL ? 0 : strlen(a->service_params))
            + (a->protocol_name == NULL ? 0 : strlen(a->protocol_name))
            + (a->protocol_address == NULL ? 0 : strlen(a->protocol_address))
            + 6);
}

char *jxta_endpoint_address_to_string(const Jxta_endpoint_address * a)
{
    char *str;
    const char *pName = a->protocol_name;
    const char *pAddress = a->protocol_address;

    JXTA_OBJECT_CHECK_VALID(a);

    str = malloc(jxta_endpoint_address_size(a));

    if (str == NULL)
        return NULL;

    if (a->protocol_name == NULL) {
        pName = "";
    }

    if (a->protocol_address == NULL) {
        pAddress = "";
    }

    if (!a->service_name)
        sprintf(str, "%s://%s/", pName, pAddress);
    else if (a->service_params)
        sprintf(str, "%s://%s/%s/%s", pName, pAddress, a->service_name, a->service_params);
    else
        sprintf(str, "%s://%s/%s", pName, pAddress, a->service_name);

    return str;
}


const char *jxta_endpoint_address_get_protocol_name(const  Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->protocol_name;
}

const char *jxta_endpoint_address_get_protocol_address(const Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->protocol_address;
}

const char *jxta_endpoint_address_get_service_name(const Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->service_name;
}

const char *jxta_endpoint_address_get_service_params(const Jxta_endpoint_address * a)
{
    JXTA_OBJECT_CHECK_VALID(a);

    return a->service_params;
}

static boolean string_compare(char const *v1, char const *v2)
{

    if (v1 == v2) {
        return TRUE;
    }
    if ((v1 == NULL) || (v2 == NULL)) {
        return FALSE;
    }
    return (apr_strnatcasecmp(v1, v2) == 0);
}

Jxta_boolean jxta_endpoint_address_equals(const Jxta_endpoint_address * addr1, const Jxta_endpoint_address * addr2)
{

    JXTA_OBJECT_CHECK_VALID(addr1);
    JXTA_OBJECT_CHECK_VALID(addr2);

    if ((addr1 == NULL) || (addr2 == NULL)) {
        /* Bad arguments. Return FALSE */
        return FALSE;
    }

    /* Optimization: if the addr1 == addr2, the must be the same ! */
    if (addr1 == addr2) {
        return TRUE;
    }
    return (string_compare((char const *) addr1->protocol_name, (char const *) addr2->protocol_name) &&
            string_compare((char const *) addr1->protocol_address, (char const *) addr2->protocol_address) &&
            string_compare((char const *) addr1->service_name, (char const *) addr2->service_name) &&
            string_compare((char const *) addr1->service_params, (char const *) addr2->service_params));
}
