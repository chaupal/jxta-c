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
 * $Id$
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <apr_time.h>
#include <apr_general.h>

#include "jxta.h"
#include "jxta_peergroup.h"
#include "jxta_peer.h"

#include "jxta_shell_application.h"
#include "jxta_shell_getopt.h"
#include "jxta_rdv_service_private.h"
#include "jxta_peerview_priv.h"
#include "jxta_rdv_lease_options.h"
#include "rdvstatus.h"

static const char *__log_cat = "rdvstatus";

void print_options(Jxta_peer * peer, JString *out_string);

static Jxta_PG *group;
static JxtaShellEnvironment *environment;

JxtaShellApplication *jxta_rdvstatus_new(Jxta_PG * pg,
                                         Jxta_listener * standout,
                                         JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{

    JxtaShellApplication *app = JxtaShellApplication_new(pg, standout, env, parent, terminate);
    if (app == NULL) {
        return NULL;
    }
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      jxta_rdvstatus_print_help,
                                      jxta_rdvstatus_start, (shell_application_stdin) jxta_rdvstatus_process_input);
    group = pg;
    environment = env;
    return app;
}

void jxta_rdvstatus_process_input(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}

void print_peerview_peer( JString *outputLine, Jxta_peer *peer )
{
   Jxta_status err;
   char linebuff[1024];
   Jxta_id *pid = NULL;
   Jxta_PA *adv = NULL;
   JString *string = NULL;
   Jxta_time expires = 0;
   Jxta_boolean connected = FALSE;
   Jxta_time currentTime = jpr_time_now();

   err = jxta_peer_get_peerid(peer, &pid);

   if ((NULL != pid) && (err == JXTA_SUCCESS)) {
       jxta_id_to_jstring(pid, &string);
       sprintf(linebuff, "%s", jstring_get_string(string));
       jstring_append_2(outputLine, linebuff);
       JXTA_OBJECT_RELEASE(string);
       JXTA_OBJECT_RELEASE(pid);
   }

   err = jxta_peer_get_adv(peer, &adv);

   if ((NULL != adv) && (err == JXTA_SUCCESS)) {
       string = jxta_PA_get_Name(adv);
       JXTA_OBJECT_RELEASE(adv);
   } else {
       string = jstring_new_2("(unknown)");
   }

   jstring_append_2(outputLine, "/");
   jstring_append_1(outputLine, string);
   JXTA_OBJECT_RELEASE(string);

   expires = jxta_peer_get_expires(peer);

   if (expires >= currentTime) {
       expires -= currentTime;

       sprintf(linebuff, "\t" JPR_ABS_TIME_FMT "s", expires / 1000 );
   } else {
       sprintf(linebuff, "\t(expired)");
   }

   jstring_append_2(outputLine, linebuff);
   print_options(peer, outputLine);

   jstring_append_2(outputLine, "\n");
}

Jxta_boolean display_peers(Jxta_object * appl, Jxta_rdv_service * rdv)
{
    Jxta_boolean res = TRUE;
    JString *outputLine = jstring_new_0();
    Jxta_status err;
    Jxta_vector *peers = NULL;
    unsigned int i;
    char linebuff[64];
    Jxta_time_diff time;
    Jxta_peerview *pv = jxta_rdv_service_get_peerview(rdv);
    Jxta_peer *selfPVE = NULL;

    RdvConfig_configuration config = jxta_rdv_service_config(rdv);
    Jxta_time_diff auto_interval = jxta_rdv_service_auto_interval(rdv);

    jstring_append_2(outputLine, "Auto-rdv check interval : ");

    if (auto_interval >= 0) {
        char linebuff[256];

        sprintf(linebuff, "\t" JPR_DIFF_TIME_FMT "ms", auto_interval);
        jstring_append_2(outputLine, linebuff);
        jstring_append_2(outputLine, "\n");
    } else {
        jstring_append_2(outputLine, "[disabled]\n");
    }

    if( NULL != pv ) {
        jxta_peerview_get_self_peer(pv, &selfPVE);
        sprintf(linebuff, "Cluster number:%i\n", jxta_peerview_get_cluster_number(pv));
        jstring_append_2(outputLine, linebuff);

        /* Get the list of peers */
        err = jxta_peerview_get_globalview(pv, &peers);
        if (err != JXTA_SUCCESS) {
            jstring_append_2(outputLine, "Failed getting the associate peers.\n");
            res = FALSE;
            goto Common_Exit;
        }

        jstring_append_2(outputLine, "\nAssociate Peers:\n");

        for (i = 0; i < jxta_vector_size(peers); ++i) {
            Jxta_peer *peer = NULL;

            err = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
            if (err != JXTA_SUCCESS) {
                jstring_append_2(outputLine, "Failed getting a peer.\n");
                res = FALSE;
                goto Common_Exit;
            }

            print_peerview_peer( outputLine, peer );

            JXTA_OBJECT_RELEASE(peer);
            }

        JXTA_OBJECT_RELEASE(peers);

        err = jxta_peerview_get_localview(pv, &peers);
        if (err != JXTA_SUCCESS) {
            jstring_append_2(outputLine, "Failed getting the partner peers.\n");
            res = FALSE;
            goto Common_Exit;
        }

        jstring_append_2(outputLine, "\nPartner Peers:\n");

        for (i = 0; i < jxta_vector_size(peers); ++i) {
            Jxta_peer *peer = NULL;
            char linebuff[1024];
            Jxta_id *pid = NULL;
            Jxta_PA *adv = NULL;
            JString *string = NULL;
            Jxta_time expires = 0;
            Jxta_boolean connected = FALSE;
            Jxta_time currentTime = jpr_time_now();

            err = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
            if (err != JXTA_SUCCESS) {
                jstring_append_2(outputLine, "Failed getting a peer.\n");
                res = FALSE;
                goto Common_Exit;
            }

            print_peerview_peer( outputLine, peer );

            JXTA_OBJECT_RELEASE(peer);
        }
        
        JXTA_OBJECT_RELEASE(peers);
    }
    jstring_append_2(outputLine, "\nConnections:\n");

    /* Get the list of peers */
    err = jxta_rdv_service_get_peers(rdv, &peers);
    if (err != JXTA_SUCCESS) {
        jstring_append_2(outputLine, "Failed getting the connected peers.\n");
        res = FALSE;
        goto Common_Exit;
    }

    for (i = 0; i < jxta_vector_size(peers); ++i) {
        Jxta_peer *peer = NULL;
        Jxta_id *pid = NULL;
        char linebuff[1024];
        Jxta_PA *adv = NULL;
        JString *string = NULL;
        Jxta_time expires = 0;
        Jxta_boolean connected = FALSE;
        Jxta_time currentTime;

        err = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
        if (err != JXTA_SUCCESS) {
            jstring_append_2(outputLine, "Failed getting a peer.\n");
            res = FALSE;
            goto Common_Exit;
        }

        err = jxta_peer_get_adv(peer, &adv);
        if ((NULL != adv) && (err == JXTA_SUCCESS)) {
            string = jxta_PA_get_Name(adv);
            sprintf(linebuff, "%s", jstring_get_string(string));
            jstring_append_2(outputLine, linebuff);
            JXTA_OBJECT_RELEASE(string);
            JXTA_OBJECT_RELEASE(adv);
        }
        err = jxta_peer_get_peerid(peer, &pid);
        if ((NULL != pid) && (err == JXTA_SUCCESS)) {
            jxta_id_to_jstring(pid, &string);
            sprintf(linebuff, "\t[%s]", jstring_get_string(string));
            jstring_append_2(outputLine, linebuff);
            JXTA_OBJECT_RELEASE(string);
            JXTA_OBJECT_RELEASE(pid);
        }

        expires = jxta_peer_get_expires(peer);

        currentTime = jpr_time_now();

        if (expires > currentTime) {
            Jxta_time_diff remaining = (Jxta_time_diff) (expires - currentTime);;
            Jxta_time_diff seconds = 0;

            seconds = remaining / (1000);

            sprintf(linebuff, "\tLease : " JPR_DIFF_TIME_FMT " second(s)", seconds);

        } else {
            sprintf(linebuff, "\tLease : expired");
        }
        jstring_append_2(outputLine, linebuff);

        print_options(peer, outputLine);
        jstring_append_2(outputLine, "\n");

        JXTA_OBJECT_RELEASE(peer);
    }
    
    JXTA_OBJECT_RELEASE(peers);
    if (pv)
        JXTA_OBJECT_RELEASE(pv);
    jstring_append_2(outputLine, "Rendezvous service config : ");

    switch (config) {
    case config_adhoc:
        jstring_append_2(outputLine, "ad-hoc\n");
        break;

    case config_edge:
        jstring_append_2(outputLine, "client\n");
        break;

    case config_rendezvous:
        jstring_append_2(outputLine, "rendezvous\n");
        break;

    default:
        jstring_append_2(outputLine, "[unknown]\n");
        break;
    }


    if (jxta_rdv_service_is_demoting(rdv)) {
        jstring_append_2(outputLine, "demoting\n");
    }
    time = jxta_rdv_service_get_running_time(rdv);
    apr_snprintf(linebuff, sizeof(linebuff), JPR_DIFF_TIME_FMT, time / 1000);
    jstring_append_2(outputLine, " running_time: ");
    jstring_append_2(outputLine, linebuff);
    jstring_append_2(outputLine, " s\n");

  Common_Exit:
    if (jstring_length(outputLine) > 0) {
        JxtaShellApplication_print((JxtaShellApplication *) appl, outputLine);
    }

    if (NULL != selfPVE) {
        JXTA_OBJECT_RELEASE(selfPVE);
    }

    JXTA_OBJECT_RELEASE(outputLine);
    return TRUE;
}

void jxta_rdvstatus_print_help(Jxta_object * appl)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("#\trdvstatus - displays the connected state of rendezvous.\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "\trdvstatus\n\n");
    jstring_append_2(inputLine, "DESCRIPTION\n");
    jstring_append_2(inputLine, "\t'rdvstatus' allows you see what rendezvous you are connected to.\n\n");
    jstring_append_2(inputLine, "OPTIONS\n");
    jstring_append_2(inputLine, "\t-h\tthis help information. \n");

    if (app != NULL) {
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}

void print_options(Jxta_peer * peer, JString *out_string)
{
    Jxta_vector *options;
    unsigned int i;
    jxta_peer_get_options(peer, &options);
    if (NULL == options) {
         return;
    }
       for (i=0; i < jxta_vector_size(options); i++) {
            Jxta_status res;
            char tmp[256];
            Jxta_rdv_lease_options * lease_option;
            res = jxta_vector_get_object_at(options, JXTA_OBJECT_PPTR(&lease_option), i);

            if (JXTA_SUCCESS != res) {
                continue;
            }
            if (NULL == lease_option) continue;
            if (strcmp(jxta_advertisement_get_document_name((Jxta_advertisement *) lease_option), "jxta:RdvLeaseOptions")) {
                JXTA_OBJECT_RELEASE(lease_option);
                lease_option = NULL;
                continue;
            }
            jstring_append_2(out_string, "\t");
            apr_snprintf(tmp, sizeof(tmp), "%d", jxta_rdv_lease_options_get_suitability(lease_option));
            jstring_append_2(out_string, " suit:");
            jstring_append_2(out_string, tmp);
            apr_snprintf(tmp, sizeof(tmp), "%d", jxta_rdv_lease_options_get_willingness(lease_option));
            jstring_append_2(out_string, " will:");
            jstring_append_2(out_string, tmp);
            JXTA_OBJECT_RELEASE(lease_option);
        }
        JXTA_OBJECT_RELEASE(options);
}

void jxta_rdvstatus_start(Jxta_object * appl, int argc, char **argv)
{

    Jxta_rdv_service *rdv = NULL;
    JString *outputLine = jstring_new_0();

    jxta_PG_get_rendezvous_service(group, &rdv);
    display_peers(appl, rdv);
    JXTA_OBJECT_RELEASE(rdv);

  Common_Exit:
    if (jstring_length(outputLine) > 0)
        JxtaShellApplication_print(appl, outputLine);

    JXTA_OBJECT_RELEASE(outputLine);
    JxtaShellApplication_terminate(appl);
}

/* vim: set ts=4 sw=4 et tw=130: */
