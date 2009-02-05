/*
 * Copyright (c) 2008 Sun Microsystems, Inc.  All rights reserved.
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

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"

#include "jxta_shell_getopt.h"

#include "monitor.h"

#include <apr_general.h>

#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_vector.h"
#include "jxta_hashtable.h"
#include "jxta_advertisement.h"
#include "jxta_rdv_service.h"
#include "jxta_monitor_service.h"
#include "jxta_peerview_monitor_entry.h"
#include "jxta_peerview_pong_msg.h"

#include "apr_time.h"


#define DEFAULT_TIMEOUT (1 * 60 * 1000 * 1000)

static void JXTA_STDCALL monitor_listener(Jxta_object * obj, void *arg);
static void register_listener();
static void unregister_listener();

static Jxta_PG *group;
static JxtaShellEnvironment *environment = NULL;
static Jxta_discovery_service *discovery = NULL;
static Jxta_rdv_service * rdv = NULL;
static JString *peerName = NULL;
static Jxta_boolean monitorON = FALSE;
static Jxta_boolean listener_registered = FALSE;
static Jxta_monitor_listener * mon_listener;

JxtaShellApplication *monitor_new(Jxta_PG * pg,
                                  Jxta_listener * standout,
                                  JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{

    JxtaShellApplication *app = NULL;

    app = JxtaShellApplication_new(pg, standout, env, parent, terminate);

    if (app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      (shell_application_printHelp) monitor_print_help,
                                      (shell_application_start) monitor_start, (shell_application_stdin) monitor_process_input);
    group = pg;

    jxta_PG_get_discovery_service(group, &discovery);
    jxta_PG_get_rendezvous_service(group, &rdv);
    jxta_PG_get_peername(group, &peerName);

    environment = env;
    return app;
}


void monitor_process_input(JxtaShellApplication * appl, JString * inputLine)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void monitor_start(JxtaShellApplication * appl, int argv, const char **arg)
{

    /*  JxtaShellApplication * app = (JxtaShellApplication*)appl; */
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, arg, (char *) "ht");
    Jxta_boolean toggle = FALSE;


    while (1) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

        if( type == -1) break;
        switch(type){
        case 't':
            monitorON = (monitorON == TRUE) ? FALSE:TRUE;
            toggle = TRUE;
            break;
        case 'h':
            monitor_print_help(appl);
            break;
        default:
            monitor_print_help(appl);
            error = 1;
            break;
        }
        if( error != 0) {
            break;
        }
    }

    if (toggle) {
        if (monitorON) {
            register_listener();
        } else {
            unregister_listener();
        }
    }
    if (discovery)
        JXTA_OBJECT_RELEASE(discovery);
    if (rdv)
        JXTA_OBJECT_RELEASE(rdv);

    if (peerName)
        JXTA_OBJECT_RELEASE(peerName);    
    JxtaShellGetopt_delete(opt);
    JxtaShellApplication_terminate(appl);
}

void monitor_print_help(JxtaShellApplication * appl)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("monitor - monitor service \n\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "    monitor [-h -t (toggle between listener)]\n");
    if (app != 0) {
        JXTA_OBJECT_SHARE(inputLine);
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}

static void register_listener()
{
    Jxta_monitor_service * mon_service;
    printf("Register listener\n");
    if (listener_registered) return;
    mon_service = jxta_monitor_service_get_instance();
    if (NULL != mon_service) {
        JString *env_type;
        JString *name;
        JxtaShellObject *sh_obj = NULL;

        name = jstring_new_2("MonListener1");
        jxta_monitor_service_listener_new(monitor_listener, NULL, NULL, NULL, &mon_listener);
        jxta_monitor_service_add_listener(mon_service, mon_listener);

        env_type = jstring_new_2("MonitorListener");
        sh_obj = JxtaShellObject_new(name, (Jxta_object *) mon_listener, env_type);

        if (NULL != sh_obj && NULL != environment) {
            JxtaShellEnvironment_add_0(environment, sh_obj);
        } else {
            printf("Can't create a shell object for the monitor listener??\n");
        }
        printf("Added the listener to the environment\n");

        JXTA_OBJECT_RELEASE(env_type);
        JXTA_OBJECT_RELEASE(sh_obj);
        JXTA_OBJECT_RELEASE(name);

        JXTA_OBJECT_RELEASE(mon_listener);
        listener_registered = TRUE;
    }
}

static void unregister_listener()
{
    printf("Unregister listener\n");

    if (listener_registered) {
        Jxta_monitor_service * service;
        service = jxta_monitor_service_get_instance();
        if (NULL != service && mon_listener) {
            jxta_monitor_service_remove_listener(service, mon_listener);
        }
    }
    listener_registered = FALSE;
}

/*************************************************************************************
                      Demo Listener

*/ 
static void JXTA_STDCALL monitor_listener(Jxta_object * obj, void *arg)
{
    Jxta_monitor_entry * entry = (Jxta_monitor_entry *) obj;
    char * context = NULL;
    char * sub_context = NULL;
    char * type = NULL;
    Jxta_advertisement * adv = NULL;
    JString *outputLine;

    outputLine = jstring_new_0();
    jxta_monitor_entry_get_context(entry, &context);
    jxta_monitor_entry_get_sub_context(entry, &sub_context);
    jxta_monitor_entry_get_type(entry, &type);
    jxta_monitor_entry_get_entry(entry, &adv);

    /* print the PV3 Monitor entry */
    if (0 == strcmp(type, "jxta:PV3MonEntry")) {
        Jxta_vector * partners;
        Jxta_vector * associates;
        Jxta_peerview_pong_msg * pong;
        unsigned int i;
        Jxta_id * id;
        JString * id_j;
        char tmp[256];

        jxta_peerview_monitor_entry_get_pong_msg((Jxta_peerview_monitor_entry *) adv, &pong);

        id = jxta_peerview_pong_msg_get_peer_id(pong);
        jxta_id_to_jstring(id, &id_j);
        jstring_append_2(outputLine, "\n");
        sprintf(tmp, "****** Peer - %s -- %d \n",jstring_get_string(id_j), jxta_peerview_monitor_entry_get_cluster_number((Jxta_peerview_monitor_entry *)adv));
        jstring_append_2(outputLine, tmp);

        JXTA_OBJECT_RELEASE(id);
        JXTA_OBJECT_RELEASE(id_j);

        associates = jxta_pong_msg_get_associate_infos(pong);
        if (jxta_vector_size(associates) > 0) {
            jstring_append_2(outputLine,"Associates:\n");
        }
        for (i =0; i<jxta_vector_size(associates); i++) {
            Jxta_peerview_peer_info * pinfo;

            jxta_vector_get_object_at(associates, JXTA_OBJECT_PPTR(&pinfo), i);
            id = jxta_peerview_peer_info_get_peer_id(pinfo);
            if (NULL != id) {
                jxta_id_to_jstring(id, &id_j);
                sprintf(tmp, "    %s\n",jstring_get_string(id_j));
                jstring_append_2(outputLine, tmp);
                JXTA_OBJECT_RELEASE(id);
                JXTA_OBJECT_RELEASE(id_j);
            }
            JXTA_OBJECT_RELEASE(pinfo);
        }

        JXTA_OBJECT_RELEASE(associates);
        partners = jxta_pong_msg_get_partner_infos(pong);
        if (jxta_vector_size(partners) > 0) {
            jstring_append_2(outputLine, "Partners:\n");
        }
        for (i =0; i<jxta_vector_size(partners); i++) {
            Jxta_peerview_peer_info * pinfo;

            jxta_vector_get_object_at(partners, JXTA_OBJECT_PPTR(&pinfo), i);
            id = jxta_peerview_peer_info_get_peer_id(pinfo);
            if (NULL != id) {
                jxta_id_to_jstring(id, &id_j);
                sprintf(tmp, "    %s\n",jstring_get_string(id_j));
                jstring_append_2(outputLine, tmp);
                JXTA_OBJECT_RELEASE(id);
                JXTA_OBJECT_RELEASE(id_j);
            }
            JXTA_OBJECT_RELEASE(pinfo);
        }
        JXTA_OBJECT_RELEASE(partners);

        JXTA_OBJECT_RELEASE(pong);
    } else {
        printf("Received type :%s\n", type);
    }


    if (context)
        free(context);
    if (sub_context)
        free(sub_context);
    if (type)
        free(type);
    if (adv)
        JXTA_OBJECT_RELEASE(adv);
    if (NULL != outputLine) {
        printf("%s", jstring_get_string(outputLine));
    }
    JXTA_OBJECT_RELEASE(outputLine);
    JXTA_OBJECT_RELEASE(entry);

}
