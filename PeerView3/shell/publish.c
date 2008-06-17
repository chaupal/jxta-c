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

#include "jxta_shell_getopt.h"

#include "publish.h"

#include <apr_general.h>

#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_vector.h"
#include "jxta_hashtable.h"
#include "jxta_advertisement.h"
#include "jxta_test_adv.h"
#include "jxta_range.h"

#include "apr_time.h"


#define DEFAULT_TIMEOUT (1 * 60 * 1000)

static Jxta_PG *group;
static JxtaShellEnvironment *environment;
static Jxta_discovery_service *discovery = NULL;
static Jxta_pipe_service *pipe_service = NULL;
static JString *peerName = NULL;

static void create_adv(const char *userName, Jxta_boolean remote, Jxta_boolean unique, long int timeout);

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
                                      (shell_application_printHelp) publish_print_help,
                                      (shell_application_start) publish_start, (shell_application_stdin) publish_process_input);
    group = pg;

    jxta_PG_get_discovery_service(group, &discovery);
    jxta_PG_get_pipe_service(group, &pipe_service);
    jxta_PG_get_peername(group, &peerName);

    environment = env;
    return app;
}


void publish_process_input(JxtaShellApplication * appl, JString * inputLine)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void publish_start(JxtaShellApplication * appl, int argv, const char **arg)
{

    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, arg, (char *) "hl:t:r:n:t:");
    JString *jUserName = NULL;
    Jxta_boolean publishRemote = FALSE;
    int multiple=1;
    long int timeout=DEFAULT_TIMEOUT;

    while (1) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

    if( type == -1) break;
    switch(type){
    
    case 'r':
      publishRemote = TRUE;
      jUserName = JxtaShellGetopt_getOptionArgument(opt);
      break;
    case 'h':
      publish_print_help(appl);
      break;
    case 'l':
      jUserName = JxtaShellGetopt_getOptionArgument(opt);
      break;
    case 'n':
      multiple = strtol(jstring_get_string(JxtaShellGetopt_OptionArgument(opt)), NULL, 0);  
      multiple++;
      break;
    case 't':
      timeout = strtol(jstring_get_string(JxtaShellGetopt_OptionArgument(opt)), NULL, 0);
      timeout *= 1000;
      break;
    default:
      publish_print_help(appl);
      error = 1;
      break;
    }
    if( error != 0) {
      break;
    }
    }
    if (NULL != jUserName) {
        JString *jFullName=NULL;
        int i;
        jFullName = jstring_new_0();
        for (i=0; i<multiple; i++) {
            char buf[64];
            jstring_reset(jFullName, NULL);
            jstring_append_1(jFullName, jUserName);
            if (i > 0) {
                memset(buf, 0, 64);
                apr_snprintf(buf, 64, "%d", i);
                jstring_append_2(jFullName, buf);
            }
            create_adv (jstring_get_string(jFullName), publishRemote, i==0 ? TRUE:FALSE ,timeout);
        }
        JXTA_OBJECT_RELEASE(jFullName);
        JXTA_OBJECT_RELEASE(jUserName);
    }
    if (discovery)
        JXTA_OBJECT_RELEASE(discovery);
    if (pipe_service)
        JXTA_OBJECT_RELEASE(pipe_service);
    if (peerName)
        JXTA_OBJECT_RELEASE(peerName);    
    JxtaShellGetopt_delete(opt);
    JxtaShellApplication_terminate(appl);
}

void publish_print_help(JxtaShellApplication * appl)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("publish - publish to another JXTA user \n\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "    publish [-r Name -l Name -n number -t timeout]\n");
    if (app != 0) {
        JXTA_OBJECT_SHARE(inputLine);
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}


static void create_adv(const char *userName, Jxta_boolean remote, Jxta_boolean unique, long int timeout)
{

    Jxta_test_adv *adv = NULL;
    Jxta_id *pipeid = NULL;
    JString *tmpString = NULL;
    char *publishUserName = calloc(1, 50 + strlen(userName));
    Jxta_status res;
    Jxta_id *gid = NULL;
    Jxta_vector *ranges=NULL;

    sprintf(publishUserName, "JxtaTest%sAdvertisement", userName);

    jxta_PG_get_GID(group, &gid);
    if (unique) {
        jxta_id_pipeid_new_1(&pipeid, gid);
    } else {
        jxta_id_pipeid_new_2(&pipeid, gid, (unsigned char const *) publishUserName, strlen(publishUserName));
    }
    JXTA_OBJECT_RELEASE(gid);
    JXTA_OBJECT_CHECK_VALID(pipeid);
    jxta_id_to_jstring(pipeid, &tmpString);
    JXTA_OBJECT_RELEASE(pipeid);
    JXTA_OBJECT_CHECK_VALID(tmpString);

    adv = jxta_test_adv_new(); 
    jxta_test_adv_set_Id (adv, (char*) jstring_get_string (tmpString));
    jxta_test_adv_set_IdAttr(adv, "#200.35", "(-100 :: 400)");
    jxta_test_adv_set_IdAttr1(adv, "this is the second attribute");
    jxta_test_adv_set_Type (adv, (char*) JXTA_UNICAST_PIPE);
    jxta_test_adv_set_Name (adv, publishUserName);
    jxta_test_adv_set_NameAttr1(adv, "this is from publish and is the attribute");
    jxta_test_adv_set_GenericNumeric(adv, "#600", "(300 :: 900)");
    jxta_test_adv_set_GenericNumeric(adv, "#600", NULL);

    free(publishUserName);
    JXTA_OBJECT_RELEASE(tmpString);
    if (remote)  {
        res = discovery_service_remote_publish(discovery,
                                  NULL,
                                  (Jxta_advertisement *) adv,
                                  DISC_ADV,
                                  DEFAULT_TIMEOUT);

    } else {
        res = discovery_service_publish(discovery,
                                  (Jxta_advertisement *) adv,
                                  DISC_ADV,
                                  timeout,
                                  timeout);
    }
    /* res = discovery_service_publish(discovery, (Jxta_advertisement *) adv, DISC_ADV, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT); */

    if (res != JXTA_SUCCESS) {
      printf ("publish failed\n");
    } else {
      printf ("Publish Complete\n");
    }
  
    /* ranges = jxta_test_adv_get_ranges(adv); */
    ranges = jxta_range_get_ranges();
    if (NULL != ranges) {
        unsigned int i;
        for (i=0; i < jxta_vector_size(ranges); i++) {
            Jxta_range* rge;
            if (JXTA_SUCCESS == jxta_vector_get_object_at(ranges, JXTA_OBJECT_PPTR(&rge), i )) {
                printf ("Range recorded %s %s %s low: %f high:%f \n"
                        , jxta_range_get_name_space(rge)
                        , jxta_range_get_element(rge)
                        , jxta_range_get_attribute(rge)
                        , jxta_range_get_low(rge)
                        , jxta_range_get_high(rge) );
                JXTA_OBJECT_RELEASE(rge);
            }
        }
        JXTA_OBJECT_RELEASE(ranges);
    }
    JXTA_OBJECT_RELEASE(adv);
    printf("done.\n");
}
