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
 * $Id: mod_jxta.c,v 1.9 2005/07/22 03:12:57 slowhog Exp $
 */


/****
 **  WARNING
 **
 ** lomax@jxta.org
 **
 ** THIS FILE DOES NOT COMPILE YET. IT IS COMMITED FOR REPOSITORY
 ** PURPOSE FOR THE TIME BEING.
 **
 ****/

#include "jxta.h"

#include "ap_config.h"
#include "ap_mmn.h"
#include "httpd.h"
#include "http_config.h"
#include "http_connection.h"
#include "http_log.h"
#include "hash_query.h"
#include "http_protocol.h"
#include "registration.h"


/*
#include "http_request.h"
#include "http_protocol.h"
*/

#define MODNAME "mod_jxta"

AP_DECLARE_DATA module jxta_module;

typedef struct _JxtaConfig JxtaConfig;

struct _JxtaConfig {
    int enabled;
    apr_pool_t *pool;
    Jxta_endpoint_service *endpoint_service;
    JxtaRelayService *relay_service;

};

static int process_jxta_poll(request_rec * r);
static int process_jxta_message(request_rec * r);


static void *create_jxta_server_config(apr_pool_t * p, server_rec * s)
{
    JxtaConfig *config = apr_pcalloc(p, sizeof *config);

    config->enabled = 0;

    return config;
}





static
int jxta_init_handler(apr_pool_t * p, apr_pool_t * plog, apr_pool_t * ptemp, server_rec * s)
{


    ap_add_version_component(p, "JXTA/2 ");

    return 0;
}


static const char *jxta_on(cmd_parms * cmd, void *dummy, int arg)
{
    JxtaConfig *config = ap_get_module_config(cmd->server->module_config,
                                              &jxta_module);
    ap_log_error(APLOG_MARK, APLOG_ERR, 0,  /* status */
                 cmd->server, MODNAME ": started %s, %d", APLOG_MARK);

    config->enabled = arg;

    apr_pool_create(&config->pool, NULL);

    if (config->enabled) {
        config->endpoint_service = jxta_endpoint_service_new_instance();
        config->relay_service = jxta_relay_service_new(config->pool);

        /* FIXME someone: No longer matching. Gene 14/2/02 */
        http_transport_init(config->endpoint_service);
    }

    return NULL;
}



static int process_jxta_request(request_rec * r)
{

    if (strcmp(r->handler, "jxta"))
        return DECLINED;



    if (strcmp(r->method, "GET") == 0)
        return process_jxta_poll(r);
    else
        return process_jxta_message(r);
}


/* FIXME: Refactor this function. */
static int process_jxta_poll(request_rec * r)
{
    //const char ** querylist;
    //apr_status_t status;
    //apr_pool_t * context;

    /* need later */
    //apr_hash_t * querytable;

    /* Check to see if this is NULL, so we won't waste time 
     * doing anything with it if it is.
     */
    char *q = r->parsed_uri.query;

    /* need later */
    /*
       if (q != NULL) {
       querytable = hash_query(r->pool, q);
       }
     */


    /* There is some very peculiar stuff going on here...
     * If this conditional is evaluated false, it should
     * return the block below it.  But it doesn't.  
     * returns an empty document error to the browser.
     * But using the query works fine.  Alternatively,
     * commenting this out everything works fine.
     */
    if (q == NULL) {

        r->content_type = "text/html";

        ap_rputs(DOCTYPE_HTML_3_2 "<html><head><title>JXTA Peer Information</title></head>\n", r);
        ap_rputs("<body><h1 align=\"center\">JXTA Peer  Information</h1>\n", r);
        ap_rputs("JXTA Peer information will appear in this space later.\n", r);
        ap_rputs("<h2>Using mod_jxta</h2>\n", r);
        ap_rputs("mod_jxta recognizes simple queries:\n", r);
        ap_rputs("<ul>\n", r);
        /* item 1 */
        ap_rputs("<li>For retrieving a search registration:", r);
        ap_rputs("<a href=\"http://10.10.10.28/jxta?registration\">", r);
        ap_rputs("/jxta?registration</a></li>\n", r);
        /* item 2 */
        ap_rputs("<li>For retrieving peer information:", r);
        ap_rputs("<a href=\"http://10.10.10.28/jxta?peerinfo\">", r);
        ap_rputs("/jxta?peerinfo</a></li>\n", r);
        ap_rputs("</ul>\n", r);

        /* If there is an unrecognized query, print it out here. */

        ap_rputs("</body>\n</html>\n", r);
        return OK;

    } else if (apr_strnatcmp((const char *) q, "registration") == 0) {
        char *reg = jxta_get_registration();
        r->content_type = "text/xml";
        //Might need DOCTYPE_XML somewhere 
        //ap_rputs(DOCTYPE_HTML_4_0S,r);
        ap_rputs(reg, r);
        return OK;
    } else if (apr_strnatcmp((const char *) q, "peerinfo") == 0) {
        char *reg = jxta_get_peerinfo(r);
        r->content_type = "text/xml";
        //Might need DOCTYPE_XML somewhere 
        //ap_rputs(DOCTYPE_HTML_4_0S,r);
        ap_rputs(reg, r);
        return OK;
    }

    /* FIXME: find the error code for this. */
    return 404;

}


static
apr_status_t read_from_http_request(void *stream, char *buf, apr_size_t len)
{
    request_rec *req = (request_rec *) stream;

    return ap_get_client_block(req, buf, len);
}



static
int process_jxta_message(request_rec * r)
{
    JxtaConfig *config = ap_get_module_config(r->connection->base_server->module_config,
                                              &jxta_module);

    Jxta_message *msg = jxta_message_new();

    int rc = OK;

    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, /* status */
                  r, MODNAME ": reading msg");

    jxta_message_read(msg, read_from_http_request, r);

    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, /* status */
                  r, MODNAME ": read msg");

    {
        apr_hash_t *elements = jxta_message_get_elements_of_namespace(msg, "jxta");
        apr_hash_index_t *entry = apr_hash_first(r->pool, elements);
        void *value;

        while (entry != NULL) {

            apr_hash_this(entry, NULL, NULL, &value);

            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, jxta_message_element_get_name((Jxta_message_element *) value));

            entry = apr_hash_next(entry);
        }
    }

    jxta_endpoint_service_demux(config->endpoint_service, msg);

    r->no_cache = 1;

    return rc;
}



static const command_rec jxta_cmds[] = {
    AP_INIT_FLAG("JXTA", jxta_on, NULL, RSRC_CONF,
                 "\"On\" to enable a jxta peer on this host,  \"Off\" to disable"),
    {NULL}
};

static void register_hooks(apr_pool_t * p)
{
    ap_hook_post_config(jxta_init_handler, NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_handler(process_jxta_request, NULL, NULL, APR_HOOK_LAST);
}

AP_DECLARE_DATA module jxta_module = {
    STANDARD20_MODULE_STUFF,
    NULL,   /* create per-directory config structure */
    NULL,   /* merge per-directory config structures */
    create_jxta_server_config,  /* create per-server config structure */
    NULL,   /* merge per-server config structures */
    jxta_cmds,  /* command apr_table_t */
    register_hooks  /* register hooks */
};
