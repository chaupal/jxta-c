/* 
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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

static const char *__log_cat = "DPT";

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <apr_general.h>
#include "apr_time.h"

#include "jxta.h"
#include "jxta_object_type.h"
#include "jxta_vector.h"
#include "jxta_hashtable.h"
#include "jxta_peergroup.h"
#include "jxta_discovery_service.h"
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_rdv_service.h"
#include "jxta_pipe_service.h"

/*
 *
 * Discovery Publication Test
 *
 * Premise of the test:
 *  - Each peer listens on a common propagate pipe.
 *  - Each peer publishes Pipe Advertisements
 *  - Each peer announces itself on the common propagate pipe serveral time.
 *  - Each peer performs queries against other peers it has learned of via the propagate pipe.
 *  - Each peer shuts down.
 */

/**
*   The pipe id we use for discovering other discovery publication test peers.
*
*   This ID was randomly generated using the shell.
**/
static const char ANNOUNCE_PIPE_ID[] = "urn:jxta:uuid-59616261646162614E50472050325033EA751C655A5E403CB9C1F05C98E7F28104";

static const int ADVERTISEMENTS_TO_PUBLISH = 100;

static const Jxta_expiration_time ADVERTISEMENT_LIFETIME = 24L * 60L * 60L * 1000L;

static const int ADVERTISEMENT_SEARCHES = 100;

static Jxta_pipe_adv *get_pipe_adv( const char *pipe_id, const char *pipe_name, const char *pipe_type );
static Jxta_status announce_pipe_login(void);
static Jxta_status announce_pipe_logout(void);
static void JXTA_STDCALL announce_pipe_listener(Jxta_object * obj, void *arg);
static Jxta_status announce_pipe_announcer(int ads);

static Jxta_PG *group = NULL;
static Jxta_discovery_service *discovery_service = NULL;
static Jxta_pipe_service *pipe_service = NULL;
static Jxta_PID *local_pid;

static Jxta_pipe *announce_pipe = NULL;
static Jxta_inputpipe *announce_input_pipe = NULL;
static Jxta_outputpipe *announce_output_pipe = NULL;
static Jxta_listener *announce_listener = NULL;

struct Other_peer {
    Extends(Jxta_object);       /* equivalent to JXTA_OBJECT_HANDLE; char* thisType. */
    JString *pid;
    int ads;
};
typedef struct Other_peer Other_peer;
static Other_peer *other_peer_new( JString* pid, int ads );
static void other_peer_delete(Jxta_object * addr);
static Other_peer *JXTA_STDCALL other_peer_construct(Other_peer * it, JString* pid, int ads );
void JXTA_STDCALL other_peer_destruct(Other_peer * it);

static Jxta_hashtable *other_peers = NULL;

static Other_peer *other_peer_new( JString* pid, int ads ) {
    Other_peer *self = (Other_peer *) calloc(1, sizeof(Other_peer));

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, other_peer_delete, 0);
    self->thisType = "Other_peer";

    return other_peer_construct(self, pid, ads);
}

static void other_peer_delete(Jxta_object * addr)
{
    Other_peer *self = (Other_peer *) addr;

    other_peer_destruct(self);
    self->thisType = NULL;

    free(self);
}

static Other_peer *JXTA_STDCALL other_peer_construct(Other_peer * it, JString* pid, int ads )
{
    it->pid = JXTA_OBJECT_SHARE(pid);
    it->ads = ads;

    return it;
}

void JXTA_STDCALL other_peer_destruct(Other_peer * it)
{
    if (it->pid) {
        JXTA_OBJECT_RELEASE(it->pid);
    }    
}

/**
*   Make a pipe advertisement out of pipe parameters.
*/
static Jxta_pipe_adv *get_pipe_adv( const char *pipe_id, const char *pipe_name, const char *pipe_type ) {
    Jxta_pipe_adv *adv = jxta_pipe_adv_new();

    jxta_pipe_adv_set_Id(adv, pipe_id);
    jxta_pipe_adv_set_Name(adv, pipe_name);
    jxta_pipe_adv_set_Type(adv, pipe_type);

    return adv;
}

static Jxta_status announce_pipe_login(void) {
    Jxta_status status;
    Jxta_pipe_adv *adv = get_pipe_adv( ANNOUNCE_PIPE_ID, "JXTA-C Discovery Publish Test", JXTA_PROPAGATE_PIPE );
    
    /* Create a listener for the propagate messages */
    announce_listener = jxta_listener_new((Jxta_listener_func) announce_pipe_listener, NULL, 1, 200);
    if (announce_listener == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot create announce listener\n");
        return JXTA_FAILED;
    }

    jxta_listener_start(announce_listener);

    /* Get the pipe */
    status = jxta_pipe_service_timed_accept(pipe_service, adv, 1000, &announce_pipe);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot get pipe  : %ld\n", status );
        return status;
    }

    status = jxta_pipe_get_inputpipe(announce_pipe, &announce_input_pipe);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot get inputpipe : %ld\n", status );
        return status;
    }

    status = jxta_inputpipe_add_listener(announce_input_pipe, announce_listener);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot add listener : %ld\n", status );
        return status;
    }
    
    status = jxta_pipe_get_outputpipe(announce_pipe, &announce_output_pipe);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot get outputpipe : %ld\n", status );
        return status;
    }

    if (announce_output_pipe == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot get outputpipe\n" );
        return JXTA_FAILED;
    }

    JXTA_OBJECT_RELEASE(adv);
    
    return JXTA_SUCCESS;
}

static Jxta_status announce_pipe_logout(void) {

    if (NULL != announce_output_pipe) {
        JXTA_OBJECT_RELEASE(announce_output_pipe);
        announce_output_pipe = NULL;
    }

    if (NULL != announce_input_pipe) {
        jxta_inputpipe_remove_listener( announce_input_pipe, announce_listener );
        JXTA_OBJECT_RELEASE(announce_input_pipe);
        announce_input_pipe = NULL;
    }

    if (NULL != announce_pipe) {
        JXTA_OBJECT_RELEASE(announce_pipe);
        announce_pipe = NULL;
    }

    if (NULL != announce_listener) {
        jxta_listener_stop(announce_listener);
        JXTA_OBJECT_RELEASE(announce_listener);
        announce_listener = NULL;
    }
    
    return JXTA_SUCCESS;
}

static void JXTA_STDCALL announce_pipe_listener(Jxta_object * obj, void *arg) {
    Jxta_status status;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *bytes;
    JString *string;
    int ads;
    Other_peer* other_peer;

    JXTA_OBJECT_CHECK_VALID(msg);
    
    status = jxta_message_get_element_2(msg, "", "ADS", &el);

    if ((JXTA_SUCCESS != status) || (NULL == el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Incomplete announce message : %ld\n", status);
        return;
    }
    
    bytes = jxta_message_element_get_value(el);
    JXTA_OBJECT_RELEASE(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);
    
    ads = atoi( jstring_get_string(string) );   
    JXTA_OBJECT_RELEASE(string);

    status = jxta_message_get_element_2(msg, "", "PID", &el);

    if ((JXTA_SUCCESS != status) || (NULL == el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Incomplete announce message : %ld\n", status);
        return;
    }
    
    bytes = jxta_message_element_get_value(el);
    JXTA_OBJECT_RELEASE(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);
    
    if( 0 != ads ) {
        other_peer = other_peer_new( string, ads );
 
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding other_peer record for %s (%d)\n", jstring_get_string(string), ads );
    
        jxta_hashtable_put(other_peers, jstring_get_string(string), jstring_length(string) + 1, (Jxta_object *) other_peer);
        JXTA_OBJECT_RELEASE(other_peer);        
    } else {
        jxta_hashtable_del(other_peers, jstring_get_string(string), jstring_length(string) + 1, NULL);
    }
    JXTA_OBJECT_RELEASE(string);
}

static Jxta_status announce_pipe_announcer(int ads) {
    Jxta_status status;
    Jxta_message *msg = jxta_message_new();
    Jxta_message_element *el = NULL;
    JString *pid_string;
    char ads_string[256];
        
    jxta_id_to_jstring(local_pid, &pid_string);
    el = jxta_message_element_new_2( "", "PID", "text/plain", jstring_get_string(pid_string), jstring_length(pid_string), NULL);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    sprintf( ads_string, "%d", ads );
    el = jxta_message_element_new_2( "", "ADS", "text/plain", ads_string, strlen(ads_string), NULL);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    status = jxta_outputpipe_send(announce_output_pipe, msg);
    
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Sending announce failed: %ld\n", status);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sent announce\n");
    }

    JXTA_OBJECT_RELEASE(msg);

    return status;
}

static Jxta_status publish_advertisements() {
    Jxta_status status;
    int eachAdv;
    Jxta_id *gid = NULL;
    JString *pid_string;
    
    jxta_PG_get_GID(group, &gid);
    jxta_id_to_jstring(local_pid, &pid_string);

    for( eachAdv = 1; eachAdv <= ADVERTISEMENTS_TO_PUBLISH; eachAdv++ ) {
        Jxta_id *pipeid = NULL;
        char ad_number_string[512];
        JString *pipeid_string;
        Jxta_pipe_adv *adv;
        
        jxta_id_pipeid_new_1(&pipeid, gid);

        JXTA_OBJECT_CHECK_VALID(pipeid);

        jxta_id_to_jstring(pipeid, &pipeid_string);

        sprintf( ad_number_string, "JXTA-C Discovery Publish Test : Peer %s Advertisement %d", jstring_get_string(pid_string), eachAdv );

        adv = get_pipe_adv( jstring_get_string(pipeid_string), ad_number_string, JXTA_UNICAST_PIPE );
       
        status = discovery_service_publish(discovery_service, (Jxta_advertisement *) adv, DISC_ADV, ADVERTISEMENT_LIFETIME, ADVERTISEMENT_LIFETIME);
        JXTA_OBJECT_RELEASE(adv);

        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "publish failed\n");
            break;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Published adv %d\n", eachAdv );
        }
        
        if ( 0 == (eachAdv % 100) ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Announcing %d advertisements\n", eachAdv );
            announce_pipe_announcer( eachAdv );
        }
    }
    
    if( JXTA_SUCCESS == status ) {
        announce_pipe_announcer( eachAdv );
    }
    
    JXTA_OBJECT_RELEASE(gid);
    JXTA_OBJECT_RELEASE(pid_string);
    
    return status;
}

static Jxta_status search_advertisements(void) {
    Jxta_status status;
    int eachAdv;
    Jxta_vector* others = NULL;
    JString *pid_string;
    
    jxta_id_to_jstring(local_pid, &pid_string);
    
    for( eachAdv = 1; eachAdv <= ADVERTISEMENT_SEARCHES; eachAdv++ ) {
        Other_peer* other = NULL;
        
        if( (NULL == others) || (0 == jxta_vector_size(others)) ) {
            if( NULL != others ) {
                JXTA_OBJECT_RELEASE(others);
            }
            
            others = jxta_hashtable_values_get(other_peers);
            
            if(0 == jxta_vector_size(others)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No other peers to query! Sleeping a bit.\n" );
                apr_sleep( 5 * 1000 * 1000 );
                continue;
            }
            
            jxta_vector_shuffle(others);
            }
        
        status = jxta_vector_remove_object_at( others, (Jxta_object**) &other, 0 );
        
        if( (JXTA_SUCCESS != status) || (NULL == other)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failure loading other peer.\n" );
            continue;
        } else {
            char ad_query_string[512];
            int whichAdv = (rand() % other->ads) + 1;
            
            sprintf( ad_query_string, "JXTA-C Discovery Publish Test : Peer %s Advertisement %d", jstring_get_string(other->pid), whichAdv );
        
            status = discovery_service_get_remote_advertisements( discovery_service, NULL, DISC_ADV, "Name", ad_query_string, 1, NULL );
        }
    }
    
    JXTA_OBJECT_RELEASE(pid_string);
    JXTA_OBJECT_RELEASE(others);
    
    return status;
}

static int jxta_discovery_publish_test(void)
{
    Jxta_status status;
    int res = 0;
    Jxta_rdv_service* rendezvous_service;
    RdvConfig_configuration configuration;
    
    status = jxta_PG_new_netpg(&group);
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "# jxta_PG_netpg_new() failed : %ld\n", status);
        return -1;
    }
    jxta_PG_get_discovery_service(group, &discovery_service);
    jxta_PG_get_pipe_service(group, &pipe_service);
    jxta_PG_get_PID(group, &local_pid);
    other_peers = jxta_hashtable_new_0(10, TRUE);
    
    jxta_PG_get_rendezvous_service(group, &rendezvous_service);

    configuration = jxta_rdv_service_config(rendezvous_service);
    if( (config_adhoc != configuration) && (config_rendezvous != configuration) ) {
        int rdv_wait_count = 0;
        Jxta_boolean hasRdv = FALSE;
        
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Waiting for RendezVous.\n" );
               
        while( !hasRdv && (rdv_wait_count < 20) ) {
            Jxta_vector* rdv_peers;

            jxta_rdv_service_get_peers(rendezvous_service, &rdv_peers );
            
            hasRdv = 0 != jxta_vector_size(rdv_peers);
            
            JXTA_OBJECT_RELEASE(rdv_peers);
            
            /* sleep a short time */
            apr_sleep( 3 * 1000 * 1000 );
        }
        
        if( !hasRdv ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "# No rendezvous connection. Quitting\n" );
            return -1;
        }
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "RendezVous Service ready for publications!\n" );
    
    status = announce_pipe_login();
    if( JXTA_SUCCESS != status ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to login announce pipe : %ld\n", status);
        res = -1;
        goto Common_Exit;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Completed announce pipe login.\n");
    
    publish_advertisements();
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Completed advertisement publish.\n");
    
    apr_sleep( 60 * 1000 * 1000);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Completed 60 second nap.\n");
    
    search_advertisements();
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Completed advertisement search.\n");
        
    apr_sleep( 60 * 1000 * 1000);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Completed 60 second nap.\n");

    status = announce_pipe_logout();
    if( JXTA_SUCCESS != status ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to logout announce pipe : %ld\n", status);
        res = -1;
        goto Common_Exit;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Completed announce pipe logout.\n");

Common_Exit:
    JXTA_OBJECT_RELEASE(discovery_service);
    JXTA_OBJECT_RELEASE(pipe_service);
    JXTA_OBJECT_RELEASE(local_pid);
    JXTA_OBJECT_RELEASE(other_peers);
   
    jxta_module_stop((Jxta_module *) group);
    JXTA_OBJECT_RELEASE(group);
    group = NULL;

    return res;
}


int main(int argc, char **argv)
{
    Jxta_log_file *log_f;
    Jxta_log_selector *log_s;
    Jxta_status status;
    int result = 1;
    
    jxta_initialize();
    
    srand( time(NULL) );
    
    log_s = jxta_log_selector_new_and_set( "*.info-fatal", &status);
    if (NULL == log_s) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "# %s - Failed to init default log selector.\n", argv[0]);
        result = -1;
        goto Common_Exit;
    }

    jxta_log_file_open(&log_f, "-");
    jxta_log_using(jxta_log_file_append, log_f);
    jxta_log_file_attach_selector(log_f, log_s, NULL);

    result = jxta_discovery_publish_test();
    
    jxta_log_using(NULL, NULL);
    jxta_log_file_close(log_f);
    jxta_log_selector_delete(log_s);

Common_Exit : 
    jxta_terminate();
    
    return result;
}
