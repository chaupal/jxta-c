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
#include "jxta_rdv_config_adv.h"

#include "jxta_shell_application.h"
#include "jxta_shell_getopt.h"
#include "jxta_rdv_service_private.h"
#include "rdvcontrol.h"

static Jxta_PG *group;
static JxtaShellEnvironment *environment;

JxtaShellApplication *jxta_rdvcontrol_new(Jxta_PG * pg,
                                          Jxta_listener * standout,
                                          JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{

    JxtaShellApplication *app = JxtaShellApplication_new(pg, standout, env, parent, terminate);
    if (app == NULL) {
        return NULL;
    }
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      jxta_rdvcontrol_print_help,
                                      jxta_rdvcontrol_start, (shell_application_stdin) jxta_rdvcontrol_process_input);
    group = pg;
    environment = env;
    return app;
}

void jxta_rdvcontrol_process_input(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}

void jxta_rdvcontrol_print_help(Jxta_object * appl)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("#\trdvcontrol - Controls behaviour of the rendezvous service\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "\trdvcontrol\n\n");
    jstring_append_2(inputLine, "DESCRIPTION\n");
    jstring_append_2(inputLine, "\t'rdvcontrol' allows you to control behaviour of the rendezvous service.\n\n");
    jstring_append_2(inputLine, "OPTIONS\n");
    jstring_append_2(inputLine, "\t-c\tSwitch to client mode.\n");
    jstring_append_2(inputLine, "\t-r\tSwitch to rendezvous mode.\n");
    jstring_append_2(inputLine, "\t-C <interval>\tSwitch to auto-rendezvous client mode.\n");
    jstring_append_2(inputLine, "\t-R <interval>\tSwitch to auto-rendezvous rendezvous mode.\n");

    if (app != NULL) {
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}


void jxta_rdvcontrol_start(Jxta_object * appl, int argc, const char **argv)
{
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argc, argv, "cC:rR:");
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *outputLine = jstring_new_0();
    RdvConfig_configuration config = config_adhoc;
    JString *interval = NULL;
    Jxta_time_diff auto_interval = -1;
    Jxta_rdv_service *rdv;

    while (1) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

        if (type == -1)
            break;
        switch (type) {
        case 'c':
            config = config_edge;
            break;

        case 'C':
            config = config_edge;
            interval = JxtaShellGetopt_getOptionArgument(opt);
            auto_interval = atol(jstring_get_string(interval));
            JXTA_OBJECT_RELEASE(interval);
            break;

        case 'r':
            config = config_rendezvous;
            break;

        case 'R':
            config = config_rendezvous;
            interval = JxtaShellGetopt_getOptionArgument(opt);
            auto_interval = atol(jstring_get_string(interval));
            JXTA_OBJECT_RELEASE(interval);
            break;

        default:
            jxta_rdvcontrol_print_help(appl);
            error = 1;
            break;
        }
        if (error != 0)
            goto Common_Exit;
    }

    JxtaShellGetopt_delete(opt);

    jxta_PG_get_rendezvous_service(group, &rdv);

    jxta_rdv_service_set_auto_interval(rdv, -1);

    jxta_rdv_service_set_config(rdv, config);

    if (-1 != auto_interval) {
        jxta_rdv_service_set_auto_interval(rdv, auto_interval);
    }

    JXTA_OBJECT_RELEASE(rdv);

  Common_Exit:
    if (jstring_length(outputLine) > 0)
        JxtaShellApplication_print(app, outputLine);
    JXTA_OBJECT_RELEASE(outputLine);

    JxtaShellApplication_terminate(app);

}

/* vim: set ts=4 sw=4 et tw=130: */
