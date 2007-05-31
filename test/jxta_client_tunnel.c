/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: jxta_client_tunnel.c,v 1.4 2005/04/07 22:58:53 slowhog Exp $
 */

#include <stdio.h>
#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_object.h"
#include "jxta_log.h"
#include "jxta_discovery_service.h"
#include "jxta_socket_tunnel.h"

#define CLT_TUNNEL_TEST_LOG "ClientTunnelTest"

int main(int argc, char *argv[])
{
    Jxta_socket_tunnel *tun;
    Jxta_pipe_adv *adv;
    Jxta_PG *pg;
    Jxta_status rv;
    int ch;

    FILE *f;
    Jxta_log_file *log_f;
    Jxta_log_selector *log_s;

    if (argc < 3) {
        printf("Usage: %s addr_spec adv_file_name\n", argv[0]);
        exit(1);
    }

    jxta_initialize();
    if (argc >= 4) {
        printf("Use log filter: %s\n", argv[3]);
        log_s = jxta_log_selector_new_and_set(argv[3], &rv);
    } else {
        log_s = jxta_log_selector_new_and_set("*.info-fatal", &rv);
    }
    if (NULL == log_s) {
        printf("failed to init log selector.\n");
        exit(-1);
    }

    jxta_log_file_open(&log_f, "-");
    jxta_log_file_attach_selector(log_f, log_s, NULL);
    jxta_log_using(jxta_log_file_append, log_f);

    rv = jxta_PG_new_netpg(&pg);
    if (rv != JXTA_SUCCESS) {
        printf("jxta_PG_new_netpg failed with error: %ld\n", rv);
        exit(-1);
    }

    adv = jxta_pipe_adv_new();
    f = fopen(argv[2], "r");
    if (NULL == f) {
        printf("Failed to open pipe advertisement file %s\n", argv[1]);
    }

    jxta_pipe_adv_parse_file(adv, f);

    jxta_log_append(CLT_TUNNEL_TEST_LOG, JXTA_LOG_LEVEL_TRACE, "Creating socket tunnel ...\n");
    rv = jxta_socket_tunnel_create(pg, argv[1], &tun);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(CLT_TUNNEL_TEST_LOG, JXTA_LOG_LEVEL_ERROR, "Failed to create tunnel with error %ld\n", rv);
        exit(-4);
    }

    jxta_log_append(CLT_TUNNEL_TEST_LOG, JXTA_LOG_LEVEL_TRACE, "Establishing tunnel ...\n");
    rv = jxta_socket_tunnel_establish(tun, adv);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(CLT_TUNNEL_TEST_LOG, JXTA_LOG_LEVEL_ERROR, "Failed to establish tunnel with error %ld\n", rv);
        exit(-4);
    }

    printf("Type 'q'/'Q' to quit\n");
    for (ch = getchar(); 'q' != ch && 'Q' != ch; ch = getchar());

    jxta_log_append(CLT_TUNNEL_TEST_LOG, JXTA_LOG_LEVEL_TRACE, "Quit command issued\n");
    if (jxta_socket_tunnel_is_established(tun)) {
            jxta_log_append(CLT_TUNNEL_TEST_LOG, JXTA_LOG_LEVEL_TRACE, "Teardown tunnel ...\n");
            jxta_socket_tunnel_teardown(tun);
    }
    jxta_log_append(CLT_TUNNEL_TEST_LOG, JXTA_LOG_LEVEL_TRACE, "Delete socket tunnel\n");
    jxta_socket_tunnel_delete(tun);

    jxta_module_stop((Jxta_module *) pg);
    JXTA_OBJECT_RELEASE(pg);

    jxta_log_file_close(log_f);
    jxta_terminate();

    return 0;
}

/* vim: set ts=4 sw=4 wm=130 et: */
