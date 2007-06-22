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
 * $Id: jxta_util_priv.c,v 1.19 2007/04/20 14:43:18 mmx2005 Exp $
 */

static const char *__log_cat = "UTIL";

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "jxta_util_priv.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_peergroup.h"
#include "jxta_membership_service.h"
#include "jxta_discovery_service_private.h"
#include "jxta_log.h"
#include "jxta_id.h"
#include "jxta_transport_http_client.h"
#include "jxta_endpoint_address.h"

#define MAX_LINE_LENGTH 80

Jxta_vector *getPeerids(Jxta_vector * peers)
{
    Jxta_vector *peerIds = NULL;
    unsigned int i = 0;
    JString *jPeerId;
    Jxta_hashtable *peersHash;

    if (NULL == peers)
        return NULL;
    if (jxta_vector_size(peers) > 0) {
        peerIds = jxta_vector_new(jxta_vector_size(peers));
    } else {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "----- check %d peerids: \n", jxta_vector_size(peers));
    peersHash = jxta_hashtable_new(jxta_vector_size(peers));
    for (i = 0; i < jxta_vector_size(peers); i++) {
        const char *pid;
        JString *temp;

        jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&jPeerId), i);
        pid = jstring_get_string(jPeerId);
        if (jxta_hashtable_get(peersHash, pid, strlen(pid), JXTA_OBJECT_PPTR(&temp)) != JXTA_SUCCESS) {
            Jxta_id *jid;

            jxta_hashtable_put(peersHash, pid, strlen(pid), (Jxta_object *) jPeerId);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "----- add peerid to result set: %s\n", pid);
            jxta_id_from_jstring(&jid, jPeerId);
            jxta_vector_add_object_last(peerIds, (Jxta_object *) jid);
            JXTA_OBJECT_RELEASE(jid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "----- peerid is in result set already: %s\n", pid);
            if (temp != NULL) {
                JXTA_OBJECT_RELEASE(temp);
            }
        }
        JXTA_OBJECT_RELEASE(jPeerId);
    }
    JXTA_OBJECT_RELEASE(peersHash);
    return peerIds;
}

static Jxta_vector *load_http_seeding(const apr_uri_t * const uri)
{
    HttpClient *client = NULL;
    HttpRequest *req = NULL;
    HttpResponse *res = NULL;
    Jxta_status status;

    JString *j_uri = NULL;
    JString *seeds_string;

    Jxta_vector *seeds = NULL;
    Jxta_endpoint_address *addr;

    char buf[200];
    int len = 200;
    char *last;
    char *next;
    char *s;
    apr_port_t port;

    /* default http port to 80 */
    port = (uri->port_str) ? uri->port : APR_URI_HTTP_DEFAULT_PORT;

    /* TODO: support proxy. */
    client = http_client_new(NULL, 0, uri->hostname, port, NULL);

    if (client == NULL)
        goto Common_Exit;

    status = http_client_connect(client);
    if (status != JXTA_SUCCESS)
        goto Common_Exit;

    j_uri = jstring_new_0();

    if (uri->path == NULL) {
        jstring_append_2(j_uri, "/");
    } else {
        jstring_append_2(j_uri, uri->path);
        if (uri->query) {
            jstring_append_2(j_uri, "?");
            jstring_append_2(j_uri, uri->query);
        }
    }

    req = http_client_start_request(client, "GET", jstring_get_string(j_uri), NULL);
    http_request_write(req, "\r\n", 2);
    res = http_request_done(req);

    seeds_string = jstring_new_0();

    /* read content */
    len = 200;
    memset(buf, 0, len);
    while (http_response_read(res, buf, &len) == APR_SUCCESS) {
        jstring_append_2(seeds_string, buf);
        len = 200;
        memset(buf, 0, len);
    }

    /* parse seeds */
    s = (char *) jstring_get_string(seeds_string);
    /* 10 should be plenty in most cases */
    seeds = jxta_vector_new(10);

    while ((next = apr_strtok(s, "\n", &last)) != NULL) {
        addr = jxta_endpoint_address_new(next);
        if (addr) {
            jxta_vector_add_object_last(seeds, (Jxta_object *) addr);
            JXTA_OBJECT_RELEASE(addr);
        }
        s = NULL;
    }

    JXTA_OBJECT_RELEASE(seeds_string);

  Common_Exit:
    if (j_uri)
        JXTA_OBJECT_RELEASE(j_uri);
    if (req)
        http_request_free(req);
    if (res)
        http_response_free(res);
    if (client)
        http_client_free(client);

    return seeds;
}

Jxta_vector *load_file_seeding(const apr_uri_t * const uri)
{
    Jxta_vector *seeds = NULL;
    FILE *inp = NULL;
    char line[MAX_LINE_LENGTH];
    char *status;
    JString *addr_jstr = NULL;
    Jxta_endpoint_address *seed_ea = NULL;

    inp = fopen(uri->path, "r");
    if (inp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to open seeding file at location: %s\n", uri->path);
        return NULL;
    }

    /* 10 should be plenty in most cases */
    seeds = jxta_vector_new(10);

    /* FIXME: line limitation is OK for now. */
    for (status = fgets(line, MAX_LINE_LENGTH, inp); status != 0; status = fgets(line, MAX_LINE_LENGTH, inp)) {
        addr_jstr = jstring_new_1(strlen(line));
        jstring_append_0(addr_jstr, line, strlen(line));
        jstring_trim(addr_jstr);
        seed_ea = jxta_endpoint_address_new_1(addr_jstr);
        jxta_vector_add_object_last(seeds, (Jxta_object *) seed_ea);
        JXTA_OBJECT_RELEASE(seed_ea);
        seed_ea = NULL;
        JXTA_OBJECT_RELEASE(addr_jstr);
        addr_jstr = NULL;
    }

    fclose(inp);

    return seeds;
}

/**
 * Load a list of seeds from the provided seeding URI.
 *
 * @param seeding_uri The seeding URI. 
 *
 * @return A vector of Jxta_endpoint_address for seeds, NULL if error occurs.
 */
Jxta_vector *load_seeds_from_uri(const apr_uri_t * const seeding_uri)
{
    assert(seeding_uri);

    /* althogu uri requires to have scheme, APR accepts that.
     * since APR 1.2.x failed to handle uri like scheme:path-rootless by assuming scheme is bind to '://',
     * thus failed to support URI alike file:relative_pathr.
     * So we would use no scheme as a way to provide relative path for 'file:'.
     */
    if (NULL == seeding_uri->scheme || 0 == strcasecmp("file", seeding_uri->scheme)) {
        if (seeding_uri->path) {
            return load_file_seeding(seeding_uri);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Invalid file URI: must have a path.\n");
    } else if (0 == strcasecmp("http", seeding_uri->scheme)) {
        if (seeding_uri->hostname) {
            return load_http_seeding(seeding_uri);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Invalid http URI: must have a hostname.\n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Not supported URI scheme in seeding URI: %s\n", seeding_uri->scheme);
    }

    return NULL;
}

/**
 * Load a list of seeds from the provided seeding URI.
 *
 * @param seeding_uri The seeding URI as a NULL-terminated string.
 * @param pool The memory pool used during the process. NULL will cause a temporary memory pool to be created.
 *
 * @return A vector of Jxta_endpoint_address for seeds, NULL if error occurs.
 */
Jxta_vector *load_seeds(const char *seeding_uri, apr_pool_t * pool)
{
    apr_pool_t *p;
    apr_uri_t uri;
    apr_status_t rc;
    Jxta_vector *rv = NULL;

    assert(seeding_uri);

    if (NULL == pool) {
        apr_pool_create(&p, NULL);
    } else {
        p = pool;
    }

    rc = apr_uri_parse(p, seeding_uri, &uri);
    if (APR_SUCCESS == rc) {
        rv = load_seeds_from_uri(&uri);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Invalid seeding URI: %s\n", seeding_uri);
    }

    if (NULL == pool) {
        apr_pool_destroy(p);
    }

    return rv;
}

char *get_service_key(const char *svc_name, const char *svc_param)
{
    char *key = NULL;
    size_t len;

    if (!svc_name) {
        return NULL;
    }

    len = strlen(svc_name);
    key = (svc_param) ? malloc(len + strlen(svc_param) + 2) : malloc(len + 1);
    if (!key) {
        return NULL;
    }

    strcpy(key, svc_name);
    if (svc_param) {
        key[len] = '/';
        strcpy(key + len + 1, svc_param);
    }
    return key;
}

Jxta_status qos_setting_to_xml(apr_hash_t * setting, char **result, apr_pool_t * p)
{
    apr_hash_index_t *hi = NULL;
    const void *key;
    apr_ssize_t len;
    void *val;
    int cnt;
    JString *buf = NULL;

    cnt = 0;
    for (hi = apr_hash_first(p, setting); hi; hi = apr_hash_next(hi)) {
        if (!cnt++) {
            assert(NULL == buf);
            buf = jstring_new_1(128);
            jstring_append_2(buf, "<QoS_Setting>");
        }
        apr_hash_this(hi, &key, &len, &val);
        assert(NULL != key);
        assert(NULL != val);
        jstring_append_2(buf, "<QoS>");
        jstring_append_2(buf, key);
        jstring_append_2(buf, "<Value>");
        jstring_append_2(buf, val);
        jstring_append_2(buf, "</Value></QoS>");
    }
    if (cnt) {
        jstring_append_2(buf, "</QoS_Setting>");
        *result = apr_pstrdup(p, jstring_get_string(buf));
    } else {
        *result = "";
    }

    JXTA_OBJECT_RELEASE(buf);

    return JXTA_SUCCESS;
}

Jxta_status xml_to_qos_setting(const char *xml, apr_hash_t ** result, apr_pool_t * p)
{
    apr_status_t rv;
    apr_xml_parser *psr;
    apr_xml_doc *doc;
    apr_xml_elem *elt;
    apr_xml_elem *ve;
    apr_hash_t *ht = NULL;

    psr = apr_xml_parser_create(p);
    if (!psr) {
        *result = NULL;
        return JXTA_FAILED;
    }
    rv = apr_xml_parser_feed(psr, xml, strlen(xml));
    if (rv != JXTA_SUCCESS) {
        *result = NULL;
        return JXTA_INVALID_ARGUMENT;
    }
    apr_xml_parser_done(psr, &doc);
    elt = doc->root;
    if (strcmp(elt->name, "QoS_Setting")) {
        *result = NULL;
        return JXTA_INVALID_ARGUMENT;
    }
    for (elt = elt->first_child; elt; elt = elt->next) {
        if (strcmp(elt->name, "QoS")) {
            continue;
        }
        ve = elt->first_child;
        if (strcmp(ve->name, "Value")) {
            continue;
        }
        if (NULL == ht) {
            ht = apr_hash_make(p);
        }
        apr_hash_set(ht, elt->first_cdata.first->text, APR_HASH_KEY_STRING, ve->first_cdata.first->text);
    }

    *result = ht;
    return JXTA_SUCCESS;
}

Jxta_status qos_support_to_xml(const char **capability_list, char **result, apr_pool_t * p)
{
    int cnt;
    JString *buf = NULL;
    const char *name;

    cnt = 0;
    for (name = capability_list[cnt]; *name; name = capability_list[cnt]) {
        if (!cnt++) {
            assert(NULL == buf);
            buf = jstring_new_1(128);
            jstring_append_2(buf, "<QoS_Support>");
        }
        jstring_append_2(buf, "<QoS>");
        jstring_append_2(buf, name);
        jstring_append_2(buf, "</QoS>");
    }
    if (cnt) {
        jstring_append_2(buf, "</QoS_Support>");
        *result = apr_pstrdup(p, jstring_get_string(buf));
    } else {
        *result = "";
    }

    JXTA_OBJECT_RELEASE(buf);

    return JXTA_SUCCESS;
}

Jxta_status xml_to_qos_support(const char *xml, char ***result, apr_pool_t * p)
{
    /* FIXME: implement it */
    return JXTA_NOTIMP;
}

Jxta_status ep_tcp_socket_listen(apr_socket_t ** me, const char *addr, apr_port_t port, apr_int32_t backlog, apr_pool_t * p)
{
    apr_status_t rv;
    apr_sockaddr_t *sa;
    char msg[256];

    rv = apr_sockaddr_info_get(&sa, addr, APR_UNSPEC, port, 0, p);
    if (APR_SUCCESS != rv) {
        apr_strerror(rv, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Socket addr info failed : %s\n", msg);
        *me = NULL;
        return JXTA_FAILED;
    }

    rv = apr_socket_create(me, sa->family, SOCK_STREAM, APR_PROTO_TCP, p);
    if (APR_SUCCESS != rv) {
        apr_strerror(rv, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed create server socket : %s\n", msg);
        *me = NULL;
        return JXTA_FAILED;
    }

    /* REUSEADDR: this may help in case of the port is in TIME_WAIT */
    rv = apr_socket_opt_set(*me, APR_SO_REUSEADDR, 1);
    if (APR_SUCCESS != rv) {
        apr_strerror(rv, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed setting options : %s\n", msg);
    }

    /* bind */
    rv = apr_socket_bind(*me, sa);
    if (APR_SUCCESS != rv) {
        apr_socket_close(*me);
        apr_strerror(rv, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to bind socket : %s\n", msg);
        *me = NULL;
        return JXTA_FAILED;
    }

    /* listen */
    rv = apr_socket_listen(*me, backlog);
    if (APR_SUCCESS != rv) {
        apr_socket_close(*me);
        apr_strerror(rv, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to listen on socket : %s\n", msg);
        *me = NULL;
        return JXTA_FAILED;
    }

    return JXTA_SUCCESS;
}

Jxta_status JXTA_STDCALL brigade_read(void *stream, char *buf, apr_size_t len)
{
    apr_bucket_brigade *b = stream;
    apr_bucket *e;
    apr_bucket *trash;
    apr_status_t rv;
    const char *data;
    apr_size_t sz;
    int done = 0;

    if (0 == len) {
        return JXTA_SUCCESS;
    }

    e = APR_BRIGADE_FIRST(b);
    while (APR_BRIGADE_SENTINEL(b) != e && !done) {
        rv = apr_bucket_read(e, &data, &sz, APR_BLOCK_READ);
        if (APR_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Brigade[%pp] failed to read with status %d.\n", b, rv);
            return rv;
        }

        if (0 == sz) {
            return APR_EOF;
        }

        if (sz >= len) {
            memcpy(buf, data, len);
            /* avoid split when size == len */
            if (sz > len) {
                apr_bucket_split(e, len);
            }
            done = 1;
        } else {
            memcpy(buf, data, sz);
            len -= sz;
            buf += sz;
        }

        trash = e;
        e = APR_BUCKET_NEXT(e);
        apr_bucket_delete(trash);
    }
    return JXTA_SUCCESS;
}

Jxta_status query_all_advs(const char *query, Jxta_credential * scope[], int threshold, Jxta_vector ** results)
{
    Jxta_status status = JXTA_SUCCESS;
    unsigned int i;
    Jxta_credential **passScope = NULL;
    Jxta_vector *groups = NULL;
    Jxta_PG *pg = NULL;
    Jxta_PG *netPeerGroup = NULL;
    Jxta_discovery_service *discovery = NULL;

    groups = jxta_get_registered_groups();
    for (i = 0; (groups != NULL) && i < jxta_vector_size(groups); i++) {
        status = jxta_vector_get_object_at(groups, JXTA_OBJECT_PPTR(&pg), i);
        if (NULL == jxta_PG_parent(pg)) {
            netPeerGroup = pg;
            break;
        }
        JXTA_OBJECT_RELEASE(pg);
        pg = NULL;
    }
    if (NULL == netPeerGroup) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to find NetPeerGroup in query_all_advs\n");
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }
    if (NULL == scope && jxta_vector_size(groups) > 0) {
        int j = 0;

        passScope = calloc(1, sizeof(Jxta_credential *) * (jxta_vector_size(groups) + 1));
        for (i = 0; i < jxta_vector_size(groups); i++) {
            Jxta_PG *ppg;
            Jxta_vector *creds;
            Jxta_membership_service *membership;

            jxta_vector_get_object_at(groups, JXTA_OBJECT_PPTR(&ppg), i);
            jxta_PG_get_membership_service(ppg, &membership);
            if (NULL == membership) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "No membership service found query_all_advs\n");
                JXTA_OBJECT_RELEASE(ppg);
                continue;
            }
            jxta_membership_service_get_currentcreds(membership, &creds);
            if (NULL != creds) {
                if (jxta_vector_size(creds) > 0) {
                    jxta_vector_get_object_at(creds, JXTA_OBJECT_PPTR(&passScope[j++]), 0);
                }
                JXTA_OBJECT_RELEASE(creds);
            }
            JXTA_OBJECT_RELEASE(membership);
            JXTA_OBJECT_RELEASE(ppg);
        }
        passScope[j] = NULL;
    }
    jxta_PG_get_discovery_service(netPeerGroup, &discovery);
    if (NULL == discovery) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "No discovery service found query_all_advs\n");
        goto FINAL_EXIT;
    }
    if (NULL == passScope || NULL == *passScope) {
        passScope = scope;
    }
    status = getLocalGroupsQuery(discovery, query, passScope, results, threshold, FALSE);

  FINAL_EXIT:

    if (NULL == scope && passScope != NULL) {
        i = 0;
        while (passScope[i]) {
            JXTA_OBJECT_RELEASE(passScope[i++]);
        }
        free(passScope);
    }
    if (groups)
        JXTA_OBJECT_RELEASE(groups);
    if (pg)
        JXTA_OBJECT_RELEASE(pg);
    if (discovery)
        JXTA_OBJECT_RELEASE(discovery);
    return status;
}

/* vim: set ts=4 sw=4 et tw=130: */
