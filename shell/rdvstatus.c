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
 * $Id: rdvstatus.c,v 1.12 2005/04/07 02:38:27 bondolo Exp $
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <apr_time.h>
#include <apr_general.h>
#include <apr_thread_proc.h>

#include "jpr/jpr_thread.h"

#include "jxta.h"
#include "jxta_peergroup.h"

#include "jxta_shell_application.h"
#include "jxta_shell_getopt.h"
#include "rdvstatus.h"

#define WAIT_TIME (15 * 1000 * 1000) /* 15 seconds */


static Jxta_PG * group;
static JxtaShellEnvironment * environment;

JxtaShellApplication * jxta_rdvstatus_new(Jxta_PG * pg,
        Jxta_listener* standout,
        JxtaShellEnvironment *env,
        Jxta_object * parent,
        shell_application_terminate terminate) {

    JxtaShellApplication *app  =
        JxtaShellApplication_new(pg,standout,env,parent,terminate);
    if (app == NULL) {
        return NULL;
    }
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object*)app,
                                      jxta_rdvstatus_print_help,
                                      jxta_rdvstatus_start,
                                      (shell_application_stdin)jxta_rdvstatus_process_input);
    group = pg;
    environment = env;
    return app;
}

void  jxta_rdvstatus_process_input(Jxta_object * appl,
                                   JString * inputLine) {
    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JxtaShellApplication_terminate(app);
}

Jxta_boolean display_peers(Jxta_object * appl, Jxta_rdv_service* rdv) {
    Jxta_boolean res = TRUE;
    Jxta_peer* peer = NULL;
    Jxta_id* pid = NULL;
    Jxta_PA*      adv = NULL;
    JString*  string = NULL;
    Jxta_time expires = 0;
    Jxta_boolean connected = FALSE;
    Jxta_status err;
    Jxta_vector* peers = NULL;
    Jxta_time currentTime;
    int i;
    char  linebuff[1024];
    JString * outputLine = jstring_new_0();
    JString * disconnectedMessage = jstring_new_2("Status: NOT CONNECTED\n");

    /* Get the list of peers */
    err = jxta_rdv_service_get_peers(rdv, &peers);
    if (err != JXTA_SUCCESS) {
        jstring_append_2(outputLine, "Failed getting the peers.\n");
        res = FALSE;
        goto Common_Exit;
    }
    
    for (i = 0; i < jxta_vector_size(peers); ++i) {
        err = jxta_vector_get_object_at(peers,(Jxta_object**) &peer, i);
        if (err != JXTA_SUCCESS) {
            jstring_append_2(outputLine, "Failed getting a peer.\n");
            res = FALSE;
            goto Common_Exit;
        }

        connected = jxta_rdv_service_peer_is_connected(rdv, peer);
        if (connected) {
            err = jxta_peer_get_adv(peer, &adv);
            if ( (NULL != adv) && (err == JXTA_SUCCESS)) {
                string = jxta_PA_get_Name(adv);
                sprintf(linebuff, "Name: [%s]\n", jstring_get_string(string));
                jstring_append_2(outputLine, linebuff);
                JXTA_OBJECT_RELEASE(adv);
            }
            err = jxta_peer_get_peerid(peer, &pid);
            if ( (NULL != pid) && (err == JXTA_SUCCESS)) {
                jxta_id_to_jstring(pid, &string);
                sprintf(linebuff, "PeerId: [%s]\n", jstring_get_string(string));
                jstring_append_2(outputLine, linebuff);
                JXTA_OBJECT_RELEASE(string);
            }

            sprintf(linebuff, "Status: %s\n", connected ? "CONNECTED" : "NOT CONNECTED");
            jstring_append_2(outputLine, linebuff);
            expires = jxta_rdv_service_peer_get_expires(rdv, peer);

            if (connected && (expires > 0)) {
                Jxta_time_diff hours = 0;
                Jxta_time_diff minutes = 0;
                Jxta_time_diff seconds = 0;

                currentTime = jpr_time_now();
                if (expires < currentTime) {
                    expires = 0;
                } else {
                    expires -= currentTime;
                }

                seconds = expires / (1000);

                hours = (Jxta_time_diff) (seconds / (Jxta_time_diff) (60 * 60));
                seconds -= hours * 60 * 60;

                minutes = seconds / 60;
                seconds -= minutes * 60;


                /* This produces a compiler warning about L not being ansi.
                 * But long longs aren't ansi either. 
                 */
#ifndef WIN32

                sprintf(linebuff, "\nLease expires in %lld hour(s) %lld minute(s) %lld second(s)\n",
                         (Jxta_time_diff) hours,
                         (Jxta_time_diff) minutes,
                         (Jxta_time_diff) seconds);
#else

                sprintf(linebuff, "\nLease expires in %I64d hour(s) %I64d minute(s) %I64d second(s)\n",
                         (Jxta_time) hours,
                         (Jxta_time) minutes,
                         (Jxta_time) seconds);
#endif

                jstring_append_2(outputLine, linebuff);
            } else {
                sprintf(linebuff, "\nLease expired\n" );
                jstring_append_2(outputLine, linebuff);
            }
            jstring_append_2(outputLine, "-----------------------------------------------------------------------------\n");
        }
        JXTA_OBJECT_RELEASE(peer);
    }
    JXTA_OBJECT_RELEASE(peers);

Common_Exit:
    if(jstring_length(outputLine) > 0) {
        JxtaShellApplication_print((JxtaShellApplication *)appl, outputLine);
    } else {
        JxtaShellApplication_print((JxtaShellApplication *)appl, disconnectedMessage);
    }

    JXTA_OBJECT_RELEASE(outputLine);
    JXTA_OBJECT_RELEASE(disconnectedMessage);
    return TRUE;
}

void jxta_rdvstatus_print_help(Jxta_object *appl) {

    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JString * inputLine = jstring_new_2("#\trdvstatus - displays the connected state of rdvz.\n");
    jstring_append_2(inputLine,"SYNOPSIS\n");
    jstring_append_2(inputLine,"\trdvstatus\n\n");
    jstring_append_2(inputLine,"DESCRIPTION\n");
    jstring_append_2(inputLine,"\t'rdvstatus' allows you see what rendezvous you are connected to.\n\n");
    jstring_append_2(inputLine,"OPTIONS\n");
    jstring_append_2(inputLine,"\t-h\tthis help information. \n");

    if(app != NULL) {
        JXTA_OBJECT_SHARE(inputLine);
        JxtaShellApplication_print(app,inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}


void jxta_rdvstatus_start(Jxta_object * appl,
                          int argc,
                          char **argv) {

    Jxta_rdv_service* rdv = NULL;
    JString * outputLine = jstring_new_0();

    jxta_PG_get_rendezvous_service(group, &rdv);
    display_peers(appl, rdv);

Common_Exit:
    if(jstring_length(outputLine) > 0)
        JxtaShellApplication_print(appl, outputLine);

    JXTA_OBJECT_RELEASE(outputLine);
    JxtaShellApplication_terminate(appl);
}
