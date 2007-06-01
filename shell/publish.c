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
 * $Id: publish.c,v 1.5 2005/08/24 01:21:22 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"

#include "jxta_shell_getopt.h"

#include "publish.h"

#include <apr_general.h>

#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_vector.h"
#include "jxta_hashtable.h"
#include "jxta_test_adv.h"

#include "apr_time.h"


#define DEFAULT_TIMEOUT (1 * 60 * 1000 * 1000)

static Jxta_PG *group;
static JxtaShellEnvironment *environment;
static JString *env_type = NULL;
static Jxta_discovery_service *discovery = NULL;
static Jxta_pipe_service *pipe_service = NULL;
static JString *peerName = NULL;
static char *userName = NULL;
static Jxta_hashtable *listeners = NULL;

static void create_adv(char *userName);

JxtaShellApplication *publish_new(Jxta_PG * pg,
                                  Jxta_listener * standout,
                                  JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{

    JxtaShellApplication *app = NULL;

    app = JxtaShellApplication_new(pg, standout, env, parent, terminate);

    if (app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      publish_print_help, publish_start, (shell_application_stdin) publish_process_input);
    group = pg;

    jxta_PG_get_discovery_service(group, &discovery);
    jxta_PG_get_pipe_service(group, &pipe_service);
    jxta_PG_get_peername(group, &peerName);

    listeners = jxta_hashtable_new(0);

    environment = env;
    env_type = jstring_new_2("TestAdvertisement");
    return app;
}


void publish_process_input(Jxta_object * appl, JString * inputLine)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void publish_start(Jxta_object * appl, int argv, char **arg)
{

    /*  JxtaShellApplication * app = (JxtaShellApplication*)appl; */
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, arg, (char *) "hl:t:r:");
    JString *userName = NULL;

    while (1) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

        if (type == -1)
            break;
        switch (type) {

        case 'r':
            userName = JxtaShellGetopt_getOptionArgument(opt);
            create_adv((char *) jstring_get_string(userName));
            JXTA_OBJECT_RELEASE(userName);
            break;

        case 'h':
            publish_print_help(appl);
            break;
        default:
            publish_print_help(appl);
            error = 1;
            break;
        }
        if (error != 0) {
            break;
        }
    }

    JxtaShellGetopt_delete(opt);
    JxtaShellApplication_terminate(appl);
}

void publish_print_help(Jxta_object * appl)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("publish - publish to another JXTA user \n\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "    publish [-r Name value]\n");
    if (app != 0) {
        JXTA_OBJECT_SHARE(inputLine);
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}


static void create_adv(char *userName)
{

    Jxta_test_adv *adv = NULL;
    Jxta_id *pipeid = NULL;
    JString *tmpString = NULL;
    char *publishUserName = malloc(50 + strlen(userName));
    Jxta_status res;
    Jxta_id *gid = NULL;

    sprintf(publishUserName, "JxtaTestAdvertisement.%s", userName);

    jxta_PG_get_GID(group, &gid);

    jxta_id_pipeid_new_1(&pipeid, gid);

    JXTA_OBJECT_CHECK_VALID(pipeid);

    jxta_id_to_jstring(pipeid, &tmpString);

    JXTA_OBJECT_CHECK_VALID(tmpString);

    adv = jxta_test_adv_new();
    jxta_test_adv_set_Id(adv, (char *) jstring_get_string(tmpString));
    jxta_test_adv_set_IdAttr(adv, "this is from publish");
    jxta_test_adv_set_IdAttr1(adv, "this is the second attribute");
    jxta_test_adv_set_Type(adv, (char *) JXTA_UNICAST_PIPE);
    jxta_test_adv_set_Name(adv, publishUserName);
    jxta_test_adv_set_NameAttr1(adv, "this is from publish and is the attribute");
/*  jxta_test_adv_set_NameAttr2(adv, "this is for attribute 2 from the publish"); */

    free(publishUserName);
    JXTA_OBJECT_RELEASE(tmpString);

    res = discovery_service_publish(discovery, (Jxta_advertisement *) adv, DISC_ADV, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);

    if (res != JXTA_SUCCESS) {
        printf("publish failed\n");
    }


    printf("done.\n");
}
