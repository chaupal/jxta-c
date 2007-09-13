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
 * $Id$
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "peers.h"

#include "jxta_shell_getopt.h"

static Jxta_PG *group;
static JxtaShellEnvironment *environment;

JxtaShellApplication *jxta_peers_new(Jxta_PG * pg,
                                     Jxta_listener * standout,
                                     JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{
    JxtaShellApplication *app = JxtaShellApplication_new(pg, standout, env, parent, terminate);
    if (app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      jxta_peers_print_help,
                                      jxta_peers_start, (shell_application_stdin) jxta_peers_process_input);
    group = pg;
    environment = env;
    return app;
}


void jxta_peers_process_input(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void jxta_peers_start(Jxta_object * appl, int argv, char **arg)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, arg, "rfhp:n:a:v:");
    JString *pid = jstring_new_0();
    JString *attr = jstring_new_0();
    JString *value = jstring_new_0();
    Jxta_boolean rf = FALSE;
    Jxta_boolean pf = FALSE;
    Jxta_boolean ff = FALSE;
    Jxta_discovery_service *discovery;
    long qid = -1;
    long responses = 10;
    Jxta_vector *res_vec = NULL;
    int error = 0;

    while (1) {
        JString *op_val = NULL;
        int type = JxtaShellGetopt_getNext(opt);

        if (type == -1)
            break;
        switch (type) {
        case 'r':
            rf = TRUE;
            break;
        case 'f':
            ff = TRUE;
            break;
        case 'p':
            pf = TRUE;
            op_val = JxtaShellGetopt_getOptionArgument(opt);
            jstring_append_1(pid, op_val);
            JXTA_OBJECT_RELEASE(op_val);
            break;
        case 'n':
            op_val = JxtaShellGetopt_getOptionArgument(opt);
            responses = strtol(jstring_get_string(op_val), NULL, 0);
            JXTA_OBJECT_RELEASE(op_val);
            break;
        case 'a':
            op_val = JxtaShellGetopt_getOptionArgument(opt);
            jstring_append_1(attr, op_val);
            JXTA_OBJECT_RELEASE(op_val);
            break;
        case 'v':
            op_val = JxtaShellGetopt_getOptionArgument(opt);
            jstring_append_1(value, op_val);
            JXTA_OBJECT_RELEASE(op_val);
            break;
        case 'h':
            jxta_peers_print_help(appl);
            error = 1;
            break;
        default:
            jxta_peers_print_help(appl);
            error = 1;
            break;
        }
        if (error != 0)
            break;
    }

    JxtaShellGetopt_delete(opt);
    jxta_PG_get_discovery_service(group, &discovery);
    if (ff) {
        Jxta_PA * pa = NULL;
		JString *tmp_str = NULL;
        discovery_service_flush_advertisements(discovery, NULL, DISC_PEER);
        tmp_str = jstring_new_2("PeerAdvertisement");
        JxtaShellEnvironment_delete_type(environment, tmp_str);

        /* publish local peer advertisement */
        jxta_PG_get_PA(group, &pa);
        discovery_service_publish(discovery, pa, DISC_PEER, DEFAULT_LIFETIME, DEFAULT_EXPIRATION);

        JxtaShellApplication_terminate(app);
        JXTA_OBJECT_RELEASE(tmp_str);
        JXTA_OBJECT_RELEASE(pid);
        JXTA_OBJECT_RELEASE(attr);
        JXTA_OBJECT_RELEASE(value);
        return;
    }
    if (rf) {
        char *paddr = NULL;
        if (pf) {
            paddr = (char *) jstring_get_string(pid);
        }
        qid = discovery_service_get_remote_advertisements(discovery,
                                                          paddr,
                                                          DISC_PEER,
                                                          (char *) jstring_get_string(attr),
                                                          (char *) jstring_get_string(value), responses, NULL);
    } else if (error == 0) {
        discovery_service_get_local_advertisements(discovery,
                                                   DISC_PEER,
                                                   (char *) jstring_get_string(attr),
                                                   (char *) jstring_get_string(value), &res_vec);
        if (res_vec != NULL) {
            int i;
            Jxta_PA *padv;
            char buf[64];
            JString *name = NULL;
            JxtaShellObject *sh_obj;
            JString *env_type;

            printf("restored %d peer advertisement(s) \n", jxta_vector_size(res_vec));
            for (i = 0; i < jxta_vector_size(res_vec); i++) {
                /* Create the shell object for the peer */
                sprintf(buf, "peer%d", i);
                name = jstring_new_2(buf);
                jxta_vector_get_object_at(res_vec, JXTA_OBJECT_PPTR(&padv), i);
                env_type = jstring_new_2("PeerAdvertisement");
                sh_obj = JxtaShellObject_new(name, (Jxta_object *) padv, env_type);
                JXTA_OBJECT_RELEASE(env_type);
                JxtaShellEnvironment_add_0(environment, sh_obj);
                JXTA_OBJECT_RELEASE(sh_obj);
                JXTA_OBJECT_RELEASE(name);
                JXTA_OBJECT_RELEASE(padv);

                /** And print the information to the screen */
                name = jxta_PA_get_Name(padv);
                printf("%d: %s\n", i, jstring_get_string(name));
                JXTA_OBJECT_RELEASE(name);
            }
            JXTA_OBJECT_RELEASE(res_vec);
        } else {
            printf("No peer advertisements retrieved\n");
        }
    }

    JXTA_OBJECT_RELEASE(discovery);

    JxtaShellApplication_terminate(app);
    JXTA_OBJECT_RELEASE(pid);
    JXTA_OBJECT_RELEASE(attr);
    JXTA_OBJECT_RELEASE(value);
}

void jxta_peers_print_help(Jxta_object * appl)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("peers - discover peers \n\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "    peers [-p peerid name attribute]\n");
    jstring_append_2(inputLine, "           [-n n] limit the number of responses to n from a single peer\n");
    jstring_append_2(inputLine, "           [-r] discovers peer groups using remote propagation\n");
    jstring_append_2(inputLine, "           [-a] specify Attribute name to limit discovery to\n");
    jstring_append_2(inputLine, "           [-v] specify Attribute value to limit discovery to. wild card is allowed\n");
    jstring_append_2(inputLine, "           [-f] flush peer advertisements\n");
    jstring_append_2(inputLine, "           [-h] print this information\n");
    if (app != 0) {
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}

/* vi: set ts=4 sw=4 tw=130 et: */
