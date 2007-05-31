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
 * $Id: talk.c,v 1.12 2005/03/28 19:30:12 bondolo Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <apr_general.h>
#include "apr_time.h"

#include "jpr/jpr_thread.h"

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"

#include "jxta_shell_getopt.h"

#include "jxta_debug.h"
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_vector.h"
#include "jxta_pipe_service.h"
#include "jxta_hashtable.h"
#include "jxta_listener.h"
#include "jxta_dr.h"

#include "talk.h"

Jxta_listener *listener = NULL;

static const char *SENDERNAME = "JxtaTalkSenderName";
static const char *SENDERGROUPNAME = "GrpName";
static const char *SENDERMESSAGE = "JxtaTalkSenderMessage";
static const char *MYGROUPNAME = "NetPeerGroup";

#define DEFAULT_TIMEOUT (1 * 60 * 1000 * 1000)
#define TALK_ADVERTISEMENT_LIFETIME (Jxta_expiration_time) (24L* 60L * 60L * 1000L)
static Jxta_PG *group;
static JxtaShellEnvironment *environment;
static JString *env_type = NULL;
static Jxta_discovery_service *discovery = NULL;
static Jxta_pipe_service *pipe_service = NULL;
static JString *peerName = NULL;

static Jxta_hashtable *listeners = NULL;

static void message_listener(Jxta_object * obj, void *arg);
static void login_user(const char *userName);
static void create_user(const char *userName);
static void connect_to_user(const char *userName);
static void process_user_input(Jxta_outputpipe * pipe, const char *userName);

JxtaShellApplication *talk_new(Jxta_PG * pg,
                               Jxta_listener * standout,
                               JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{
    JxtaShellApplication *app = NULL;

    app = JxtaShellApplication_new(pg, standout, env, parent, terminate);

    if (app == NULL) {
        return NULL;
    }

    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      talk_print_help, talk_start, (shell_application_stdin) talk_process_input);
    group = pg;

    jxta_PG_get_discovery_service(group, &discovery);
    jxta_PG_get_pipe_service(group, &pipe_service);
    jxta_PG_get_peername(group, &peerName);

    listeners = jxta_hashtable_new(0);

    environment = env;
    env_type = jstring_new_2("PeerAdvertisement");
    return app;
}


void talk_process_input(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void talk_start(Jxta_object * appl, int argv, const char **arg)
{
    /*  JxtaShellApplication * app = (JxtaShellApplication*)appl; */
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, arg, "l:t:r:");
    JString *userName = NULL;

    while (TRUE) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

        if (type == -1)
            break;
        switch (type) {

        case 'l':
            userName = JxtaShellGetopt_getOptionArgument(opt);
            login_user(jstring_get_string(userName));
            JXTA_OBJECT_RELEASE(userName);
            break;

        case 't':
            userName = JxtaShellGetopt_getOptionArgument(opt);
            connect_to_user(jstring_get_string(userName));
            JXTA_OBJECT_RELEASE(userName);
            break;


        case 'r':
            userName = JxtaShellGetopt_getOptionArgument(opt);
            create_user(jstring_get_string(userName));
            JXTA_OBJECT_RELEASE(userName);
            break;

        case 'h':
            talk_print_help(appl);
            break;
        default:
            talk_print_help(appl);
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

void talk_print_help(Jxta_object * appl)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("talk - talk to another JXTA user \n\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "    talk [-t remote user]\n");
    jstring_append_2(inputLine, "           [-l local user] login for the local user\n");
    jstring_append_2(inputLine, "           [-r user name] register a new user\n");
    if (app != 0) {
        JXTA_OBJECT_SHARE(inputLine);
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}

static Jxta_pipe_adv *get_user_adv(char *userName, Jxta_time timeout)
{
    char *talkUserName = malloc(50 + strlen(userName));
    Jxta_listener *listener = jxta_listener_new(NULL, NULL, 1, 1);
    Jxta_status res;
    Jxta_DiscoveryResponseElement *element = NULL;
    Jxta_DiscoveryResponse *dr = NULL;
    Jxta_pipe_adv *adv = jxta_pipe_adv_new();
    Jxta_vector *responses = NULL;

    sprintf(talkUserName, "JxtaTalkUserName.%s", userName);

    /*
     * check first our local cache
     */
    discovery_service_get_local_advertisements(discovery, DISC_ADV, "Name", talkUserName, &responses);

    if (responses == NULL) {

        jxta_listener_start(listener);

        discovery_service_get_remote_advertisements(discovery,
                                                    NULL,
                                                    DISC_ADV, "Name", talkUserName, 1, (Jxta_discovery_listener *) listener);

        res = jxta_listener_wait_for_event(listener, timeout, (Jxta_object **) & dr);

        if (res != JXTA_SUCCESS) {
            printf("wait on listener failed\n");
            JXTA_OBJECT_RELEASE(listener);
            free(talkUserName);
            return NULL;
        }

        jxta_discovery_response_get_responses(dr, &responses);
        JXTA_OBJECT_RELEASE(dr);

        if (responses == NULL) {
            printf("No responses\n");
            JXTA_OBJECT_RELEASE(listener);
            free(talkUserName);
            return NULL;
        }

        res = jxta_vector_get_object_at(responses, (Jxta_object **) & element, 0);
        if (res != JXTA_SUCCESS) {
            printf("get response failed\n");
            JXTA_OBJECT_RELEASE(responses);
            JXTA_OBJECT_RELEASE(listener);
            free(talkUserName);
            return NULL;
        }

  /**
   ** Builds the advertisement.
   **/
        jxta_pipe_adv_parse_charbuffer(adv, jstring_get_string(element->response), strlen(jstring_get_string(element->response)));

        JXTA_OBJECT_RELEASE(element);

    } else {
        res = jxta_vector_get_object_at(responses, (Jxta_object **) & adv, 0);
        if (res != JXTA_SUCCESS) {
            printf("get local response failed\n");
            JXTA_OBJECT_RELEASE(responses);
            free(talkUserName);
            return NULL;
        }
    }

    JXTA_OBJECT_RELEASE(responses);
    free(talkUserName);

    return adv;
}

static void login_user(const char *userName)
{
    Jxta_pipe_adv *adv = NULL;
    Jxta_listener *listener = NULL;
    Jxta_pipe *pipe = NULL;
    Jxta_status res;
    Jxta_inputpipe *ip = NULL;

    adv = get_user_adv(userName, DEFAULT_TIMEOUT);

    if (adv != NULL) {
    /**
     ** Create a listener for the myJXTA messages
     **/
        listener = jxta_listener_new((Jxta_listener_func) message_listener, NULL, 1, 200);
        if (listener == NULL) {
            printf("Cannot create listener\n");
            return;
        }

    /**
     ** Get the pipe
     **/
        res = jxta_pipe_service_timed_accept(pipe_service, adv, 1000, &pipe);

        if (res != JXTA_SUCCESS) {
            printf("Cannot get pipe reason= %d\n", (int) res);
            return;
        }

        res = jxta_pipe_get_inputpipe(pipe, &ip);

        if (res != JXTA_SUCCESS) {
            printf("Cannot get inputpipe\n");
            return;
        }

        res = jxta_inputpipe_add_listener(ip, listener);

        if (res != JXTA_SUCCESS) {
            printf("Cannot add listener\n");
            return;
        }

        jxta_listener_start(listener);

        JXTA_OBJECT_RELEASE(adv);
    } else {
        printf("Cannot retrieve advertisement for %s\n", userName);
    }
}

static void create_user(const char *userName)
{
    Jxta_pipe_adv *adv = NULL;
    Jxta_id *pipeid = NULL;
    JString *tmpString = NULL;
    char *talkUserName = malloc(50 + strlen(userName));
    Jxta_status res;
    Jxta_id *gid = NULL;

    sprintf(talkUserName, "JxtaTalkUserName.%s", userName);

    jxta_PG_get_GID(group, &gid);

    jxta_id_pipeid_new_1(&pipeid, gid);

    JXTA_OBJECT_CHECK_VALID(pipeid);

    jxta_id_to_jstring(pipeid, &tmpString);

    JXTA_OBJECT_CHECK_VALID(tmpString);

    adv = jxta_pipe_adv_new();
    jxta_pipe_adv_set_Id(adv, jstring_get_string(tmpString));
    jxta_pipe_adv_set_Type(adv, JXTA_UNICAST_PIPE);
    jxta_pipe_adv_set_Name(adv, talkUserName);

    free(talkUserName);
    JXTA_OBJECT_RELEASE(tmpString);

    res = discovery_service_publish(discovery, adv, DISC_ADV, TALK_ADVERTISEMENT_LIFETIME, TALK_ADVERTISEMENT_LIFETIME);

    if (res != JXTA_SUCCESS) {
        printf("publish failed\n");
    }

    /*
     * wait for the SRDI index to be published
     */
    printf("(wait 20 sec for RDV SRDI indexing).\n");
    jpr_thread_delay(20 * 1000 * 1000);
    printf("done.\n");
}

static void connect_to_user(const char *userName)
{
    Jxta_pipe_adv *adv = NULL;
    char *talkUserName = malloc(50 + strlen(userName));
    Jxta_status res;
    Jxta_pipe *pipe = NULL;
    Jxta_outputpipe *op = NULL;

    sprintf(talkUserName, "JxtaTalkUserName.%s", userName);
    adv = get_user_adv(userName, DEFAULT_TIMEOUT);

    if (adv != NULL) {

        res = jxta_pipe_service_timed_connect(pipe_service, adv, 30 * 1000 * 1000, NULL, &pipe);

        if (res != JXTA_SUCCESS) {
            printf("Cannot get pipe for user %s\n", userName);
            return;
        }

        res = jxta_pipe_get_outputpipe(pipe, &op);

        if (res != JXTA_SUCCESS) {
            printf("Cannot get outputpipe for user %s\n", userName);
            return;
        }

        process_user_input(op, userName);
        JXTA_OBJECT_RELEASE(adv);
    } else {
        printf("Cannot retrieve advertisement for %s\n", userName);
    }

}

static void send_message(Jxta_outputpipe * op, const char *userName, const char *userMessage)
{
    Jxta_message *msg = jxta_message_new();
    Jxta_message_element *el = NULL;
    char *pt = NULL;
    Jxta_status res;

    /*
       JXTA_LOG ("Send message to [%s] msg=[%s]\n", userName, userMessage);
     */

    JXTA_OBJECT_CHECK_VALID(op);

    pt = jstring_get_string(peerName);

    el = jxta_message_element_new_1(SENDERNAME, "text/plain", pt, strlen(pt), NULL);

    jxta_message_add_element(msg, el);

    JXTA_OBJECT_RELEASE(el);


    pt = malloc(strlen(MYGROUPNAME) + 1);
    strcpy(pt, MYGROUPNAME);

    el = jxta_message_element_new_1(SENDERGROUPNAME, "text/plain", pt, strlen(pt), NULL);

    jxta_message_add_element(msg, el);

    JXTA_OBJECT_RELEASE(el);

    free(pt);

    el = jxta_message_element_new_1(SENDERMESSAGE, "text/plain", userMessage, strlen(userMessage), NULL);

    jxta_message_add_element(msg, el);

    JXTA_OBJECT_RELEASE(el);

    res = jxta_outputpipe_send(op, msg);
    if (res != JXTA_SUCCESS) {
        printf("sending failed: %d\n", (int) res);
    }

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_RELEASE(msg);
}


static void processIncomingMessage(Jxta_message * msg)
{
    JString *groupname = NULL;
    JString *senderName = NULL;
    JString *message = NULL;
    Jxta_message_element *el = NULL;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_message_get_element_1(msg, SENDERGROUPNAME, &el);

    if (el) {
        Jxta_bytevector *val = jxta_message_element_get_value(el);

        groupname = jstring_new_3(val);
        JXTA_OBJECT_RELEASE(val);
        val = NULL;
        JXTA_OBJECT_RELEASE(el);
    }

    el = NULL;
    jxta_message_get_element_1(msg, SENDERMESSAGE, &el);

    if (el) {
        Jxta_bytevector *val = jxta_message_element_get_value(el);
        message = jstring_new_3(val);
        JXTA_OBJECT_RELEASE(val);
        val = NULL;
        JXTA_OBJECT_RELEASE(el);
    }

    el = NULL;
    jxta_message_get_element_1(msg, SENDERNAME, &el);

    if (el) {
        Jxta_bytevector *val = jxta_message_element_get_value(el);
        senderName = jstring_new_3(val);
        JXTA_OBJECT_RELEASE(val);
        val = NULL;
        JXTA_OBJECT_RELEASE(el);
    }

    if (message) {
        printf("\n##############################################################\n");
        printf("CHAT MESSAGE from %s :\n", senderName == NULL ? "Unknown" : jstring_get_string(senderName));
        printf("%s\n", jstring_get_string(message));
        printf("##############################################################\n\n");
        fflush(stdout);
        JXTA_OBJECT_RELEASE(message);
    } else {
        printf("Received empty message\n");
    }
}

static void message_listener(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;

    JXTA_OBJECT_CHECK_VALID(msg);
    processIncomingMessage(msg);

    JXTA_OBJECT_RELEASE(msg);
}


static char *get_user_string(void)
{
    int capacity = 8192;
    char *pt = malloc(capacity);

    memset(pt, 0, 256);

    if (fgets(pt, 8192, stdin) != NULL) {
        if (strlen(pt) > 1) {
            pt[strlen(pt) - 1] = 0;     /* Strip off last /n */
            return pt;
        }
    }
    free(pt);
    return NULL;
}

static void process_user_input(Jxta_outputpipe * pipe, const char *userName)
{
    char *userMessage = NULL;

  /**
   ** Let every else start...
   **/
    printf("Welcome to JXTA-C Chat, %s\n", userName);
    printf("Type a '.' at begining of line to quit.\n");
    for (;;) {

        userMessage = get_user_string();
        if (userMessage != NULL)
            if (userMessage[0] == '.') {
                free(userMessage);
                break;
            }
        if (userMessage != NULL) {
            send_message(pipe, userName, userMessage);
        }
    }
}
