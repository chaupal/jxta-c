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
 * $Id: pg_start_stop_test.c,v 1.8 2005/11/17 03:46:27 slowhog Exp $
 */

#include <apr_time.h>

#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_rdv_service.h"

/* rerutn number of connected peers, minus number in case of errors */
int display_peers(Jxta_rdv_service * rdv)
{
    int res = 0;
    Jxta_peer *peer = NULL;
    Jxta_id *pid = NULL;
    Jxta_PA *adv = NULL;
    JString *string = NULL;
    Jxta_time expires = 0;
    Jxta_boolean connected = FALSE;
    Jxta_status err;
    Jxta_vector *peers = NULL;
    Jxta_time currentTime;
    int i;
    char linebuff[1024];
    JString *outputLine = jstring_new_0();
    JString *disconnectedMessage = jstring_new_2("Status: NOT CONNECTED\n");

    /* Get the list of peers */
    err = jxta_rdv_service_get_peers(rdv, &peers);
    if (err != JXTA_SUCCESS) {
        jstring_append_2(outputLine, "Failed getting the peers.\n");
        res = -1;
        goto Common_Exit;
    }

    for (i = 0; i < jxta_vector_size(peers); ++i) {
        err = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
        if (err != JXTA_SUCCESS) {
            jstring_append_2(outputLine, "Failed getting a peer.\n");
            res = -2;
            goto Common_Exit;
        }

        connected = jxta_rdv_service_peer_is_connected(rdv, peer);
        if (connected) {
            res++;
            err = jxta_peer_get_adv(peer, &adv);
            if (err == JXTA_SUCCESS) {
                string = jxta_PA_get_Name(adv);
                JXTA_OBJECT_RELEASE(adv);
                sprintf(linebuff, "Name: [%s]\n", jstring_get_string(string));
                jstring_append_2(outputLine, linebuff);
                JXTA_OBJECT_RELEASE(string);
            }
            err = jxta_peer_get_peerid(peer, &pid);
            if (err == JXTA_SUCCESS) {
                jxta_id_to_jstring(pid, &string);
                JXTA_OBJECT_RELEASE(pid);
                sprintf(linebuff, "PeerId: [%s]\n", jstring_get_string(string));
                jstring_append_2(outputLine, linebuff);
                JXTA_OBJECT_RELEASE(string);
            }

            sprintf(linebuff, "Status: %s\n", connected ? "CONNECTED" : "NOT CONNECTED");
            jstring_append_2(outputLine, linebuff);
            expires = jxta_rdv_service_peer_get_expires(rdv, peer);

            if (connected && (expires >= 0)) {
                Jxta_time hours = 0;
                Jxta_time minutes = 0;
                Jxta_time seconds = 0;

                currentTime = jpr_time_now();
                if (expires < currentTime) {
                    expires = 0;
                } else {
                    expires -= currentTime;
                }

                seconds = expires / (1000);

                hours = (Jxta_time) (seconds / (Jxta_time) (60 * 60));
                seconds -= hours * 60 * 60;

                minutes = seconds / 60;
                seconds -= minutes * 60;

                /* This produces a compiler warning about L not being ansi.
                 * But long longs aren't ansi either. 
                 */
#ifndef WIN32

                sprintf(linebuff, "\nLease expires in %lld hour(s) %lld minute(s) %lld second(s)\n",
                        (Jxta_time) hours, (Jxta_time) minutes, (Jxta_time) seconds);
#else

                sprintf(linebuff, "\nLease expires in %I64d hour(s) %I64d minute(s) %I64d second(s)\n",
                        (Jxta_time) hours, (Jxta_time) minutes, (Jxta_time) seconds);
#endif

                jstring_append_2(outputLine, linebuff);
            }
            jstring_append_2(outputLine, "-----------------------------------------------------------------------------\n");
        }
        JXTA_OBJECT_RELEASE(peer);
    }
    JXTA_OBJECT_RELEASE(peers);

  Common_Exit:
    if (jstring_length(outputLine) > 0) {
        printf(jstring_get_string(outputLine));
    } else {
        printf(jstring_get_string(disconnectedMessage));
    }

    JXTA_OBJECT_RELEASE(outputLine);
    JXTA_OBJECT_RELEASE(disconnectedMessage);
    return res;
}

int main(int argc, char **argv)
{
    int a;
    Jxta_PG *pg;
    int peer_cnt, retry;
    Jxta_rdv_service *rdv;
    Jxta_log_file *f;
    Jxta_log_selector *s;

    Jxta_status rv;

    jxta_initialize();
    jxta_log_file_open(&f, "pg_test.log");
    s = jxta_log_selector_new_and_set("*.*", &rv);
    jxta_log_file_attach_selector(f, s, NULL);
    jxta_log_using(jxta_log_file_append, f);

    a = 1;
    while (a--) {
        /* Create and start peer group */
        jxta_PG_new_netpg(&pg);

        jxta_PG_get_rendezvous_service(pg, &rdv);
        peer_cnt = 0;

        for (retry = 3; 0 == peer_cnt && retry > 0; --retry) {
            printf("waiting for 3 seconds to see if RDV connects...\n");
            apr_sleep((Jxta_time) 3000000L);
            peer_cnt = display_peers(rdv);
        }

        JXTA_OBJECT_RELEASE(rdv);

        printf("Stopping PeerGroup ...\n");
        jxta_module_stop((Jxta_module *) pg);

        printf("PeerGroup Ref Count after stop :%d.\n", JXTA_OBJECT_GET_REFCOUNT(pg));
        JXTA_OBJECT_RELEASE(pg);
        pg = NULL;
    }

    jxta_log_using(NULL, NULL);
    jxta_log_file_close(f);
    jxta_log_selector_delete(s);

    jxta_terminate();
    return 0;
}

/* vim: set ts=4 sw=4 tw=130 et: */
