/*
 * Copyright (c) 2004 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: whisp.c,v 1.12 2005/11/15 18:41:35 slowhog Exp $
 */

/*
 *  whisp.c
 *  jxta tutorials
 *
 *  Created by Brent Gulanowski on 11/2/04.
 *
 */


#include <unistd.h>             /* usleep() */
#include <assert.h>             /* assert() */
#include <time.h>               /* ctime(), time() */
#include <string.h>

#include <jxta_dr.h>
#include <jxta_pipe_service.h>

#include "jxta_tutorial_shared.h"
#include "jxta_tutorial_args.h"


#define THIRTY_SECONDS_IN_uS    30 * 1000 * 1000

#define ONE_HOUR_IN_MS    1000 * 60 * 60
#define ONE_DAY_IN_MS    ONE_HOUR_IN_MS * 24
#define ONE_WEEK_IN_MS    ONE_DAY_IN_MS * 7

#define BUF_SIZE    256
#define MAX_PEERS    10
#define PAUSE_DURATION    10000
#define MAX_PIPE_NAME_LENGTH    50

#define PIPE_CONNECT_TIMEOUT    THIRTY_SECONDS_IN_uS
#define GROUP_LIFETIME    ONE_HOUR_IN_MS * 2
#define GROUP_LIFETIME_IN_WILD    ONE_HOUR_IN_MS

#define MESSAGE_PIPE_LIFETIME    ONE_HOUR_IN_MS


#define CREATE_PGA_THEN_PG TRUE

#define USE_AD_PARSING_BUG_WORKAROUND TRUE


typedef enum {
    USER_ERROR,
    USER_SEARCH,
    USER_PEERS,
    USER_GROUP,
    USER_GROUPS,
    USER_JOIN,
    USER_LEAVE,
    USER_SEND,
    USER_NICK,
    USER_STATUS,
    USER_HELP,
    USER_QUIT
} userOption;


typedef struct {
    Jxta_pipe_adv *recipientPipeAd;
    JString *message;
} messageContext;


/** file prototypes **/
void start_npg(void);
void listen_on_message_pipe(void);
void shut_down_message_pipe(void);
unsigned read_user_input(JString * operands[2]);

/* functions for user commands */
void user_search(void);
void user_peers(void);
void user_groups(void);
void user_new_group(JString * groupName);
void user_join_subgroup(JString * groupName);
void user_leave_subgroup(void);
void user_send_message(JString * recipient, JString * message);
void user_nickname(JString * newNick);
void user_status(void);
void user_help(void);

/* group-related helper functions */
JString *qualified_subgroup_name(JString * baseName);
JString *new_subgroup_description(void);
Jxta_PGA *new_subgroup_advertisement(JString * groupName);
Jxta_PGA *find_subgroup_advertisement(JString * groupName);
Jxta_PG *create_new_subgroup(JString * groupName);

void print_greeting(void);
void print_prompt(void);
void update_group_services(void);
int index_of_pipe_ad_with_name(JString * recipient);

/* callbacks */
void rdv_event_received(Jxta_object * obj, void *arg);
void discovery_response_received(Jxta_object * obj, void *arg);
void pipe_accept_event_received(Jxta_object * obj, void *arg);
void message_received(Jxta_object * obj, void *arg);
void pipe_connect_event_received(Jxta_object * obj, void *arg);


/** file variables **/

Jxta_PG *netPeerGroup;
#if CREATE_PGA_THEN_PG
Jxta_id *npgMSID;
#endif

Jxta_PG *currentGroup;
/* cached service objects */
Jxta_pipe_service *pipeSvc;
Jxta_discovery_service *discoSvc;
Jxta_rdv_service *rdvSvc;

Jxta_peer *rdv;
Jxta_vector *pipeAds;
Jxta_vector *groupAds;

Jxta_pipe_adv *messagePipeAd;
Jxta_pipe *messagePipe;
Jxta_inputpipe *inPipe;

Jxta_listener *rdvListener;
Jxta_listener *discoListener;
Jxta_listener *pipeEventListener;
Jxta_listener *messageListener;
Jxta_listener *connectionListener;

JString *nickname;
JString *message;

char buffer[BUF_SIZE];


/* UNIX null-terminated table of environment variables (see `man environ`) */
extern char **environ;


/** tutorial main hook functions **/

Jxta_boolean process_args(int argc, const char *argv[])
{

    arg **args = NULL;
    unsigned i;

    unsigned count = arg_parse_launch_args(argc, argv, &args);
    if (count < 1) {
        char **env = environ;
        while (env != NULL && 0 != strncmp(*env, "USER=", 5))
            ++env;
        strncpy(buffer, (*env) + 5, BUF_SIZE);
    } else {
        for (i = 0; i < count; i++) {
            switch (arg_name(args[i])) {
            case 'n':
                /* assign the nickname */
                if (arg_operand_count(args[i]) > 0) {
                    strncpy(buffer, arg_operand_at_index(args[i], 0), BUF_SIZE);
                }
                break;
            default:
                break;
            }
        }
    }

    return TRUE;
}

Jxta_boolean init(void)
{

    /* initialize the library */
    jxta_initialize();

    netPeerGroup = NULL;
    pipeSvc = NULL;
    discoSvc = NULL;
    rdvSvc = NULL;
    rdv = NULL;

    messagePipeAd = NULL;
    messagePipe = NULL;
    inPipe = NULL;

    rdvListener = jxta_listener_new(rdv_event_received, NULL, 1, 5);
    assert(rdvListener);
    jxta_listener_start(rdvListener);

    discoListener = jxta_listener_new(discovery_response_received, NULL, 1, 5);
    assert(discoListener);
    jxta_listener_start(discoListener);

    pipeEventListener = jxta_listener_new(pipe_accept_event_received, NULL, 1, 5);
    assert(pipeEventListener);
    jxta_listener_start(pipeEventListener);

    messageListener = jxta_listener_new(message_received, NULL, 1, 5);
    assert(messageListener);
    jxta_listener_start(messageListener);

    nickname = jstring_new_2(buffer);

    return TRUE;
}

int run(void)
{

    JString *operands[2] = {
        NULL, NULL
    };

    start_npg();

    print_greeting();
    user_help();

    /* loop and process user input */
    while (1) {
        switch (read_user_input(operands)) {
        case USER_SEARCH:
            user_search();
            break;
        case USER_PEERS:
            user_peers();
            break;
        case USER_GROUP:
            user_new_group(operands[0]);
            break;
        case USER_GROUPS:
            user_groups();
            break;
        case USER_JOIN:
            user_join_subgroup(operands[0]);
            break;
        case USER_LEAVE:
            user_leave_subgroup();
            break;
        case USER_SEND:
            user_send_message(operands[0], operands[1]);
            break;
        case USER_NICK:
            user_nickname(operands[0]);
            break;
        case USER_STATUS:
            user_status();
            break;
        case USER_HELP:
            user_help();
            break;
        case USER_QUIT:
            return 0;
            break;
        default:
            if (strlen(buffer) > 0) {
                printf("  The command `%s` was not understood\n", buffer);
                user_help();
            }
            break;
        }

        if (operands[0]) {
            JXTA_OBJECT_RELEASE(operands[0]);
            operands[0] = NULL;
        }
        if (operands[1]) {
            JXTA_OBJECT_RELEASE(operands[1]);
            operands[1] = NULL;
        }

        usleep(PAUSE_DURATION);
    }

    return 0;
}

void cleanup(void)
{
    if (netPeerGroup)
        JXTA_OBJECT_RELEASE(netPeerGroup);
    if (pipeSvc)
        JXTA_OBJECT_RELEASE(pipeSvc);
    if (discoSvc)
        JXTA_OBJECT_RELEASE(discoSvc);
    if (rdvSvc)
        JXTA_OBJECT_RELEASE(rdvSvc);

    if (messagePipeAd)
        JXTA_OBJECT_RELEASE(messagePipeAd);

    jxta_terminate();
}


/** file functions **/

void start_npg(void)
{

#if CREATE_PGA_THEN_PG
    Jxta_PGA *npgAd = NULL;
#endif
    Jxta_status status;

    /* retrieve necessary services */
    status = jxta_PG_new_netpg(&netPeerGroup);
    assert(status == JXTA_SUCCESS);
    currentGroup = netPeerGroup;

    update_group_services();

#if CREATE_PGA_THEN_PG
    /* get the MSID from the advertisement -- for use creating new groups */
    jxta_PG_get_PGA(netPeerGroup, &npgAd);
    assert(npgAd);
    npgMSID = jxta_PGA_get_MSID(npgAd);
    assert(npgMSID);
#endif
}

void find_or_create_message_pipe(void)
{

    char pipeName[MAX_PIPE_NAME_LENGTH];
    Jxta_vector *pipeAds = NULL;
    Jxta_id *groupId = NULL;
    JString *groupIdString = NULL;
    Jxta_id *pipeId = NULL;
    Jxta_status status;

    assert(nickname);
    snprintf(pipeName, MAX_PIPE_NAME_LENGTH, "whisp.%s", jstring_get_string(nickname));

    /* try to find our pipe ad in the advertisement cache */
    status = discovery_service_get_local_advertisements(discoSvc, DISC_ADV, "Name", pipeName, &pipeAds);
    assert(status == JXTA_SUCCESS);
    if (pipeAds && jxta_vector_size(pipeAds) > 0) {
        status = jxta_vector_get_object_at(pipeAds, JXTA_OBJECT_PPTR(&messagePipeAd), 0);
        pipe_advertisement_print_description(stdout, messagePipeAd);
        JXTA_OBJECT_RELEASE(pipeAds);
    } else {
        /* create a pipe ad */
        messagePipeAd = jxta_pipe_adv_new();
        assert(messagePipeAd);

        jxta_PG_get_GID(netPeerGroup, &groupId);
        status = jxta_id_pipeid_new_1(&pipeId, groupId);
        assert(status == JXTA_SUCCESS);
        status = jxta_id_to_jstring(pipeId, &groupIdString);
        assert(status == JXTA_SUCCESS);

        status = jxta_pipe_adv_set_Id(messagePipeAd, (char *) jstring_get_string(groupIdString));
        assert(status == JXTA_SUCCESS);
        status = jxta_pipe_adv_set_Name(messagePipeAd, pipeName);
        assert(status == JXTA_SUCCESS);
        status = jxta_pipe_adv_set_Type(messagePipeAd, JXTA_UNICAST_PIPE);

        pipe_advertisement_print_description(stdout, messagePipeAd);

        JXTA_OBJECT_RELEASE(pipeId);
        JXTA_OBJECT_RELEASE(groupId);
        JXTA_OBJECT_RELEASE(groupIdString);
    }
}

void listen_on_message_pipe(void)
{

    Jxta_status status;

    if (NULL == messagePipeAd) {
        find_or_create_message_pipe();
    }

    /* publish the pipe ad */
    status =
        discovery_service_remote_publish(discoSvc, NULL, (Jxta_advertisement *) messagePipeAd, DISC_ADV, MESSAGE_PIPE_LIFETIME);
#if 0
    status = discovery_service_publish(discoSvc, (Jxta_advertisement *)messagePipeAd, DISC_ADV, MESSAGE_PIPE_LIFETIME, MESSAGE_PIPE_LIFETIME);
#endif    
    assert(status == JXTA_SUCCESS);

    /* prepare the listener to respond to messages sent to us */
    status = jxta_pipe_service_add_accept_listener(pipeSvc, messagePipeAd, pipeEventListener);
    assert(status == JXTA_SUCCESS);
}

void shut_down_message_pipe(void)
{
    /* FIXME this is not implemented yet */
#if 0
    Jxta_status status = jxta_pipe_service_remove_accept_listener(pipeSvc, messagePipeAd, pipeEventListener);
    assert(status == JXTA_SUCCESS);
#endif

    /* just free the pipe and input pipe objects */
    if (inPipe) {
        JXTA_OBJECT_RELEASE(inPipe);
        inPipe = NULL;
    }
    if (messagePipe) {
        JXTA_OBJECT_RELEASE(messagePipe);
        messagePipe = NULL;
    }
}


unsigned read_user_input(JString * operands[2])
{

    unsigned result = USER_ERROR;
    char *tokens[3] = {
        NULL, NULL, NULL
    };
    char **last;
    char *rest;
    unsigned length;

    print_prompt();
    fgets(buffer, BUF_SIZE, stdin);
    length = strlen(buffer);
    buffer[length - 1] = '\0';

    /* find a command and up to two operands, bypassing all white space */
    rest = buffer;
    last = tokens;
    *last = strtok(buffer, " \t");
    while (*last != NULL) {
        if (**last != '\0') {
            ++last;
            if (last >= &tokens[2]) {
                tokens[2] = tokens[1] + strlen(tokens[1]) + 1;
                break;
            }
        }
        *last = strtok(NULL, " \t");
    }

    if (NULL == tokens[0]) {
        return USER_ERROR;
    }
    if (tokens[1]) {
        operands[0] = jstring_new_2(tokens[1]);
        if (tokens[2]) {
            operands[1] = jstring_new_2(tokens[2]);
        }
    }

    /* which command was entered? */
    if (0 == strncmp(tokens[0], "search", 5)) {
        result = USER_SEARCH;
    } else if (0 == strncmp(tokens[0], "peers", 4)) {
        result = USER_PEERS;
    } else if (0 == strncmp(tokens[0], "groups", 6)) {
        result = USER_GROUPS;
    } else if (0 == strncmp(tokens[0], "group", 5)) {
        if (NULL == tokens[1]) {
            return USER_ERROR;
        }
        result = USER_GROUP;
    } else if (0 == strncmp(tokens[0], "join", 4)) {
        result = USER_JOIN;
    } else if (0 == strncmp(tokens[0], "leave", 5)) {
        result = USER_LEAVE;
    } else if (0 == strncmp(tokens[0], "nick", 4)) {
        if (NULL == tokens[1]) {
            return USER_ERROR;
        }
        result = USER_NICK;
    } else if (0 == strncmp(tokens[0], "status", 5)) {
        result = USER_STATUS;
    } else if (0 == strncmp(tokens[0], "help", 4)) {
        result = USER_HELP;
    } else if (0 == strncmp(tokens[0], "send", 4)) {
        if (NULL == tokens[2]) {
            return USER_ERROR;
        }
        result = USER_SEND;
    } else if (0 == strncmp(tokens[0], "quit", 4) || 0 == strncmp(tokens[0], "exit", 4)) {
        result = USER_QUIT;
    }

    return result;
}


void user_search(void)
{
    discovery_service_get_remote_advertisements(discoSvc, NULL, DISC_ADV, "Name", "whisp.*", 10, NULL);
    printf("\n  Issued search for other whisp peers.\n  Results will be reported as they arrive.\n\n");

    discovery_service_get_remote_advertisements(discoSvc, NULL, DISC_GROUP, "Name", "whisp.*", 10, NULL);
    printf("\n  Issued search for whisp groups.\n  Results will be reported as they arrive.\n\n");
}

void user_peers(void)
{

    unsigned size;
    unsigned i;

    if (NULL == pipeAds || (size = jxta_vector_size(pipeAds)) < 1 || size > JXTA_START_ERROR) {
        printf("\n  No whisp peers found\n\n");
    } else {
        printf("\n\tDiscovered Whisp Peers\n\n");
        for (i = 0; i < size; i++) {

            Jxta_pipe_adv *ad = NULL;
            Jxta_status status;

            status = jxta_vector_get_object_at(pipeAds, JXTA_OBJECT_PPTR(&ad), i);
            assert(JXTA_SUCCESS == status);
            pipe_advertisement_print_description(stdout, ad);
            if (ad)
                JXTA_OBJECT_RELEASE(ad);
        }
        printf
            ("\n  Use `send <peername> <message>` to send a message to a listed peer.\n  The `whisp.` prefix is optional.\n  All text following <peername> will be sent.\n\n");
    }
}

void user_groups(void)
{

    unsigned size;
    unsigned i;

    if (NULL == groupAds || (size = jxta_vector_size(groupAds)) < 1 || size > JXTA_START_ERROR) {
        printf("\n  No whisp subgroups found in current group\n\n");
    } else {
        printf("\n\tDiscovered Whisp Groups\n\n");
        for (i = 0; i < size; i++) {

            Jxta_PGA *ad = NULL;
            Jxta_status status;

            status = jxta_vector_get_object_at(groupAds, JXTA_OBJECT_PPTR(&ad), i);
            assert(JXTA_SUCCESS == status);
            group_advertisement_print_description(stdout, ad);
            if (ad)
                JXTA_OBJECT_RELEASE(ad);
        }
    }
}

/* group joining is broken up into creating and then joining proper, to allow ad propagation to the rdv peer */
void user_new_group(JString * groupName)
{

    JString *qualifiedGroupName = NULL;
#if USE_AD_PARSING_BUG_WORKAROUND
    Jxta_PGA *parsedAd = NULL;
    Jxta_id *GID = NULL;
    JString *jsName = NULL;
    JString *jsDescription = NULL;
#endif
    Jxta_PGA *groupAd = NULL;
    FILE *groupFile = NULL;
    JString *fileName = NULL;
    JString *xmlString = NULL;
    Jxta_status status;

    if (NULL == groupName) {
        return;
    }

    qualifiedGroupName = qualified_subgroup_name(groupName);

    printf("\n");

    /* try to generate the ad by reading a file called "<qualifiedGroupName>.xml" */
    fileName = jstring_clone(qualifiedGroupName);
    jstring_append_2(fileName, ".xml");
    groupFile = fopen(jstring_get_string(fileName), "r");
    JXTA_OBJECT_RELEASE(fileName);
    if (groupFile) {
        printf("  Creating group ad from file.\n");
#if USE_AD_PARSING_BUG_WORKAROUND == FALSE
        groupAd = jxta_PGA_new();
        assert(groupAd);
        jxta_PGA_parse_file(groupAd, groupFile);
#else
        parsedAd = jxta_PGA_new();
        assert(parsedAd);
        jxta_PGA_parse_file(parsedAd, groupFile);
        GID = jxta_PGA_get_GID(parsedAd);
        jsName = jxta_PGA_get_Name(parsedAd);
        jsDescription = jxta_PGA_get_Desc(parsedAd);
        JXTA_OBJECT_RELEASE(parsedAd);

        groupAd = jxta_PGA_new();
        jxta_PGA_set_MSID(groupAd, jxta_genericpeergroup_specid);
        jxta_PGA_set_GID(groupAd, GID);
        jxta_PGA_set_Name(groupAd, jsName);
        jxta_PGA_set_Desc(groupAd, jsDescription);

        JXTA_OBJECT_RELEASE(GID);
        JXTA_OBJECT_RELEASE(jsName);
        JXTA_OBJECT_RELEASE(jsDescription);
#endif
        fclose(groupFile);
    } else {
        groupAd = find_subgroup_advertisement(qualifiedGroupName);
        if (groupAd) {
            printf("  That group already exists.\n");
            return;
        } else {

#if CREATE_PGA_THEN_PG
            /* create the ad directly */
            printf("  Creating new group ad from scratch.\n");
            groupAd = new_subgroup_advertisement(qualifiedGroupName);
#else
            Jxta_PG *newGroup = NULL;

            /* create the ad via creating the group; this is a failed EXPERIMENT; bug I can't isolate? */
            printf("  Creating new group ad indirectly by first creating group.\n");
            newGroup = create_new_subgroup(qualifiedGroupName);
            jxta_PG_get_PGA(newGroup, &groupAd);
            assert(groupAd);

            JXTA_OBJECT_RELEASE(newGroup);
#endif
        }
    }

    /* print ad for debugging purposes */
    jxta_PGA_get_xml(groupAd, &xmlString);
    printf("%s\n\n", jstring_get_string(xmlString));
    JXTA_OBJECT_RELEASE(xmlString);

    /* publish the group in the domain of the current group with updated Time To Live */
    discovery_service_publish(discoSvc, (Jxta_advertisement *) groupAd, DISC_GROUP, GROUP_LIFETIME, GROUP_LIFETIME_IN_WILD);
    /* does _remote_publish() remove necessity for RDV to do a remote search? I don't know for sure */
    status = discovery_service_remote_publish(discoSvc, NULL, (Jxta_advertisement *) groupAd, DISC_GROUP, GROUP_LIFETIME_IN_WILD);
    assert(JXTA_SUCCESS == status);

    JXTA_OBJECT_RELEASE(groupAd);
    JXTA_OBJECT_RELEASE(qualifiedGroupName);
}

void user_join_subgroup(JString * groupName)
{

    JString *qualifiedGroupName = NULL;
    Jxta_PGA *groupAd = NULL;
    Jxta_PG *newGroup = NULL;
    Jxta_status status;

    if (NULL == groupName) {
        return;
    }

    /* find the group's advertisement */
    qualifiedGroupName = qualified_subgroup_name(groupName);
    groupAd = find_subgroup_advertisement(qualifiedGroupName);

    if (NULL == groupAd) {
        printf("\n  Cannot find advertisement for group `%s`. Please create it first with the `group` command.\n\n",
               jstring_get_string(qualifiedGroupName));
        return;
    } else {
        Jxta_vector *resources = NULL;

        /* make group from ad -- I don't really understand this bit about resource groups */
        jxta_PG_get_resourcegroups(currentGroup, &resources);
        assert(resources);
        status = jxta_vector_add_object_last(resources, (Jxta_object *) currentGroup);
        assert(JXTA_SUCCESS == status);

        status = jxta_PG_newfromadv(currentGroup, (Jxta_advertisement *) groupAd, resources, &newGroup);
        assert(JXTA_SUCCESS == status);

        JXTA_OBJECT_RELEASE(resources);

        currentGroup = newGroup;
        update_group_services();

        JXTA_OBJECT_RELEASE(groupAd);
    }

    JXTA_OBJECT_RELEASE(qualifiedGroupName);
}

void user_leave_subgroup(void)
{

    Jxta_PG *parentGroup = NULL;

    if (currentGroup == netPeerGroup) {
        return;
    }

    jxta_PG_get_parentgroup(currentGroup, &parentGroup);
    assert(parentGroup);

    JXTA_OBJECT_RELEASE(currentGroup);
    currentGroup = parentGroup;
    update_group_services();
}

void user_send_message(JString * recipient, JString * message)
{

    messageContext *context = malloc(sizeof(messageContext));
    int index = index_of_pipe_ad_with_name(recipient);
    Jxta_status status;

    /* I could just do yet another local discovery search instead of checking the ads in memory */
    if (-1 == index) {
        printf("\n  There is no known peer with the name `%s`.\n\n", jstring_get_string(recipient));
        return;
    }
    JXTA_OBJECT_SHARE(message);

    printf("send to `%s` the message `%s`\n", jstring_get_string(recipient), jstring_get_string(message));

    status = jxta_vector_get_object_at(pipeAds, JXTA_OBJECT_PPTR(&context->recipientPipeAd), index);
    assert(JXTA_SUCCESS == status);

    context->message = message;

    /* pipe service releases listener so it has to be re-created each time */
    /* but this is convenient here; message is provided as argument */
    connectionListener = jxta_listener_new(pipe_connect_event_received, context, 1, 2);
    assert(connectionListener);
    jxta_listener_start(connectionListener);

    status = jxta_pipe_service_connect(pipeSvc, context->recipientPipeAd, PIPE_CONNECT_TIMEOUT, NULL, connectionListener);
    assert(JXTA_SUCCESS == status);
}

void user_nickname(JString * newNick)
{

    char pipeName[MAX_PIPE_NAME_LENGTH];
    Jxta_status status;

    JXTA_OBJECT_RELEASE(nickname);
    JXTA_OBJECT_SHARE(newNick);
    nickname = newNick;

    snprintf(pipeName, MAX_PIPE_NAME_LENGTH, "whisp.%s", jstring_get_string(nickname));
    status = jxta_pipe_adv_set_Name(messagePipeAd, pipeName);
    assert(status == JXTA_SUCCESS);

    /* publish the pipe ad */
    status =
        discovery_service_remote_publish(discoSvc, NULL, (Jxta_advertisement *) messagePipeAd, DISC_ADV, MESSAGE_PIPE_LIFETIME);
}

void user_status(void)
{

    printf("\n    Peer Status Report\n\n  Peer Info:\n");

    group_print_my_description(stdout, currentGroup);
    printf("\n  Group Info:\n");
    group_print_description(stdout, currentGroup);
    printf("\n  Rendezvous Peer Info:\n");
    if (rdv) {
        peer_print_description(stdout, rdv);
        JXTA_OBJECT_RELEASE(rdv);
    } else {
        printf("Not connected");
    }
    printf("\n");
}

void user_help(void)
{
    printf("\n    The help instructions:\n");
    printf("Arguments:\n");
    printf("  `-n <nick>` - use <nick> as your nickname\n\n");
    printf("Commands (there are no short versions):\n");
    printf("  `search` - send a search for other peers\n");
    printf("  `peers` - list known whisper peers in the current group\n");
    printf("  `groups` - list known whisper groups (subgroups of netPeerGroup)\n");
    printf("  `group <groupname>` - creates group if not known and publishes group ad\n");
    printf("  `join <groupname>` join group named <groupname>\n    (If blank, same as `leave`)\n");
    printf("  `leave` - leave the current group (return to netPeerGroup)\n");
    printf("  `send <name> <message>` - send <message> to peer named <name>\n");
    printf("  `nick <name>` - change your nickname to <name>\n");
    printf("  `status` - print status information\n");
    printf("  `help` - print this help message\n");
    printf("  `quit` or `exit` - quit the program\n\n");
}


JString *qualified_subgroup_name(JString * baseName)
{

    JString *qualifiedName = NULL;

    /* check for "whisp." prefix on group name; if missing add it */
    if (0 == strncmp(jstring_get_string(baseName), "whisp.", 6)) {
        qualifiedName = baseName;
        JXTA_OBJECT_SHARE(qualifiedName);
    } else {
        qualifiedName = jstring_new_2("whisp.");
        assert(qualifiedName);
        jstring_append_1(qualifiedName, baseName);
    }

    return qualifiedName;
}

JString *new_subgroup_description(void)
{

    JString *groupDesc = NULL;
    char *date = NULL;
    time_t timeVal;

    groupDesc = jstring_new_2("A trivial group for whisp message peers. Created by ");
    jstring_append_1(groupDesc, nickname);
    jstring_append_2(groupDesc, " on ");
    timeVal = time(NULL);
    date = ctime(&timeVal);
    *(date + strlen(date) - 1) = '\0';  /* remove trailing newline */
    jstring_append_2(groupDesc, date);
    jstring_append_2(groupDesc, ".");

    return groupDesc;
}


Jxta_PGA *new_subgroup_advertisement(JString * groupName)
{

    Jxta_id *groupId = NULL;
    JString *groupDesc = NULL;
    Jxta_PGA *groupAd = NULL;
    Jxta_status status;

    groupAd = jxta_PGA_new();
    assert(groupAd);

    status = jxta_id_peergroupid_new_1(&groupId);
    assert(JXTA_SUCCESS == status);

    /* set Name element */
    jxta_PGA_set_Name(groupAd, groupName);

    /* set GID element */
    jxta_PGA_set_GID(groupAd, groupId);

    /* set Desc element */
    groupDesc = new_subgroup_description();
    jxta_PGA_set_Desc(groupAd, groupDesc);

    /* set MSID element */
    jxta_PGA_set_MSID(groupAd, jxta_genericpeergroup_specid);

    JXTA_OBJECT_RELEASE(groupId);
    JXTA_OBJECT_RELEASE(groupDesc);

    return groupAd;
}

Jxta_PGA *find_subgroup_advertisement(JString * groupName)
{

    Jxta_vector *ads = NULL;
    Jxta_PGA *groupAd = NULL;

    discovery_service_get_local_advertisements(discoSvc, DISC_GROUP, "Name", jstring_get_string(groupName), &ads);
    if (ads) {

        Jxta_status status;
        int size;
        int i;

        size = jxta_vector_size(ads);
        for (i = 0; i < size; i++) {
            status = jxta_vector_get_object_at(ads, JXTA_OBJECT_PPTR(&groupAd), 0);
            assert(JXTA_SUCCESS == status);

            if (TRUE == group_advertisement_is_valid(groupAd)) {
                break;
            } else {
                /* delete any malformed ads */

                JString *idString = NULL;

                status = jxta_id_to_jstring(jxta_PGA_get_GID(groupAd), &idString);
                assert(JXTA_SUCCESS == status);
                /* arg 2 of discovery_service_flush_advertisements() should be const char * */
                discovery_service_flush_advertisements(discoSvc, (char *) jstring_get_string(idString), DISC_GROUP);
            }
        }

        JXTA_OBJECT_RELEASE(ads);
    }

    return groupAd;
}

Jxta_PG *create_new_subgroup(JString * groupName)
{

    Jxta_PG *newGroup = NULL;
    JString *groupDesc = NULL;
    Jxta_vector *resources = NULL;
    Jxta_status status;

    Jxta_MIA *newpgImplementation = NULL;

    jxta_PG_get_genericpeergroupMIA(currentGroup, &newpgImplementation);
    assert(newpgImplementation);

    groupDesc = new_subgroup_description();
    jxta_PG_get_resourcegroups(currentGroup, &resources);
    assert(resources);
    status = jxta_vector_add_object_last(resources, (Jxta_object *) currentGroup);
    assert(JXTA_SUCCESS == status);

    status = jxta_PG_newfromimpl(currentGroup,
                                 NULL, (Jxta_advertisement *) newpgImplementation, groupName, groupDesc, resources, &newGroup);
    assert(JXTA_SUCCESS == status);

    JXTA_OBJECT_RELEASE(newpgImplementation);
    JXTA_OBJECT_RELEASE(groupDesc);
    JXTA_OBJECT_RELEASE(resources);

    return newGroup;
}


void print_greeting(void)
{
    printf("\n\tThis is whisp, the budget message sender.\n\
\t\tcreated by Bored Astronaut\n\
\tpart of the jxta-c project tutorials\n\n\
\tsee http://jxta-c.jxta.org for more information\n\n");
}

void print_prompt(void)
{
    printf("%s > ", jstring_get_string(nickname));
    fflush(stdout);
}

void update_group_services(void)
{
    if (discoSvc) {
        discovery_service_remove_discovery_listener(discoSvc, (Jxta_discovery_listener *) discoListener);
        JXTA_OBJECT_RELEASE(discoSvc);
        discoSvc = NULL;
    }
    jxta_PG_get_discovery_service(currentGroup, &discoSvc);
    assert(discoSvc);
    discovery_service_add_discovery_listener(discoSvc, (Jxta_discovery_listener *) discoListener);

    if (rdvSvc) {
        jxta_rdv_service_remove_event_listener(rdvSvc, "whisp", NULL);
        JXTA_OBJECT_RELEASE(rdvSvc);
        rdvSvc = NULL;
    }
    jxta_PG_get_rendezvous_service(currentGroup, &rdvSvc);
    assert(rdvSvc);
    jxta_rdv_service_add_event_listener(rdvSvc, "whisper", NULL, rdvListener);

    /* pipe listener is added in listen_on_message_pipe(), removed in shut_down_message_pipe() */
    if (pipeSvc) {
        shut_down_message_pipe();
        JXTA_OBJECT_RELEASE(pipeSvc);
        pipeSvc = NULL;
    }
    jxta_PG_get_pipe_service(currentGroup, &pipeSvc);
    assert(pipeSvc);

    /* lose the current group's ads */
    if (pipeAds) {
        JXTA_OBJECT_RELEASE(pipeAds);
        pipeAds = NULL;

    }
    if (groupAds) {
        JXTA_OBJECT_RELEASE(groupAds);
        groupAds = NULL;
    }
}

/* only for use with the vector of pipe ads stored locally */
int index_of_pipe_ad_with_name(JString * recipient)
{

    JString *fullName = NULL;
    unsigned length;
    unsigned size;
    unsigned i;

    /*  there is a bug in jxta_vector_size() */
    if (NULL == pipeAds || (size = jxta_vector_size(pipeAds)) < 1 || size > JXTA_START_ERROR) {
        return -1;
    }

    /* add preceding "whisp." if missing */
    if (0 != strncmp(jstring_get_string(recipient), "whisp.", strlen("whisp."))) {
        fullName = jstring_new_2("whisp.");
        jstring_append_1(fullName, recipient);
    } else {
        fullName = recipient;
        JXTA_OBJECT_SHARE(fullName);
    }

    length = jstring_length(fullName);
    for (i = 0; i < size; i++) {
        /* read each pipe ad name and compare to nickname */
        Jxta_pipe_adv *ad = NULL;

        jxta_vector_get_object_at(pipeAds, JXTA_OBJECT_PPTR(&ad), i);
        if (0 == strncmp(jstring_get_string(fullName), jxta_pipe_adv_get_Name(ad), length)) {
            break;
        }
    }

    if (fullName)
        JXTA_OBJECT_RELEASE(fullName);

    if (i == size)
        i = -1;
    return i;
}


void rdv_event_received(Jxta_object * obj, void *arg)
{
    Jxta_rdv_event *event = (Jxta_rdv_event *) obj;

    printf("\n  Received a Rendezvous Service Event: ");
    switch (event->event) {
    case JXTA_RDV_FAILED:
        printf("JXTA_RDV_FAILED");
        /* user_leave_subgroup(); */
        break;
    case JXTA_RDV_CONNECTED:
        printf("JXTA_RDV_CONNECTED");
        if (rdv) {
            JXTA_OBJECT_RELEASE(rdv);
            rdv = NULL;
        }
        rdv = group_get_rendezvous_peer(netPeerGroup);
        assert(rdv);

        if (NULL == messagePipeAd) {
            find_or_create_message_pipe();
        }
        listen_on_message_pipe();
        break;

        /* not really sure what to do in this case */
    case JXTA_RDV_RECONNECTED:
        printf("JXTA_RDV_RECONNECTED");
        break;

        /* not sure if these events are ever generated */
    case JXTA_RDV_DISCONNECTED:
        printf("JXTA_RDV_DISCONNECTED");
        JXTA_OBJECT_RELEASE(rdv);
        break;
    default:
        printf("nature of event unknown!");
        break;
    }

    printf("\n\n");
    print_prompt();
}

void discovery_response_received(Jxta_object * obj, void *arg)
{

    Jxta_DiscoveryResponse *response = (Jxta_DiscoveryResponse *) obj;
    Jxta_vector *newAds = NULL;
    unsigned size;
    unsigned i;
    short type;
    Jxta_status status;

    status = jxta_discovery_response_get_advertisements(response, &newAds);
    assert(JXTA_SUCCESS == status);

    type = jxta_discovery_response_get_type(response);
    if (DISC_ADV == type) {
        /* Pipe advertisements */

        /* print out info about the new pipeAds */
        printf("\n\tNew whisp peer pipeAds received:\n");

        size = jxta_vector_size(newAds);
        for (i = 0; i < size; i++) {

            Jxta_pipe_adv *ad = NULL;

            status = jxta_vector_get_object_at(newAds, JXTA_OBJECT_PPTR(&ad), i);
            assert(JXTA_SUCCESS == status);
#if 0
      discovery_service_publish(discoSvc, (Jxta_advertisement *)ad, DISC_ADV, MESSAGE_PIPE_LIFETIME, MESSAGE_PIPE_LIFETIME);
#endif      
            printf("\n");
            pipe_advertisement_print_description(stdout, ad);

            if (ad)
                JXTA_OBJECT_RELEASE(ad);
        }
        printf("\n");

        if (pipeAds) {
            JXTA_OBJECT_RELEASE(pipeAds);
        }
        pipeAds = newAds;

        /*
         * instead of manually merging existing and new pipeAds, let the library do it -- doesn't work ??
         * The hash table doesn't seem to have any values stored for the Name element in DISC_ADV
         */
#if 0
    status = discovery_service_get_local_advertisements(discoSvc, DISC_ADV, "Name", "whisp.*", &pipeAds);
    assert(JXTA_SUCCESS == status);
#endif
    } else if (DISC_GROUP == type) {
        /* Group advertisements */

        /* print out info about the new ads */
        printf("\n\tNew whisp group ads received:\n");

        size = jxta_vector_size(newAds);
        for (i = 0; i < size; i++) {

            Jxta_PGA *ad = NULL;

            status = jxta_vector_get_object_at(newAds, JXTA_OBJECT_PPTR(&ad), i);
            assert(JXTA_SUCCESS == status);
            printf("\n");
            group_advertisement_print_description(stdout, ad);

            if (ad)
                JXTA_OBJECT_RELEASE(ad);
        }
        printf("\n");

        if (groupAds) {
            JXTA_OBJECT_RELEASE(groupAds);
        }
        groupAds = newAds;
    } else {
        printf("\n\tUnrecognized ads received:\n");
    }

    print_prompt();
}

void pipe_accept_event_received(Jxta_object * obj, void *arg)
{

    Jxta_pipe_connect_event *event = (Jxta_pipe_connect_event *) obj;
    Jxta_status status;

    usleep(10000);

    if (JXTA_PIPE_CONNECTED == jxta_pipe_connect_event_get_event(event)) {
        status = jxta_pipe_connect_event_get_pipe(event, &messagePipe);
        assert(JXTA_SUCCESS == status);

        status = jxta_pipe_get_inputpipe(messagePipe, &inPipe);
        assert(JXTA_SUCCESS == status);

        status = jxta_inputpipe_add_listener(inPipe, messageListener);
        assert(JXTA_SUCCESS == status);

        printf("\n  Pipe accept event received. Ready to receive messages.\n\n");
    } else {
        printf("\n  Apologies. Apparently we cannot receive messages.\n\n");
    }

    print_prompt();
}

void message_received(Jxta_object * obj, void *arg)
{

    Jxta_message *message = (Jxta_message *) obj;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *bytes = NULL;
    JString *sender = NULL;
    JString *content = NULL;
    Jxta_status status;

    status = jxta_message_get_element_1(message, "Sender", &el);
    assert(JXTA_SUCCESS == status);
    bytes = jxta_message_element_get_value(el);
    assert(bytes);
    sender = jstring_new_3(bytes);
    assert(sender);
    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(bytes);

    status = jxta_message_get_element_1(message, "Content", &el);
    assert(JXTA_SUCCESS == status);
    bytes = jxta_message_element_get_value(el);
    assert(bytes);
    content = jstring_new_3(bytes);
    assert(content);
    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(bytes);

    printf("\n  Message Received from user %s:\n\n", jstring_get_string(sender));
    printf("\t%s\n\n", jstring_get_string(content));

    JXTA_OBJECT_RELEASE(sender);
    JXTA_OBJECT_RELEASE(content);
    print_prompt();
}

void pipe_connect_event_received(Jxta_object * obj, void *arg)
{

    Jxta_pipe_connect_event *event = (Jxta_pipe_connect_event *) obj;
    messageContext *context = (messageContext *) arg;
    Jxta_pipe *sendPipe = NULL;
    Jxta_status status;

    status = jxta_pipe_connect_event_get_pipe(event, &sendPipe);
    assert(JXTA_SUCCESS == status);

    /* get the output pipe */
    if (JXTA_PIPE_CONNECTED == jxta_pipe_connect_event_get_event(event)) {

        Jxta_outputpipe *outPipe = NULL;
        Jxta_message *message = NULL;
        Jxta_message_element *el = NULL;

        status = jxta_pipe_get_outputpipe(sendPipe, &outPipe);
        assert(JXTA_SUCCESS == status);

        printf("\n  Successfully connected. Sending message.\n\n");
        print_prompt();

        /* create and send the message */
        message = jxta_message_new();
        assert(message);
        el = jxta_message_element_new_1("Sender", "text/plain", jstring_get_string(nickname), jstring_length(nickname), NULL);
        jxta_message_add_element(message, el);
        JXTA_OBJECT_RELEASE(el);
        el = jxta_message_element_new_1("Content", "text/plain", jstring_get_string(context->message),
                                        jstring_length(context->message), NULL);
        jxta_message_add_element(message, el);
        JXTA_OBJECT_RELEASE(el);

        jxta_outputpipe_send(outPipe, message);

        JXTA_OBJECT_RELEASE(outPipe);
        JXTA_OBJECT_RELEASE(message);
    } else {

        /* the target is not receiving -- delete the ad from the memory cache and the local cache */
        vector_remove_object(pipeAds, (Jxta_object *) context->recipientPipeAd);
        discovery_service_flush_advertisements(discoSvc, jxta_pipe_adv_get_Id(context->recipientPipeAd), DISC_ADV);

        printf("\n  Unable to connect to user pipe.\n\n");
        print_prompt();
    }

    JXTA_OBJECT_RELEASE(sendPipe);

    JXTA_OBJECT_RELEASE(context->recipientPipeAd);
    JXTA_OBJECT_RELEASE(context->message);
    free(context);
}
