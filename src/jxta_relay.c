/*
 * Copyright (c) 2002-2004 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_relay.c,v 1.61 2007/01/06 01:45:12 slowhog Exp $
 */
#include <stdlib.h> /* for atoi */

#ifndef WIN32
#include <signal.h> /* for sigaction */
#endif

#include "jxta_apr.h"
#include "jpr/jpr_excep_proto.h"

#include "jxta_types.h"
#include "jxta_log.h"
#include "jxta_errno.h"
#include "jstring.h"
#include "jxta_transport_private.h"
#include "jxta_peergroup.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_relaya.h"
#include "jxta_relay.h"
#include "jxta_rdv.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_vector.h"
#include "jxta_object_type.h"
#include "jxta_peer.h"
#include "jxta_peer_private.h"
#include "jxta_endpoint_service_priv.h"

static const char *__log_cat = "RELAY";

#define ONE_SECOND_MICRO (1000000L)
#define ONE_SECOND_MILLI (1000)
#define MILLI_TO_MICRO (1000)

/**
 ** Renewal of the leases must be done before the lease expires. The following
 ** constant defines the number of microseconds to subtract to the expiration
 ** time in order to set the time of renewal.
 **/
#define RELAY_RENEWAL_DELAY ((Jxta_time_diff) (5 * 60 * 1000))  /* 5 Minutes */

/**
 ** Time the background thread that regularly checks the peer will wait
 ** between two iteration.
 **/

#define RELAY_THREAD_NAP_TIME_NORMAL (1 * 60 * 1000 * 1000) /* 1 Minute */
#define RELAY_THREAD_NAP_TIME_FAST (2 * 1000 * 1000)    /* 2 Seconds */

/**
 ** Minimal number of relays the peers need to be connected to.
 ** If there is fewer connected relay, the Relay Service
 ** becomes more aggressive trying to connect to a relay.
 **/

#define MIN_NB_OF_CONNECTED_RELAYS 1

/**
 ** Maximum time between two connection attempts to the same peer.
 **/

#define RELAY_MAX_RETRY_DELAY (30 * 60 * 1000)  /* 30 Minutes */

/**
 ** Minimum delay between two connection attempts to the same peer.
 **/

#define RELAY_MIN_RETRY_DELAY (3 * 1000)    /* 3 Seconds */

/**
 ** When the number of connected peer is less than MIN_NB_OF_CONNECTED_PEERS
 ** the Relay Service tries to connect to peers earlier than planned
 ** if there were scheduled for more than LONG_RETRY_DELAY
 **/

#define RELAY_LONG_RETRY_DELAY (RELAY_MAX_RETRY_DELAY / 2)

/**
 ** The amount of time the relay will store the message before
 ** releasing it
 **/
#define DEFAULT_MESSAGE_STORAGE_INTERVAL (ONE_SECOND_MILLI*60*10) /* 10 minutes */

/* define the sleep time for the send messages thread to be related to the
 * message storage interval
 */
#define SEND_MESSAGES_THREAD_SLEEP_TIME (MILLI_TO_MICRO*DEFAULT_MESSAGE_STORAGE_INTERVAL/5)

#define MESSAGE_CHECK_EXPIRY_INTERVAL (DEFAULT_MESSAGE_STORAGE_INTERVAL*2)

#define LEASE_CHECK_EXPIRY_INTERVAL (5*60*ONE_SECOND_MILLI)

#define RESCAN_REMOTE_RELAY_ADVERTISEMENTS (10*60*ONE_SECOND_MILLI)

#define MAX_NUMBER_MESSAGES_PER_CLIENT 100

#define RELAY_NS "relay"

#define DEFAULT_MAX_ALLOWED_LEASES 5000

#define ENDPOINT_SOURCE_ADDRESS "EndpointSourceAddress"
#define ENDPOINT_DESTINATION_ADDRESS "EndpointDestinationAddress"
#define RELAY_DISCONNECT_REQUEST "disconnect"
#define JXTA_NS "jxta"
#define RELAY_REQUEST_MESSAGE_ELEMENT "request"
#define RELAY_PEER_ID_MESSAGE_ELEMENT "requestor_pid"


/*#define ADD_PRINT_STATEMENTS*/
#ifdef ADD_PRINT_STATEMENTS
#define RELAY_LOG_STATEMENT(value) printf(value);
#else
#define RELAY_LOG_STATEMENT(value) jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, value);
#endif

typedef Jxta_transport Jxta_transport_relay;

/*
 * _jxta_transport_relay Structure  
 */
struct _jxta_transport_relay {
    Extends(Jxta_transport);

    apr_thread_cond_t *stop_cond;
    apr_thread_mutex_t *stop_mutex;
    apr_pool_t *pool;

    Jxta_endpoint_address *address;
    char *assigned_id;
    char *peerid;
    Jxta_PG *group;
    char *groupid;
    Jxta_boolean Is_Client;
    Jxta_boolean Is_Server;
    Jxta_vector *HttpRelays;
    Jxta_vector *TcpRelays;
    Jxta_vector *peers;
    Jxta_endpoint_service *endpoint;
    apr_thread_t *thread;
    volatile Jxta_boolean running;

    void *ep_cookie;

	/*server entries*/
	apr_thread_t *messages_thread;
	apr_thread_cond_t *messages_cond;
    apr_thread_mutex_t *messages_mutex;
	volatile Jxta_boolean messages_thread_running;
	Jxta_vector* leases;
	unsigned long max_leases_allowed;
	Jxta_boolean messages_new_arrivals;
};

typedef struct _jxta_transport_relay _jxta_transport_relay;


/**
*    Default duration of leases measured in relative milliseconds.
**/
static const Jxta_time_diff DEFAULT_LEASE_DURATION = (((Jxta_time_diff) 20) * 60 * 1000L);  /* 20 Minutes */

typedef struct _relay_lease _relay_lease;

typedef struct _stored_message _stored_message;

typedef struct _jxta_linked_list_node Jxta_Linked_List_Node;


/* Lease structure
*/
struct _relay_lease {
	JXTA_OBJECT_HANDLE;
	JString* lease_requestor_id;
	Jxta_endpoint_address* lease_requestor_address;
	Jxta_time issue_time;
	Jxta_time_diff lease_duration;
	_stored_message* stored_messages;
    int total_stored;
	/*Jxta_vector* stored_messages;*/
	apr_pool_t *pool;
	apr_thread_mutex_t *mutex;
	Jxta_boolean messages_new_additions;
    int number_failed_sends;
};

struct _jxta_linked_list_node {
	JXTA_OBJECT_HANDLE;
	Jxta_Linked_List_Node *left;
	Jxta_Linked_List_Node *right;
};


#define JXTA_LINKED_LIST_NODE_HANDLE Jxta_Linked_List_Node node
#define JXTA_LINKED_LIST_NODE(object) ((Jxta_Linked_List_Node*)object)
#define JXTA_LINKED_LIST_NODE_PPTR(x) ((Jxta_Linked_List_Node**)(void*)(x))

struct _stored_message {
	JXTA_LINKED_LIST_NODE_HANDLE;
	Jxta_message * msg;
	Jxta_time time_stored;
	Jxta_time_diff storage_interval_length; 
};



/*********************************************************************
 ** Internal structure of a peer used by this service.
 *********************************************************************/

struct _jxta_peer_relay_entry {
    Extends(_jxta_peer_entry);

    Jxta_boolean is_connected;
    Jxta_boolean try_connect;
    Jxta_time connectTime;
    Jxta_time connectRetryDelay;
};

typedef struct _jxta_peer_relay_entry _jxta_peer_relay_entry;

/*
 * Relay transport methods
 */
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * module, const char *argv[]);
static void stop(Jxta_module * module);

static JString *name_get(Jxta_transport * self);
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * self);
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr);
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params);
static Jxta_boolean allow_overload_p(Jxta_transport * tr);
static Jxta_boolean allow_routing_p(Jxta_transport * tr);
static Jxta_boolean connection_oriented_p(Jxta_transport * tr);

static Jxta_status JXTA_STDCALL relay_transport_incoming_cb(Jxta_object * obj, void *arg);
static void *APR_THREAD_FUNC connect_relay_thread(apr_thread_t * thread, void *arg);
static void check_relay_lease(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer);
static void connect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer);
static void reconnect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer);
static void check_relay_connect(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer, int nbOfConnectedPeers);
static void process_connected_reply(_jxta_transport_relay * self, Jxta_message * msg);
static void process_disconnected_reply(_jxta_transport_relay * self, Jxta_message * msg);
static void update_relay_peer_connection(_jxta_transport_relay * self, Jxta_RdvAdvertisement * relay, Jxta_time_diff lease);
static _jxta_peer_relay_entry *relay_entry_construct(_jxta_peer_relay_entry * self);
static void relay_entry_destruct(_jxta_peer_relay_entry * self);
static void check_for_lease_request(Jxta_message *msg, _jxta_transport_relay *self);
static Jxta_status send_relay_lease(_jxta_transport_relay *self, Jxta_endpoint_address *client_address);
static _relay_lease* relay_lease_new();
static void relay_lease_delete(Jxta_object * obj);
static JxtaEndpointMessenger *get_messenger(Jxta_transport * t, Jxta_endpoint_address * dest);
static void relay_messenger_delete(Jxta_object* obj);
static _stored_message* stored_message_new(Jxta_message * msg,
										   Jxta_time_diff storage_interval_length);
static void stored_message_delete(Jxta_object * obj);
static Jxta_boolean message_has_expired(const _stored_message* stored_message);
static Jxta_status archive_check_timeout(_jxta_transport_relay* self);
static Jxta_vector* archive_get_all_messages(_jxta_transport_relay* self, JString* dest_pid);
static Jxta_status archive_put_message_1(_relay_lease* jxta_lease_object, Jxta_message * msg);
static Jxta_status archive_put_message(_jxta_transport_relay* self, Jxta_message * msg, JString* dest_pid);
static _relay_lease* find_relay_lease(_jxta_transport_relay* self, JString* dest_pid);
static Jxta_status relay_send_all_messages(_jxta_transport_relay* self, JString* dest_pid);
static void *APR_THREAD_FUNC send_messages_thread(apr_thread_t * thread, void *arg);
static Jxta_status relay_send_all_messages_1(_jxta_transport_relay* self, _relay_lease* relay_lease);
static Jxta_boolean lease_is_valid(_relay_lease* relay_lease);
static Jxta_status lease_delete(_jxta_transport_relay* self, JString* lease_uid);
static Jxta_status leases_remove_expired(_jxta_transport_relay* self);
static Jxta_RdvAdvertisement *jxta_relay_build_rdva(Jxta_PG * group, JString * serviceName);
Jxta_Linked_List_Node* linked_list_init_single_item(Jxta_Linked_List_Node *node);
Jxta_Linked_List_Node* linked_list_add_last(Jxta_Linked_List_Node *node, Jxta_Linked_List_Node **head);
Jxta_Linked_List_Node* linked_list_remove(Jxta_Linked_List_Node *node, Jxta_Linked_List_Node **head);
void linked_list_clear_and_free(Jxta_Linked_List_Node* head);
Jxta_boolean linked_list_node_in_list(Jxta_Linked_List_Node* node);
Jxta_Linked_List_Node* linked_list_left(Jxta_Linked_List_Node *node);
Jxta_Linked_List_Node* linked_list_right(Jxta_Linked_List_Node *node);


typedef Jxta_transport_methods Jxta_transport_relay_methods;

static const Jxta_transport_relay_methods JXTA_TRANSPORT_RELAY_METHODS = {
    {
     "Jxta_module_methods",
     init,
     start,
     stop},
    "Jxta_transport_methods",
    name_get,
    publicaddr_get,
    get_messenger,
    ping,
    propagate,
    allow_overload_p,
    allow_routing_p,
    connection_oriented_p
};

struct _relay_messenger {
    struct _JxtaEndpointMessenger messenger;

	JString* dest_pid;
	Jxta_endpoint_address* real_dest_address;

    _jxta_transport_relay *jxta_transport_relay;
};

typedef struct _relay_messenger _relay_messenger;


/************************************************************************
 * Takes a message and redirects its to the appropriate transport or
 * puts it into storage
 *
 *
 * @param  mes the relay messager
 * @param  msg the message to send
 * @return  the status of the function
 *************************************************************************/
static Jxta_status relay_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg)
{	
    _relay_messenger *relay_messenger = (_relay_messenger*)mes;
    
	Jxta_endpoint_address *destAddr = NULL;
	Jxta_endpoint_address *dest_relay = NULL;

	Jxta_message *cloned_message=NULL;

	RELAY_LOG_STATEMENT("************ Relay messenger_send called ***************\n");

	dest_relay=jxta_message_get_destination(msg);

	/* construct the destination address by taking the address part of the
	 * lease origin and parameters from the incoming relay address */
	destAddr = jxta_endpoint_address_new_2(
		jxta_endpoint_address_get_protocol_name(relay_messenger->real_dest_address),
		jxta_endpoint_address_get_protocol_address(relay_messenger->real_dest_address),
         jxta_endpoint_address_get_service_name(dest_relay), 
		 jxta_endpoint_address_get_service_params(dest_relay));

	jxta_message_set_destination(msg, destAddr);

	JXTA_OBJECT_RELEASE(destAddr);

	/* first add the message to the list of messages for this relay client*/
	/* clone the message to avoid any potential problems */
	cloned_message=jxta_message_clone(msg);
	if (!cloned_message) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to create a copy of the message\n");
		return JXTA_FAILED;
	}

	if (archive_put_message(relay_messenger->jxta_transport_relay,
							cloned_message,
							relay_messenger->dest_pid)!=JXTA_SUCCESS) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to add message to relay message vector in messenger\n");
		JXTA_OBJECT_RELEASE(cloned_message);
		return JXTA_FAILED;
	}

	apr_thread_mutex_lock(relay_messenger->jxta_transport_relay->messages_mutex);
	relay_messenger->jxta_transport_relay->messages_new_arrivals=TRUE;
	apr_thread_mutex_unlock(relay_messenger->jxta_transport_relay->messages_mutex);

	JXTA_OBJECT_RELEASE(cloned_message);

	apr_thread_cond_signal(relay_messenger->jxta_transport_relay->messages_cond);

	JXTA_OBJECT_RELEASE(dest_relay);
				
	return JXTA_SUCCESS;

}
static Jxta_status relay_send_all_messages(_jxta_transport_relay* self, JString* dest_pid) 
{
	_relay_lease* relay_lease = NULL;

	/*get the lease for the given pid*/
	relay_lease=find_relay_lease(self,dest_pid);

	return relay_send_all_messages_1(self,relay_lease);
}


static Jxta_status relay_send_all_messages_1(_jxta_transport_relay* self, _relay_lease* relay_lease)
{
	Jxta_transport *transport = NULL;
	/* the new transport over which to send the message */
	JxtaEndpointMessenger* endpoint_messenger=NULL;
	/*_stored_message* stored_message=NULL;*/
	Jxta_endpoint_address *src_addr = NULL;
	Jxta_endpoint_address *dest_addr = NULL;
    Jxta_boolean keep_sending=TRUE;
	Jxta_boolean delete_message=FALSE;
	_stored_message* head=NULL;
	_stored_message* next=NULL;

	if (!relay_lease) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to find a lease for the given pid\n");
		return JXTA_FAILED;
	}

	apr_thread_mutex_lock(relay_lease->mutex);

	head=relay_lease->stored_messages;

	/* if there are no messages */
	if (head==NULL) {
		apr_thread_mutex_unlock(relay_lease->mutex);
		return JXTA_SUCCESS;
	}

	next=(_stored_message*)linked_list_right(JXTA_LINKED_LIST_NODE(relay_lease->stored_messages));

	do {
		_stored_message* current=next;
		next=(_stored_message*)linked_list_right(JXTA_LINKED_LIST_NODE(current));

		if (!message_has_expired(current)) {

			dest_addr=jxta_message_get_destination(current->msg);

			/* get the transport based on the destination address */
			transport = jxta_endpoint_service_lookup_transport(self->endpoint, jxta_endpoint_address_get_protocol_name(dest_addr));
			/* get the end point messenger which will take care of sending the message */
			endpoint_messenger = jxta_transport_messenger_get(transport, dest_addr);
            
			if (endpoint_messenger) {
				JXTA_OBJECT_CHECK_VALID(endpoint_messenger);
				
				src_addr = jxta_transport_publicaddr_get(transport);

				jxta_message_set_source(current->msg, src_addr);
		        
				/* try to send the message */
				if (endpoint_messenger->jxta_send(endpoint_messenger, current->msg)==JXTA_SUCCESS) {
					jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay message sent over alternative transport\n");
					delete_message=TRUE;
                    /* since was able to send the message, reset the number of failures */
                    relay_lease->number_failed_sends=0;
				}
				else
				{
                    relay_lease->number_failed_sends++;
					jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "The transport picked by the relay was not able to send the message\n");
				} /* if (endpoint_messenger->jxta_send(endpoint_messenger, stored_message->msg)==JXTA_SUCCESS) */

				JXTA_OBJECT_RELEASE(src_addr);
				
			} else {
                /* if could not create a messenger for this message, not likely
                 * will be able to create it for next message, stop sending to
                 * this client */
                keep_sending=FALSE;
                relay_lease->number_failed_sends++;
				jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not create a messenger\n");
			} /* if (endpoint_messenger) */


			JXTA_OBJECT_RELEASE(dest_addr);

		} /* if (!message_has_expired(stored_message) */
		else {
			delete_message=TRUE;
		}

		if (delete_message) {
			linked_list_remove(JXTA_LINKED_LIST_NODE(current),JXTA_LINKED_LIST_NODE_PPTR(&relay_lease->stored_messages));
            relay_lease->total_stored--;

			JXTA_OBJECT_RELEASE(current);
		}

	} while (keep_sending && head!=next);

	relay_lease->messages_new_additions=FALSE;

	apr_thread_mutex_unlock(relay_lease->mutex);

	return JXTA_SUCCESS;
}

/************************************************************************
 * Generate a new relay messenger object
 *
 *
 * @param  
 * @return  the new object
 *************************************************************************/
static _relay_messenger* relay_messenger_new(_jxta_transport_relay* jxta_transport_relay, JString* dest_pid, Jxta_endpoint_address* real_dest_address)
{
	_relay_messenger* relay_messenger_obj;

    relay_messenger_obj = (_relay_messenger *) calloc(1, sizeof(_relay_messenger));

	if (NULL == relay_messenger_obj) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to allocate relay messenger\n");
        return NULL;
	}

	relay_messenger_obj->jxta_transport_relay=JXTA_OBJECT_SHARE(jxta_transport_relay);
	relay_messenger_obj->messenger.jxta_send=relay_messenger_send;
	relay_messenger_obj->dest_pid=JXTA_OBJECT_SHARE(dest_pid);
	relay_messenger_obj->real_dest_address=JXTA_OBJECT_SHARE(real_dest_address);

    JXTA_OBJECT_INIT(relay_messenger_obj, relay_messenger_delete, 0);

	return relay_messenger_obj;
}


/************************************************************************
 * Deallocate a relay messenger object
 *
 *
 * @param  
 * @return 
 *************************************************************************/
static void relay_messenger_delete(Jxta_object* obj)
{
    _relay_messenger *relay_messenger_obj = (_relay_messenger *) obj;

    if (NULL == relay_messenger_obj)
        return;

	JXTA_OBJECT_RELEASE(relay_messenger_obj->dest_pid);
	JXTA_OBJECT_RELEASE(relay_messenger_obj->real_dest_address);
	JXTA_OBJECT_RELEASE(relay_messenger_obj->jxta_transport_relay);

	free(relay_messenger_obj);
}


/************************************************************************
 * The function gets called when the end point transport wants to send
 * a message through a relay
 *
 *
 * @param  t relay_transport
 * @return 
 *************************************************************************/
static JxtaEndpointMessenger *get_messenger(Jxta_transport * t, Jxta_endpoint_address * dest)
{
	_relay_lease* tmp_relay_lease=NULL;
	const char* dest_address;
	_jxta_transport_relay *self = PTValid(t, _jxta_transport_relay);
	JString* local_jstring=NULL;

	/* if not a relay server don't return a messenger */
	if (!self->Is_Server) {
		return NULL;
	}

	jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay get_messenger called\n");

	RELAY_LOG_STATEMENT("************ Relay get_messenger called ***************\n");


	/* REMOVE REMOVE REMOVE */
	/* changed to force relay to be the default transport */
	/*t->metric=10;*/

	dest_address=jxta_endpoint_address_get_protocol_address(dest);
	local_jstring=jstring_new_2(dest_address);

	/*need to check whether this destination has a lease on the relay server*/
	tmp_relay_lease=find_relay_lease(self,local_jstring);

	

	/* if found a lease return messenger */
	if (tmp_relay_lease) {
		/* make sure the lease is not expired */
		if (lease_is_valid(tmp_relay_lease)) {
			return (JxtaEndpointMessenger*)relay_messenger_new(self,tmp_relay_lease->lease_requestor_id,tmp_relay_lease->lease_requestor_address);
		} else {
			lease_delete(self,local_jstring);
		}
	}

	JXTA_OBJECT_RELEASE(local_jstring);

	return NULL;
}

static void relay_entry_delete(Jxta_object * addr)
{
    _jxta_peer_relay_entry *self = (_jxta_peer_relay_entry *) addr;

    relay_entry_destruct(self);

    free(self);
}

static _jxta_peer_relay_entry *relay_entry_construct(_jxta_peer_relay_entry * self)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Constructing ...\n");

    self = (_jxta_peer_relay_entry *) peer_entry_construct((_jxta_peer_entry *) self);

    if (NULL != self) {
        self->thisType = "_jxta_peer_relay_entry";
        self->is_connected = FALSE;
        self->try_connect = FALSE;
        self->connectTime = 0;
        self->connectRetryDelay = 0;
    }

    return self;
}

static void relay_entry_destruct(_jxta_peer_relay_entry * self)
{
    peer_entry_destruct((_jxta_peer_entry *) self);
}

static _jxta_peer_relay_entry *relay_entry_new(void)
{
    _jxta_peer_relay_entry *self = (_jxta_peer_relay_entry *) malloc(sizeof(_jxta_peer_relay_entry));
    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(self, 0, sizeof(_jxta_peer_relay_entry));
    JXTA_OBJECT_INIT(self, relay_entry_delete, 0);

    return relay_entry_construct(self);
}

/*
 * Init the Relay service
 */
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    apr_status_t res;
    char *address_str;
    Jxta_id *id;
    JString *uniquePid;
    Jxta_id *pgid;
    JString *uniquePGid;
    const char *tmp;
    _jxta_transport_relay *self = PTValid(module, _jxta_transport_relay);
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    JString *val = NULL;
    Jxta_vector *relays = NULL;
    JString *relay = NULL;
    JString *mid = NULL;

    size_t sz;
    size_t i;
    Jxta_RelayAdvertisement *rla;

#ifndef WIN32
    struct sigaction sa;

    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Initializing ...\n");

    /* Allocate a pool for our apr needs */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS)
        return res;

    /*
     * Create our mutex.
     */
    res = apr_thread_mutex_create(&(self->stop_mutex), APR_THREAD_MUTEX_DEFAULT, self->pool);
    if (res != APR_SUCCESS)
        return res;

    res = apr_thread_cond_create(&(self->stop_cond), self->pool);
    if (res != APR_SUCCESS) {
        return res;
    }

    res = apr_thread_mutex_create(&(self->messages_mutex), APR_THREAD_MUTEX_DEFAULT, self->pool);
    if (res != APR_SUCCESS)
        return res;

    /*res = apr_thread_mutex_create(&(self->messages_access_mutex), APR_THREAD_MUTEX_DEFAULT, self->pool);
    if (res != APR_SUCCESS)
        return res;*/

    res = apr_thread_cond_create(&(self->messages_cond), self->pool);
    if (res != APR_SUCCESS) {
        return res;
    }


    /*
     * following falls-back on backdoor config if needed only.
     * So it can be just be removed when nolonger usefull.
     */
    self->Is_Client = FALSE;
    self->Is_Server = FALSE;

    /*
     * Extract our configuration for the config adv.
     */
    jxta_PG_get_configadv(group, &conf_adv);
    if (conf_adv == NULL) {
        return JXTA_CONFIG_NOTFOUND;
    }

    jxta_PA_get_Svc_with_id(conf_adv, assigned_id, &svc);
    JXTA_OBJECT_RELEASE(conf_adv);

    if (svc == NULL)
        return JXTA_NOT_CONFIGURED;

    rla = jxta_svc_get_RelayAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);

    if (rla == NULL) {
        return JXTA_CONFIG_NOTFOUND;
    }

    /*
     * FIXME - tra@jxta.org 20040121 : for now, we ignore the following
     * relay config items: 
     *          <ServerMaximumClients>
     *          <ServerLeaseInSeconds>
     *          <ClientMaximumServers>
     *          <ClientLeaseInSeconds>
     *          <ClientQueueSize>
     */

	/* set the default values */
	self->max_leases_allowed=DEFAULT_MAX_ALLOWED_LEASES;

    val = jxta_RelayAdvertisement_get_IsServer(rla);
    if (strcmp(jstring_get_string(val), "true") == 0) {
        self->Is_Server = TRUE;
    } else {
        self->Is_Server = FALSE;
    }
    JXTA_OBJECT_RELEASE(val);

    val = jxta_RelayAdvertisement_get_IsClient(rla);
    if (strcmp(jstring_get_string(val), "true") == 0) {
        self->Is_Client = TRUE;
    } else {
        self->Is_Client = FALSE;
    }
    JXTA_OBJECT_RELEASE(val);

    relays = jxta_RelayAdvertisement_get_HttpRelay(rla);
    if (jxta_vector_clone(relays, &(self->HttpRelays), 0, 20) != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(rla);
        return JXTA_CONFIG_NOTFOUND;
    }
    JXTA_OBJECT_RELEASE(relays);

    relays = jxta_RelayAdvertisement_get_TcpRelay(rla);
    JXTA_OBJECT_RELEASE(rla);
    if (jxta_vector_clone(relays, &(self->TcpRelays), 0, 20) != JXTA_SUCCESS) {
        return JXTA_CONFIG_NOTFOUND;
    }
    JXTA_OBJECT_RELEASE(relays);

    jxta_PG_get_endpoint_service(group, &(self->endpoint));
    self->group = group;

    /*
     * Only the unique portion of the peerid is used by the relay Transport.
     */
    jxta_PG_get_PID(group, &id);
    uniquePid = NULL;
    jxta_id_get_uniqueportion(id, &uniquePid);
    tmp = jstring_get_string(uniquePid);
    self->peerid = malloc(strlen(tmp) + 1);
    if (self->peerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(id);
        JXTA_OBJECT_RELEASE(uniquePid);
        return JXTA_NOMEM;
    }
    strcpy(self->peerid, tmp);
    JXTA_OBJECT_RELEASE(uniquePid);
    JXTA_OBJECT_RELEASE(id);

    /*
     * get the unique portion of the group id
     */
    jxta_PG_get_GID(group, &pgid);
    jxta_id_get_uniqueportion(pgid, &uniquePGid);
    tmp = jstring_get_string(uniquePGid);
    self->groupid = malloc(strlen(tmp) + 1);
    if (self->groupid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(uniquePGid);
        JXTA_OBJECT_RELEASE(pgid);
        return JXTA_NOMEM;
    }
    strcpy(self->groupid, tmp);
    JXTA_OBJECT_RELEASE(uniquePGid);
    JXTA_OBJECT_RELEASE(pgid);

    /*
     * Save our assigned service id
     */
    jxta_id_get_uniqueportion(assigned_id, &mid);
    self->assigned_id = malloc(strlen(jstring_get_string(mid)) + 1);
    if (self->assigned_id == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(mid);
        return JXTA_NOMEM;
    }
    strcpy(self->assigned_id, jstring_get_string(mid));
    JXTA_OBJECT_RELEASE(mid);

    address_str = apr_psprintf(self->pool, "relay://%s", self->peerid);

    self->address = jxta_endpoint_address_new(address_str);

    if (self->address == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not generate valid local address");
        return JXTA_NOMEM;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay Service Configuration:\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "      Address=%s://%s\n",
                    jxta_endpoint_address_get_protocol_name(self->address),
                    jxta_endpoint_address_get_protocol_address(self->address));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "      IsClient=%d\n", self->Is_Client);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "      IsServer=%d\n", self->Is_Server);

    sz = jxta_vector_size(self->HttpRelays);
    for (i = 0; i < sz; ++i) {
        jxta_vector_get_object_at(self->HttpRelays, JXTA_OBJECT_PPTR(&relay), i);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "http://%s\n", jstring_get_string(relay));
        JXTA_OBJECT_RELEASE(relay);
    }

    sz = jxta_vector_size(self->TcpRelays);
    for (i = 0; i < sz; ++i) {
        jxta_vector_get_object_at(self->TcpRelays, JXTA_OBJECT_PPTR(&relay), i);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "tcp://%s\n", jstring_get_string(relay));
        JXTA_OBJECT_RELEASE(relay);
    }

	self->messages_new_arrivals=FALSE;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Initialized\n");
    return JXTA_SUCCESS;
}

/*
 * Start Module
 */
static Jxta_status start(Jxta_module * module, const char *argv[])
{
    _jxta_transport_relay *self = PTValid(module, _jxta_transport_relay);
    Jxta_status rv;

    /*
     * Check if the relay service is configured as a client or a server, if not
     * just skip starting the service
     */
    if (!self->Is_Client && !self->Is_Server) {
        return JXTA_SUCCESS;
    }

    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    /* 
     *  we need to register the relay endpoint callback
     */
    rv = endpoint_service_add_recipient(self->endpoint, &self->ep_cookie, self->assigned_id, NULL, relay_transport_incoming_cb,
                                        self);

    /*
     * Go ahead an start the Relay connect thread
	 * start only for clients
     */
	if (self->Is_Client) {
    apr_thread_create(&self->thread, NULL,  /* no attr */
                      connect_relay_thread, (void *) self, (apr_pool_t *) self->pool);

		self->leases=NULL;
	}

	/* start the message sending thread */
	if (self->Is_Server) {
		JString* local_jstring = NULL;
		Jxta_discovery_service *discovery = NULL;
		Jxta_RdvAdvertisement* rdv_advertisement = NULL;

		self->leases=jxta_vector_new(10);

		apr_thread_create(&self->messages_thread, NULL,  /* no attr */
						send_messages_thread, (void *) self, (apr_pool_t *) self->pool);

		/* publish the relay service advertisement */
		jxta_PG_get_discovery_service(self->group, &discovery);
		
		if (!discovery) {
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to get discovery service\n");
			return JXTA_FAILED;
		}

		/* build the new advertisement */
		local_jstring=jstring_new_2(self->assigned_id);
		rdv_advertisement=jxta_relay_build_rdva(self->group,local_jstring);
		JXTA_OBJECT_RELEASE(local_jstring);

		if (discovery_service_publish(discovery,
				(Jxta_advertisement *) rdv_advertisement,
				DISC_ADV,
				DEFAULT_LIFETIME,
				DEFAULT_EXPIRATION)!=JXTA_SUCCESS) {

			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to publish relay advertisement\n");

		}

		JXTA_OBJECT_RELEASE(rdv_advertisement);
		JXTA_OBJECT_RELEASE(discovery);

		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initialized Relay server\n");

	}

    return JXTA_SUCCESS;
}

/*
 * Stop Module         
 */
static void stop(Jxta_module * module)
{
    apr_status_t status;

    _jxta_transport_relay *self = PTValid(module, _jxta_transport_relay);

    if (!self->Is_Client && !self->Is_Server) {
        return;
    }

    endpoint_service_remove_recipient(self->endpoint, self->ep_cookie);
    jxta_endpoint_service_remove_transport(self->endpoint, (Jxta_transport *) self);

	/* if connect to relay thread is running stop it */
	if (self->running == TRUE) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Signal connect to relay thread to exit...\n");
    apr_thread_mutex_lock(self->stop_mutex);
    self->running = FALSE;
    apr_thread_cond_signal(self->stop_cond);
    apr_thread_mutex_unlock(self->stop_mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Waiting thread to exit...\n");
    apr_thread_join(&status, self->thread);
	}

	/* if messages thread is running stop it */
	if (self->messages_thread_running == TRUE) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Signal messages thread to exit...\n");
		apr_thread_mutex_lock(self->messages_mutex);
		self->messages_thread_running = FALSE;
		apr_thread_cond_signal(self->messages_cond);
		apr_thread_mutex_unlock(self->messages_mutex);

		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Waiting thread to exit...\n");
		apr_thread_join(&status, self->messages_thread);
	}

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay stopped.\n");
}

/*
 * Relay address
 */
static JString *name_get(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return jstring_new_2("relay");
}

/*
 * get Address
 */
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    JXTA_OBJECT_SHARE(self->address);
    return self->address;
}

/*
 * Ping
 */
static Jxta_boolean ping(Jxta_transport * relay, Jxta_endpoint_address * addr)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    /* FIXME 20050119 If the address requested is the relay we should return true */

    return FALSE;
}

/*
 * Propagate
 */
static void propagate(Jxta_transport * relay, Jxta_message * msg, const char *service_name, const char *service_params)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    /* this code intentionally left blank */
}

/*
 *  Allow overload 
 */
static Jxta_boolean allow_overload_p(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return FALSE;
}

/*
 * Allow routing    
 */
static Jxta_boolean allow_routing_p(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return TRUE;
}

/*
 * Connection oriented
 */
static Jxta_boolean connection_oriented_p(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return TRUE;
}

/*
 * Construct relay service
 */
_jxta_transport_relay *jxta_transport_relay_construct(Jxta_transport * relay, Jxta_transport_relay_methods const *methods)
{
    _jxta_transport_relay *self = NULL;

    PTValid(methods, Jxta_transport_methods);
    self = (_jxta_transport_relay *) jxta_transport_construct(relay, (Jxta_transport_methods const *) methods);
    self->_super.direction = JXTA_BIDIRECTION;

    if (NULL != self) {
        self->thisType = "_jxta_transport_relay";
        self->address = NULL;
        self->peerid = NULL;
        self->groupid = NULL;
        self->endpoint = NULL;
        self->group = NULL;
        self->stop_mutex = NULL;
        self->stop_cond = NULL;
		self->running=FALSE;
        self->pool = NULL;
        self->HttpRelays = NULL;
        self->TcpRelays = NULL;
		/* server variables */
		self->leases=NULL;
		self->messages_cond=NULL;
		self->messages_mutex=NULL;
		/*self->messages_access_mutex;*/
		self->messages_thread=NULL;
		self->messages_thread_running=FALSE;
        self->peers = jxta_vector_new(1);
        if (self->peers == NULL) {
            return NULL;
        }
    }

    return self;
}

/*
 * The destructor is never called explicitly (except by subclass' code).
 * It is called by the free function (installed by the allocator) when the object
 * becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
void jxta_transport_relay_destruct(_jxta_transport_relay * self)
{
    if (self->HttpRelays != NULL) {
        JXTA_OBJECT_RELEASE(self->HttpRelays);
    }
    if (self->TcpRelays != NULL) {
        JXTA_OBJECT_RELEASE(self->TcpRelays);
    }

    JXTA_OBJECT_RELEASE(self->peers);

    self->group = NULL;

    if (self->address != NULL) {
        JXTA_OBJECT_RELEASE(self->address);
    }
    if (self->endpoint) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }
    if (self->assigned_id) {
        free(self->assigned_id);
    }
    if (self->peerid != NULL) {
        free(self->peerid);
    }
    if (self->groupid != NULL) {
        free(self->groupid);
    }

	/* Relay server */

	if (self->leases!= NULL) {
		JXTA_OBJECT_RELEASE(self->leases);
    }
	/*****************/

    apr_thread_cond_destroy(self->stop_cond);
    apr_thread_mutex_destroy(self->stop_mutex);

	apr_thread_cond_destroy(self->messages_cond);
    apr_thread_mutex_destroy(self->messages_mutex);
	/*apr_thread_mutex_destroy(self->messages_access_mutex);*/

    if (self->pool) {
        apr_pool_destroy(self->pool);
    }

    jxta_transport_destruct((Jxta_transport *) self);

    self->thisType = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destruction finished\n");
}

/*
 * De-instantiate an instance of the Relay transport   
 */
static void relay_free(Jxta_object * obj)
{
    jxta_transport_relay_destruct((_jxta_transport_relay *) obj);

    free(obj);
}

/*
 * Instantiate a new instance of the Relay transport
 */
Jxta_transport_relay *jxta_transport_relay_new_instance(void)
{
    _jxta_transport_relay *self = (_jxta_transport_relay *) malloc(sizeof(_jxta_transport_relay));
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }
    memset(self, 0, sizeof(_jxta_transport_relay));
    JXTA_OBJECT_INIT(self, relay_free, NULL);
    if (!jxta_transport_relay_construct((Jxta_transport *) self, &JXTA_TRANSPORT_RELAY_METHODS)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }

    return (Jxta_transport_relay *) self;
}

/**
 ** This is the generic message handler of the JXTA Relay Service
 ** protocol. Since this implementation is designed for an edge peer,
 ** it only has to support:
 **
 **   - get the lease from the relay server.
 **   - get a disconnect from the relay server.
 **
 **  Added changes to implement the server
 **/
#define RELAY_LEASE_RESPONSE_NO_NS "response"
static Jxta_status JXTA_STDCALL relay_transport_incoming_cb(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    _jxta_transport_relay *self = PTValid(arg, _jxta_transport_relay);
    Jxta_message_element *el = NULL;
    JString *response = NULL;
    Jxta_bytevector *bytes = NULL;
    char *res;
    _jxta_peer_relay_entry *peer = NULL;
    unsigned int i = 0;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_boolean connected = FALSE;

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(self);

	/* try to process the message as a lease request */
	if (self->Is_Server) {
		check_for_lease_request(msg,self);
	}
	
	if (self->Is_Client) {
    for (i = 0; i < jxta_vector_size(self->peers); ++i) {
        status = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
        if (status != JXTA_SUCCESS) {
            /* We should print a LOG here. Just continue for the time being */
            continue;
        }

        if (peer->is_connected) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Drop Relay lease response as we already have a Relay connection\n");
            connected = TRUE;
        } else {
            peer->connectTime = 0;
            peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
        }
        JXTA_OBJECT_RELEASE(peer);
    }
    /*
     * we are already connected
     */
    if (connected) {
        return JXTA_SUCCESS;
    }

		jxta_message_get_element_2(msg, RELAY_NS, RELAY_LEASE_RESPONSE_NO_NS, &el);

    if (el) {
        /*
         * check for the type of responses we got
         */
        bytes = jxta_message_element_get_value(el);
        response = jstring_new_3(bytes);
        if (response == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            JXTA_OBJECT_RELEASE(el);
            JXTA_OBJECT_RELEASE(bytes);
            return JXTA_NOMEM;
        }
        res = (char *) jstring_get_string(response);

        if (strcmp(res, RELAY_RESPONSE_CONNECTED) == 0) {
            process_connected_reply(self, msg);
        } else if (strcmp(res, RELAY_RESPONSE_DISCONNECTED) == 0) {
            process_disconnected_reply(self, msg);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay response not supported: %s\n", res);
        }

        JXTA_OBJECT_RELEASE(response);
        JXTA_OBJECT_RELEASE(bytes);
        JXTA_OBJECT_RELEASE(el);

    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No relay response element\n");
    }
	}

    return JXTA_SUCCESS;
}


Jxta_boolean is_contain_address(Jxta_endpoint_address * addr, Jxta_RdvAdvertisement * relay)
{
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_AccessPointAdvertisement *apa = NULL;
    Jxta_vector *dest = NULL;
    Jxta_boolean res = FALSE;
    unsigned int i = 0;
    JString *js_addr = NULL;
    char *saddr = NULL;

    route = jxta_RdvAdvertisement_get_Route(relay);
    apa = jxta_RouteAdvertisement_get_Dest(route);
    dest = jxta_AccessPointAdvertisement_get_EndpointAddresses(apa);
    saddr = jxta_endpoint_address_to_string(addr);
    if (saddr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
        JXTA_OBJECT_RELEASE(route);
        JXTA_OBJECT_RELEASE(apa);
        JXTA_OBJECT_RELEASE(dest);
        return res;
    }

    for (i = 0; i < jxta_vector_size(dest); i++) {
        jxta_vector_get_object_at(dest, JXTA_OBJECT_PPTR(&js_addr), i);
        if (strncmp(saddr, jstring_get_string(js_addr), (strlen(saddr) - 2)) == 0) {
            res = TRUE;
            JXTA_OBJECT_RELEASE(js_addr);
            break;
        }

        JXTA_OBJECT_RELEASE(js_addr);
    }

    JXTA_OBJECT_RELEASE(apa);
    JXTA_OBJECT_RELEASE(route);
    JXTA_OBJECT_RELEASE(dest);
    free(saddr);
    return res;
}

/*
 * update relay connection state
 */
static void update_relay_peer_connection(_jxta_transport_relay * self, Jxta_RdvAdvertisement * relay, Jxta_time_diff lease)
{
    unsigned int i;
    Jxta_status res;
    _jxta_peer_relay_entry *peer = NULL;

    for (i = 0; i < jxta_vector_size(self->peers); i++) {

        res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get peer from vector peers\n");
            break;
        }

        /*
         * check if this entry correspond to our entry
         */
        if (is_contain_address(((_jxta_peer_entry *) peer)->address, relay)) {

            /*
             *  Mark the peer entry has connected
             */
            jxta_peer_lock((Jxta_peer *) peer);

			peer->_super.peerid=jxta_RdvAdvertisement_get_RdvPeerId(relay);			

            /*
             * Reset the flag to our last succeeded lease renewal state
             */
            peer->is_connected = TRUE;
            jxta_peer_set_expires((Jxta_peer *) peer, jpr_time_now() + lease);
            peer->connectTime = jxta_peer_get_expires((Jxta_peer *) peer) - RELAY_LEASE_RENEWAL_DELAY;

            jxta_endpoint_service_set_relay(self->endpoint,
                                            jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                            jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));

            jxta_peer_unlock((Jxta_peer *) peer);
            JXTA_OBJECT_RELEASE(peer);
            return;
        }

        /* We are done with this peer. Let release it */
        JXTA_OBJECT_RELEASE(peer);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed to find relay peer entry\n");
}

/*
 * extract Relay advertisement (or RDV advertisement this is the same)
 */
Jxta_RdvAdvertisement *extract_relay_adv(Jxta_message * msg)
{
    Jxta_message_element *el = NULL;
    Jxta_bytevector *bytes = NULL;
    JString *value = NULL;
    Jxta_RdvAdvertisement *relayAdv = NULL;

    if (jxta_message_get_element_1(msg, RELAY_RESPONSE_ADV, &el) == JXTA_SUCCESS) {
        if (el) {
            bytes = jxta_message_element_get_value(el);
            value = jstring_new_3(bytes);
            relayAdv = jxta_RdvAdvertisement_new();
            if (value == NULL || relayAdv == NULL) {
                JXTA_OBJECT_RELEASE(el);
                JXTA_OBJECT_RELEASE(bytes);
                JXTA_OBJECT_RELEASE(value);
                return NULL;
            }
            jxta_RdvAdvertisement_parse_charbuffer(relayAdv, jstring_get_string(value), jstring_length(value));

            JXTA_OBJECT_RELEASE(el);
            JXTA_OBJECT_RELEASE(bytes);
            JXTA_OBJECT_RELEASE(value);
        }
    }
    return relayAdv;
}

/**
 * extract Relay lease
 */
static Jxta_time extract_lease_value(Jxta_message * msg)
{
    Jxta_time_diff lease = 0;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *value = NULL;
    int length = 0;
    char *bytes = NULL;

    if (jxta_message_get_element_1(msg, RELAY_LEASE_ELEMENT, &el) == JXTA_SUCCESS) {
        if (el) {
            value = jxta_message_element_get_value(el);
            length = jxta_bytevector_size(value);
            bytes = malloc(length + 1);
            if (bytes == NULL) {
                JXTA_OBJECT_RELEASE(value);
                JXTA_OBJECT_RELEASE(el);
                return 0;
            }

            jxta_bytevector_get_bytes_at(value, (unsigned char *) bytes, 0, length);
            bytes[length] = 0;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "did not find lease element in Lease Response \n");
            return 0;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed when trying to find lease element in Lease Response \n");
        return 0;
    }

    /*
     * convert to (long long) jxta_time
     */
#ifndef WIN32
    lease = atoll(bytes);
#else
    sscanf(bytes, "%I64d", &lease);
#endif

    /*
     * Convert the lease in micro-seconds
     */
    if (lease < 0) {
        lease = 0;
    }

    free(bytes);
    bytes = NULL;
    JXTA_OBJECT_RELEASE(value);
    value = NULL;
    JXTA_OBJECT_RELEASE(el);

    return lease;
}


/**
 ** Process an incoming relay request response
 **/
static void process_connected_reply(_jxta_transport_relay * self, Jxta_message * msg)
{
    Jxta_time_diff lease = 0;
    Jxta_RdvAdvertisement *relayadv = NULL;

    lease = extract_lease_value(msg);

    if (0 == lease) {
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "got relay lease: %" APR_INT64_T_FMT " milliseconds\n", lease);
    relayadv = extract_relay_adv(msg);

    if (relayadv == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No RdvAdvertisement in relay response\n");
        return;
    }

    jxta_PG_add_relay_address(self->group, relayadv);
    update_relay_peer_connection(self, relayadv, lease);
    JXTA_OBJECT_RELEASE(relayadv);
}

/**
 ** Process a relay disconnect request
 **/
static void process_disconnected_reply(_jxta_transport_relay * self, Jxta_message * ms)
{
	unsigned int i;
	Jxta_message_element *el=NULL;
	Jxta_bytevector *bytes = NULL;
	JString *el_jstring = NULL;
	Jxta_endpoint_address *relay_address=NULL;


    //jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Receive Relay disconnect msg not fully implemented\n");

	jxta_message_get_element_2(ms, JXTA_NS, ENDPOINT_SOURCE_ADDRESS, &el);

	if (!el) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Can't find source element in message\n");
		return;
	}

	bytes = jxta_message_element_get_value(el);
	el_jstring = jstring_new_3(bytes);
	relay_address=jxta_endpoint_address_new_1(el_jstring);

	JXTA_OBJECT_RELEASE(bytes);
	JXTA_OBJECT_RELEASE(el_jstring);

	if (!relay_address) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Can't decode source element in message\n");
		return;
	}

	/* find the peer from whom came the disconnect message */
	for (i=0; i<jxta_vector_size(self->peers); ++i) {
		_jxta_peer_relay_entry *seed;

		if (jxta_vector_get_object_at(self->peers,JXTA_OBJECT_PPTR(&seed),i)!=JXTA_SUCCESS) {
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Can't retrieve a peer from vector\n");
			return;
		}

		/* if this is the peer marked is as not connected and remove it from PG advertisement */
		if (jxta_endpoint_address_equals(seed->_super.address,relay_address)) {
			seed->is_connected=FALSE;
			seed->connectTime = 0;
			seed->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
			jxta_PG_remove_relay_address(self->group,seed->_super.peerid);

			/* move the seed to the end of peer list */
			jxta_vector_move_element_last(self->peers,i);
			JXTA_OBJECT_RELEASE(seed);
		}

		JXTA_OBJECT_RELEASE(seed);

	}

}


static Jxta_boolean client_has_peer_entry(_jxta_transport_relay *self, Jxta_endpoint_address *endpoint_address)
{
	unsigned int i;

	for (i=0; i<jxta_vector_size(self->peers); ++i) {
		Jxta_peer *seed;
			
		if (jxta_vector_get_object_at(self->peers,JXTA_OBJECT_PPTR(&seed),i)!=JXTA_SUCCESS) {
			return FALSE;
		}

		if (jxta_endpoint_address_equals(endpoint_address,seed->address)) {
			JXTA_OBJECT_RELEASE(seed);
			return TRUE;
		}

		JXTA_OBJECT_RELEASE(seed);
	}

	return FALSE;
}

/************************************************************************
* Searches for remote relay advertisements and if found adds them to the
* list of peers
*
* @param self the relay
* @param number_of_runs number of times to rerun all tests
* @param continue_until_discovered searches until at least one advertisement is found
* @return success if at least one advertisement was discovered
*************************************************************************/
static Jxta_status search_remote_relay_advertisements(_jxta_transport_relay *self, Jxta_boolean continue_until_discovered)
{
	Jxta_discovery_service *discovery=NULL;
	long responses = 10;
	Jxta_vector* search_results;
	Jxta_boolean looked_once=FALSE;
	unsigned int i;

	jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Start remote discovery of relay advertisements\n");

	jxta_PG_get_discovery_service(self->group, &discovery);

	while(self->running && (!looked_once || ((jxta_vector_size(self->peers) == 0) && continue_until_discovered))) {
		JString *attr = jstring_new_2("RdvServiceName");
		JString *value = jstring_new_2("uuid-DEADBEEFDEAFBABAFEEDBABE0000000F05");

		looked_once=TRUE;

		/* search for a relay advertisement */
		discovery_service_get_remote_advertisements(discovery,NULL,DISC_ADV,
			(char *) jstring_get_string(attr),
			(char *) jstring_get_string(value),responses,NULL);

		/* wait 40 seconds */
        apr_thread_mutex_lock(self->stop_mutex);
        apr_thread_cond_timedwait(self->stop_cond, self->stop_mutex, 40*1000*1000);
        apr_thread_mutex_unlock(self->stop_mutex);

		/* get them from local cache */
		discovery_service_get_local_advertisements(discovery,
			DISC_ADV,
			(char *) jstring_get_string(attr),
			(char *) jstring_get_string(value), &search_results);

        JXTA_OBJECT_RELEASE(attr);
        JXTA_OBJECT_RELEASE(value);

		if (search_results!=NULL) {

			/* go though the search results */
			for (i = 0; i < jxta_vector_size(search_results); ++i) {
				Jxta_RdvAdvertisement *advertisement;
				Jxta_RouteAdvertisement *route;
				Jxta_AccessPointAdvertisement *access_point;
				Jxta_vector *endpoint_addresses;
				unsigned int j;

				jxta_vector_get_object_at(search_results, JXTA_OBJECT_PPTR(&advertisement), i);

				route = jxta_RdvAdvertisement_get_Route(advertisement);

				access_point=jxta_RouteAdvertisement_get_Dest(route);

				endpoint_addresses=jxta_AccessPointAdvertisement_get_EndpointAddresses(access_point);

				for (j = 0; j < jxta_vector_size(endpoint_addresses); ++j) {
					Jxta_endpoint_address *endpoint_address;
					JString *endpoint_address_str;


					jxta_vector_get_object_at(endpoint_addresses, JXTA_OBJECT_PPTR(&endpoint_address_str), j);
					endpoint_address=jxta_endpoint_address_new(jstring_get_string(endpoint_address_str));

					/* if this is a tcp or http address, add it to the list of peers*/
					if (!strcmp(jxta_endpoint_address_get_protocol_name(endpoint_address),"tcp") ||
						!strcmp(jxta_endpoint_address_get_protocol_name(endpoint_address),"http")) {
							if (!client_has_peer_entry(self,endpoint_address)) {
								Jxta_peer *seed = (Jxta_peer *) relay_entry_new();

								jxta_peer_set_address(seed, endpoint_address);

								jxta_vector_add_object_last(self->peers, (Jxta_object *) seed);

								JXTA_OBJECT_RELEASE(seed);
							}
						}


						JXTA_OBJECT_RELEASE(endpoint_address);
						JXTA_OBJECT_RELEASE(endpoint_address_str);	

				}

				JXTA_OBJECT_RELEASE(endpoint_addresses);
				JXTA_OBJECT_RELEASE(advertisement);
				JXTA_OBJECT_RELEASE(route);
				JXTA_OBJECT_RELEASE(access_point);
			} 

            JXTA_OBJECT_RELEASE(search_results);
            search_results=NULL;
		}
	}

    if (discovery) {
        JXTA_OBJECT_RELEASE(discovery);
    }

	if (jxta_vector_size(self->peers)>0) {
		return JXTA_SUCCESS;
	} else {
		return JXTA_ITEM_NOTFOUND;
	}
}

/**
 ** connect_thread_main is the entry point of the thread that regularly tries to
 ** connect to a relay.
 **/
static void *APR_THREAD_FUNC connect_relay_thread(apr_thread_t * thread, void *arg)
{
    _jxta_transport_relay *self = PTValid(arg, _jxta_transport_relay);
    Jxta_status res;
    Jxta_peer *seed;
    unsigned int i;
    int nbOfConnectedPeers = 0;
    size_t sz;
    Jxta_boolean connect_seed = FALSE;
    JString *addr;
    Jxta_endpoint_address *addr_e;
    JString *addr_a;
	Jxta_time last_remote_relay_search=0;
    /* Mark the service as running now. */
    self->running = TRUE;

    /* initial connect to relay seed */

    for (i = 0; i < jxta_vector_size(self->TcpRelays); i++) {
        seed = (Jxta_peer *) relay_entry_new();
        jxta_vector_get_object_at(self->TcpRelays, JXTA_OBJECT_PPTR(&addr), i);
        addr_a = jstring_new_2("tcp://");
        jstring_append_1(addr_a, addr);
        addr_e = jxta_endpoint_address_new((char *) jstring_get_string(addr_a));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay seed address %s\n", jstring_get_string(addr_a));
        jxta_peer_set_address(seed, addr_e);
        JXTA_OBJECT_RELEASE(addr);
        JXTA_OBJECT_RELEASE(addr_e);
        JXTA_OBJECT_RELEASE(addr_a);
        jxta_vector_add_object_last(self->peers, (Jxta_object *) seed);
        JXTA_OBJECT_RELEASE(seed);
    }

    for (i = 0; i < jxta_vector_size(self->HttpRelays); i++) {
        seed = (Jxta_peer *) relay_entry_new();
        jxta_vector_get_object_at(self->HttpRelays, JXTA_OBJECT_PPTR(&addr), i);
        addr_a = jstring_new_2("http://");
        jstring_append_1(addr_a, addr);
        addr_e = jxta_endpoint_address_new((char *) jstring_get_string(addr_a));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay seed address %s\n", (char *) jstring_get_string(addr_a));
        jxta_peer_set_address(seed, addr_e);
        JXTA_OBJECT_RELEASE(addr);
        JXTA_OBJECT_RELEASE(addr_e);
        JXTA_OBJECT_RELEASE(addr_a);
        jxta_vector_add_object_last(self->peers, (Jxta_object *) seed);
        JXTA_OBJECT_RELEASE(seed);
    }

	
	/* if not seeded with any peers look for advertisements on the network */
    if ((jxta_vector_size(self->peers) == 0)) {

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not seeded with any valid relay addresses\n");

		if (search_remote_relay_advertisements(self,TRUE)!=JXTA_SUCCESS) {
        apr_thread_exit(thread, JXTA_SUCCESS);
        return NULL;
    }

		last_remote_relay_search=jpr_time_now();

    }

    jxta_vector_shuffle(self->peers);

    /* Main loop */
    while (self->running) {
        _jxta_peer_relay_entry *peer;

        /**
         ** Do the following for each of the relay peers:
         **        - check if a renewal of the lease has to be sent (if the peer is connected)
         **        - check if an attempt to connect has to be issued (if the peer is not connected)
         **        - check if the peer has to be disconnected (the lease has not been renewed)
         **/
        nbOfConnectedPeers = 0;

        for (i = 0; i < jxta_vector_size(self->peers); ++i) {
            res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }
            if (peer->is_connected) {
                check_relay_lease(self, peer);
            }
            /**
             ** Since check_peer_lease() may have changed the connected state,
             ** we need to re-test the state.
             **/
            if (peer->is_connected) {
                ++nbOfConnectedPeers;
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /**
         ** We need to check the peers in two separated passes
         ** because:
         **          - check_relay_lease() may change the disconnected state
         **          - the total number of relay connected peers may change
         **            change the behavior of check_peer_connect()
         **/

        for (i = 0; i < jxta_vector_size(self->peers); ++i) {

            res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get peer from vector peers\n");
                continue;
            }
            if (!peer->is_connected && (nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS)) {
                check_relay_connect(self, peer, nbOfConnectedPeers);
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /**
         ** Wait for a while until the next iteration.
         **
         ** We want to have a better responsiveness (latency) of the mechanism
         ** to connect to rendezvous when there isn't enough rendezvous connected.
         ** The time to wait could be as small as we want, but the smaller it is, the
         ** more CPU it will use. Having two different periods for checking connection
         ** is a compromise for latency versus performance.
         **/

        if (nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS) {
            if (connect_seed) {
                apr_thread_mutex_lock(self->stop_mutex);
                res = apr_thread_cond_timedwait(self->stop_cond, self->stop_mutex, RELAY_THREAD_NAP_TIME_FAST);
                apr_thread_mutex_unlock(self->stop_mutex);
            }
        } else {
            apr_thread_mutex_lock(self->stop_mutex);
            res = apr_thread_cond_timedwait(self->stop_cond, self->stop_mutex, RELAY_THREAD_NAP_TIME_NORMAL);
            apr_thread_mutex_unlock(self->stop_mutex);
        }

        /*
         * Check if we have been awakened for the purpose of cancelation
         */
        if (!self->running)
            apr_thread_exit(thread, JXTA_SUCCESS);

        /*
         * Check if we receive our lease response while we were
         * sleeping
         */
        nbOfConnectedPeers = 0;
        for (i = 0; i < jxta_vector_size(self->peers); ++i) {
            res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }

          /**
           ** Since check_peer_lease() may have changed the connected state,
           ** we need to re-test the state.
           **/
            if (peer->is_connected) {
                ++nbOfConnectedPeers;
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /*
         * We still did not get a relay connect. Retry our
         * known seeding peers as that's the only thing we can do
         * at that point
         */
        if (nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS) {
            sz = jxta_vector_size(self->peers);
            for (i = 0; i < sz; i++) {
                jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
                check_relay_connect(self, peer, nbOfConnectedPeers);
                connect_seed = TRUE;
                JXTA_OBJECT_RELEASE(peer);
            }

        }

		/* if still not connected try to request the ads from the rdv again */
		if (nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS) {
			search_remote_relay_advertisements(self,FALSE);
			last_remote_relay_search=jpr_time_now();
		}

		/* even if we found some relays search for more if we need them in the future */
		if (jpr_time_now() - last_remote_relay_search > RESCAN_REMOTE_RELAY_ADVERTISEMENTS) {
			search_remote_relay_advertisements(self,FALSE);
			last_remote_relay_search=jpr_time_now();
		}


    }

    apr_thread_exit(thread, JXTA_SUCCESS);
    return NULL;
}


/************************************************************************
 * Entry point for a thread responsible for periodically trying to send
 * out all the stored messages
 *
 *
 * @param  thread the thread to start
 * @param  arg the relay transport
 * @return  
 *************************************************************************/
static void *APR_THREAD_FUNC send_messages_thread(apr_thread_t * thread, void *arg)
{
	Jxta_status res;
	Jxta_time message_expiry_last_time_checked = jpr_time_now();
	Jxta_time lease_expiry_last_time_checked = jpr_time_now();
	Jxta_time last_time_awake = jpr_time_now();
    _jxta_transport_relay *self = PTValid(arg, _jxta_transport_relay);
	unsigned int i;
	_relay_lease* relay_lease=NULL;
	Jxta_boolean send_all_messages=TRUE;

	/*mark the thread as running*/
	self->messages_thread_running=TRUE;

	while (self->messages_thread_running) {
		 
		/* why were we woken up, if the timeout was reached try to send out
		 * all messages, if we were woken up because new messages arrived
		 * send only these messages */
		apr_thread_mutex_lock(self->messages_mutex);
		send_all_messages=!self->messages_new_arrivals;
		self->messages_new_arrivals=FALSE;
		apr_thread_mutex_unlock(self->messages_mutex);

		/* make sure its not yet time to send all messages out */
		send_all_messages=send_all_messages || ((last_time_awake+(SEND_MESSAGES_THREAD_SLEEP_TIME/MILLI_TO_MICRO))<jpr_time_now());

		/*go through all leases*/
		for (i=0; i<jxta_vector_size(self->leases); i++) {
			Jxta_boolean send_messages=TRUE;
			jxta_vector_get_object_at(self->leases,JXTA_OBJECT_PPTR(&relay_lease),i);

			if (!send_all_messages) {
				apr_thread_mutex_lock(relay_lease->mutex);
				if (!relay_lease->messages_new_additions) {
					send_messages=FALSE;
				}
				apr_thread_mutex_unlock(relay_lease->mutex);
			}

			/* send the messages stored in the current lease */
			if (send_messages) {
				if (relay_send_all_messages_1(self,relay_lease)!=JXTA_SUCCESS) {
					jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Problem sending out messages to the relay client %s\n",relay_lease->lease_requestor_id);
				}
			}
		}

		/* make sure new messages were not added on while the processing
		 * was going
		 */
		{
			Jxta_boolean need_to_sleep=TRUE;

			apr_thread_mutex_lock(self->messages_mutex);
			need_to_sleep=!self->messages_new_arrivals;
			apr_thread_mutex_unlock(self->messages_mutex);

			if (need_to_sleep)
			{
				Jxta_time previous_time_awake=last_time_awake;
				/* sleep for a while */
				last_time_awake = jpr_time_now();
				apr_thread_mutex_lock(self->messages_mutex);
				if (send_all_messages) {
					res = apr_thread_cond_timedwait(self->messages_cond, self->messages_mutex, SEND_MESSAGES_THREAD_SLEEP_TIME);
				} else {
					res = apr_thread_cond_timedwait(self->messages_cond, self->messages_mutex, SEND_MESSAGES_THREAD_SLEEP_TIME-(last_time_awake-previous_time_awake)*MILLI_TO_MICRO);
				}
				apr_thread_mutex_unlock(self->messages_mutex);
			}
		}


		/* is it is time to check for expired messages */
		if (message_expiry_last_time_checked+MESSAGE_CHECK_EXPIRY_INTERVAL<jpr_time_now()) {
			archive_check_timeout(self);
			message_expiry_last_time_checked=jpr_time_now();
		}

		/* is it is time to check for expired leases */
		if (lease_expiry_last_time_checked+LEASE_CHECK_EXPIRY_INTERVAL<jpr_time_now()) {
			leases_remove_expired(self);
			lease_expiry_last_time_checked=jpr_time_now();
		}

    }

    apr_thread_exit(thread, JXTA_SUCCESS);
    return NULL;

}

static void check_relay_lease(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;
    char *add = NULL;
	Jxta_endpoint_address *relay_address=NULL;

    PTValid(peer, _jxta_peer_relay_entry);

    /*
     * check if the peer has still a valid relay
     */

    jxta_peer_lock((Jxta_peer *) peer);

    add = jxta_endpoint_service_get_relay_addr(self->endpoint);
    if (add == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "lost relay connection\n");
        peer->is_connected = FALSE;
        peer->try_connect = FALSE;
        peer->connectTime = 0;
        peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
        jxta_peer_unlock((Jxta_peer *) peer);

        /*
         * remove the relay address from our local route
         *
         * FIXME 20040701 tra we should only remove the hop
         * for the relay we just disconnected with. For now we just 
         * remove all the hops
         */
        jxta_PG_remove_relay_address(self->group, NULL);
        return;
    } else {
		JString *local_jstring;
		char *protocol = jxta_endpoint_service_get_relay_proto(self->endpoint);
		local_jstring = jstring_new_2(protocol);
		free(protocol);
		jstring_append_2(local_jstring,"://");
		jstring_append_2(local_jstring,add);
		relay_address=jxta_endpoint_address_new_1(local_jstring);
        free(add);
		JXTA_OBJECT_RELEASE(local_jstring);
    }

	/* even if we think that there is a connection to the relay in reality the server might
	 * have broken the connection, however the lease is still valid.
	 * In this case need to restore the connection to get any messages
	 * waiting on the server
	 * For now implement this only for tcp, likely do not need to do this for http
	 */
	if (relay_address) {
		Jxta_transport *transport=NULL;
		JxtaEndpointMessenger* endpoint_messenger=NULL;
		const char *protocol=jxta_endpoint_address_get_protocol_name(relay_address);

		if (!strcmp(protocol,"tcp")) {
			transport = jxta_endpoint_service_lookup_transport(self->endpoint, protocol);
		}
		
		if (transport) {
			/* get the endpoint messenger, the act of getting the messenger should reestablish the connection
			* to the transport
			*/
			endpoint_messenger = jxta_transport_messenger_get(transport, relay_address);

			if (endpoint_messenger) {
				jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "The connection to the relay is open\n");
				JXTA_OBJECT_RELEASE(endpoint_messenger);
			} else {
				jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not open a messenger to relay\n");
			}

			JXTA_OBJECT_RELEASE(transport);
		}

		JXTA_OBJECT_RELEASE(relay_address);
    }

    expires = jxta_peer_get_expires((Jxta_peer *) peer);

    /* Check if the peer really still has a lease */
    if (expires < currentTime) {
        /* Lease has expired */
        peer->is_connected = FALSE;
        peer->try_connect = FALSE;
        /**
         ** Since we have just lost a connection, it is a good idea to
         ** try to reconnect right away.
         **/
        peer->connectTime = currentTime;
        peer->connectRetryDelay = 0;
        jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }
    /* We still have a lease. Is it time to renew it ? */
    if ((expires - currentTime) <= RELAY_RENEWAL_DELAY) {
        peer->connectTime = currentTime;
        jxta_peer_unlock((Jxta_peer *) peer);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "relay connection reconnect\n");

        reconnect_to_relay(self, peer);
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "relay connection check ok %s\n",
                    jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));

    jxta_peer_unlock((Jxta_peer *) peer);
    return;
}

static void check_relay_connect(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer, int nbOfConnectedPeers)
{

    Jxta_time currentTime = jpr_time_now();

    jxta_peer_lock((Jxta_peer *) peer);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay->connect in= %" APR_INT64_T_FMT "ms\n",
                    (peer->connectTime - currentTime));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "       RetryDelay= %" APR_INT64_T_FMT "ms\n", peer->connectRetryDelay);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "     connected to= %d relays\n", nbOfConnectedPeers);

    if (peer->connectTime > currentTime) {
        /**
         ** If the number of connected peer is less than MIN_NB_OF_CONNECTED_PEERS
         ** and if peer->connectTime is more than LONG_RETRY_DELAY, then connectTime is
         ** reset, forcing an early attempt to connect.
         ** Otherwise, let it go.
         **/
        if ((nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS) && ((peer->connectTime - currentTime) > RELAY_LONG_RETRY_DELAY)) {
            /**
             ** Reset the connection time. Note that connectRetryDelay is not changed,
             ** because while it might be a good idea to try earlier, it is still very
             ** likely that the peer is still not reachable. 
             **/
            peer->connectTime = currentTime;
            jxta_peer_unlock((Jxta_peer *) peer);
            return;
        }
    } else {
        /* Time to try a connection */
        jxta_peer_unlock((Jxta_peer *) peer);

        /*
         * NOTE: 20050607 tra
         * check if we already send the try connect for that peer
         * if yes, just send a lease reconnect instead. The
         * relay server has a specific way to handle relay client
         * connection to protect itself against constant failure
         * of a client.
         */
        if (!peer->try_connect)
            connect_to_relay(self, peer);
        else
            reconnect_to_relay(self, peer);
        return;
    }
    jxta_peer_unlock((Jxta_peer *) peer);
    return;
}

static Jxta_status send_message_to_peer(_jxta_transport_relay * me, Jxta_endpoint_address * destAddr, Jxta_message * msg)
{
    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    Jxta_endpoint_address *addr = NULL;

    transport = jxta_endpoint_service_lookup_transport(me->endpoint, jxta_endpoint_address_get_protocol_name(destAddr));

    if (!transport) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not find the corresponding transport\n");
        return JXTA_FAILED;
    }

    endpoint_messenger = jxta_transport_messenger_get(transport, destAddr);

    if (endpoint_messenger) {
        JXTA_OBJECT_CHECK_VALID(endpoint_messenger);
        addr = jxta_transport_publicaddr_get(transport);
        jxta_message_set_source(msg, addr);
        jxta_message_set_destination(msg, destAddr);
        endpoint_messenger->jxta_send(endpoint_messenger, msg);
        JXTA_OBJECT_RELEASE(endpoint_messenger);
        JXTA_OBJECT_RELEASE(addr);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not create a messenger\n");
        return JXTA_FAILED;
    }
    JXTA_OBJECT_RELEASE(transport);

    return JXTA_SUCCESS;
}

static void connect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer)
{

    Jxta_time currentTime = jpr_time_now();
    Jxta_message *msg = NULL;
    Jxta_endpoint_address *destAddr = NULL;
    JString *service_name = NULL;
    JString *service_params = NULL;
	JString *connect_params = NULL;
    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    Jxta_endpoint_address *addr = NULL;
	Jxta_message_element *message_element;

    jxta_peer_lock((Jxta_peer *) peer);

    if (peer->connectTime > currentTime) {
        /* Not time yet to try to connect */
        jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "send connect request to relay\n");

    /*
     * mark that we send our new connect
     */
    peer->try_connect = TRUE;

    /**
     ** Update the delay before the next retry. Double it until it
     ** reaches MAX_RETRY_DELAY.
     **
     **/
    if (peer->connectRetryDelay > 0) {
        peer->connectRetryDelay = peer->connectRetryDelay * 2;
    } else {
        peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
    }
    if (peer->connectRetryDelay > RELAY_MAX_RETRY_DELAY) {
        peer->connectRetryDelay = RELAY_MAX_RETRY_DELAY;
    }

    peer->connectTime = currentTime + peer->connectRetryDelay;

    jxta_peer_unlock((Jxta_peer *) peer);

    /**
     ** Create a message with a connection request and build the request.
     **/
    msg = jxta_message_new();
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return;
    }

    /**
     ** Set the destination address of the message.
     **/

    service_name = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    if (service_name == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        return;
    }
    jstring_append_2(service_name, ":");
    jstring_append_2(service_name, self->groupid);

    service_params = jstring_new_2(self->assigned_id);
    if (service_params == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(service_name);
        return;
    }

	/*add the id of this peer to the message*/
    jstring_append_2(service_params, "/");
	/*jstring_append_2(service_params, self->peerid);*/

	connect_params=jstring_new_2("");
    jstring_append_2(connect_params, (char *) RELAY_CONNECT_REQUEST);
    jstring_append_2(connect_params, ",");
    jstring_append_2(connect_params, (char *) LEASE_REQUEST);
    jstring_append_2(connect_params, ",");
    jstring_append_2(connect_params, (char *) LAZY_CLOSE);

    /* for Java client compatibility add connect params to the service_params */
    jstring_append_2(service_params, jstring_get_string(connect_params));

    /* due to Java client compatibility change need to make sure C clients will
     * always receive the requesting peer id, this adds certain incompatibility
     * between full (not in welcome message) lease requests between Java and C
     * peers, however since Java ignores all full lease requests this should be ok
     */
    message_element=jxta_message_element_new_2(RELAY_NS,RELAY_PEER_ID_MESSAGE_ELEMENT,"text/plain;charset=UTF-8",self->peerid,strlen(self->peerid),NULL);
    jxta_message_add_element(msg, message_element);
    JXTA_OBJECT_RELEASE(message_element);

	/*add request to the message*/
	message_element=jxta_message_element_new_2(RELAY_NS,RELAY_REQUEST_MESSAGE_ELEMENT,"text/plain;charset=UTF-8",jstring_get_string(connect_params),jstring_length(connect_params),NULL);
	jxta_message_add_element(msg, message_element);
	JXTA_OBJECT_RELEASE(message_element);



    destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                           jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address),
                                           jstring_get_string(service_name), jstring_get_string(service_params));
    if (destAddr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(service_name);
        JXTA_OBJECT_RELEASE(service_params);
        return;
    }

    send_message_to_peer(self, destAddr, msg);

    JXTA_OBJECT_RELEASE(destAddr);
    JXTA_OBJECT_RELEASE(service_name);
    JXTA_OBJECT_RELEASE(service_params);
	JXTA_OBJECT_RELEASE(connect_params);
    JXTA_OBJECT_RELEASE(msg);
}

static void reconnect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_message *msg = NULL;
    Jxta_endpoint_address *destAddr = NULL;
    JString *tmp = NULL;
    JString *tmp1 = NULL;
    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    Jxta_endpoint_address *addr = NULL;
    JString *request;
    Jxta_message_element *msgElem;

    jxta_peer_lock((Jxta_peer *) peer);

    if (peer->connectTime > currentTime) {
        /* Not time yet to try to connect */
        jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "send reconnect request to relay\n");

    /**
     ** Update the delay before the next retry. Double it until it
     ** reaches MAX_RETRY_DELAY.
     **
     **/
    if (peer->connectRetryDelay > 0) {
        peer->connectRetryDelay = peer->connectRetryDelay * 2;
    } else {
        peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
    }
    if (peer->connectRetryDelay > RELAY_MAX_RETRY_DELAY) {
        peer->connectRetryDelay = RELAY_MAX_RETRY_DELAY;
    }

    peer->connectTime = currentTime + peer->connectRetryDelay;

    jxta_peer_unlock((Jxta_peer *) peer);

    /**
     ** Create a message with a connection request and build the request.
     **/
    msg = jxta_message_new();
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return;
    }
    
    request = jstring_new_2(RELAY_CONNECT_REQUEST);
    jstring_append_2(request, ",");
    jstring_append_2(request, LEASE_REQUEST);
    jstring_append_2(request, ",");
    jstring_append_2(request, LAZY_CLOSE);

    msgElem = jxta_message_element_new_1(RELAY_LEASE_REQUEST,
                                         "text/plain", (char *) jstring_get_string(request),
                                         jstring_length(request), NULL);

    jxta_message_add_element(msg, msgElem);
    JXTA_OBJECT_RELEASE(msgElem);
    JXTA_OBJECT_RELEASE(request);

    /* Disabled 
    * due to Java client compatibility change need to make sure C clients will
    * always receive the requesting peer id, this adds certain incompatibility
    * between full (not in welcome message) lease requests between Java and C
    * peers, however since Java ignores all full lease requests this should be ok
    */
    /*msgElem=jxta_message_element_new_2(RELAY_NS,RELAY_PEER_ID_MESSAGE_ELEMENT,"text/plain;charset=UTF-8",self->peerid,strlen(self->peerid),NULL);
    jxta_message_add_element(msg, msgElem);
    JXTA_OBJECT_RELEASE(msgElem);*/

    /**
     ** Set the destination address of the message.
     **/
    tmp = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        return;
    }
    jstring_append_2(tmp, ":");
    jstring_append_2(tmp, self->groupid);

    tmp1 = jstring_new_2(self->assigned_id);
    if (tmp1 == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(tmp);
        return;
    }
    jstring_append_2(tmp1, "/");
    jstring_append_2(tmp1, self->peerid);

    destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                           jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address),
                                           jstring_get_string(tmp), jstring_get_string(tmp1));
    if (destAddr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(tmp);
        JXTA_OBJECT_RELEASE(tmp1);
        return;
    }

    send_message_to_peer(self, destAddr, msg);

    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(tmp1);
    JXTA_OBJECT_RELEASE(destAddr);
    JXTA_OBJECT_RELEASE(msg);
}


/************************************************************************
 * Retrieve a lease from the list of relay leases
 *
 *
 * @param  self the relay
 * @param  dest_pid the pid on which to search for lease
 * @return  the lease, if the lease does not exist returns NULL, not shared
 *************************************************************************/
static _relay_lease* find_relay_lease(_jxta_transport_relay* self, JString* dest_pid)
{
	unsigned int i;
	_relay_lease* relay_lease;

	/* find the lease for the pid */
	for (i=0; i<jxta_vector_size(self->leases); i++) {
		jxta_vector_get_object_at(self->leases,JXTA_OBJECT_PPTR(&relay_lease),i);

		if (!strcmp(jstring_get_string(relay_lease->lease_requestor_id),jstring_get_string(dest_pid))) {
			JXTA_OBJECT_RELEASE(relay_lease);
			break;
		}

		JXTA_OBJECT_RELEASE(relay_lease);
	}

	if (jxta_vector_size(self->leases)>i) {
		return relay_lease;
	} else {
		return NULL;
	}
}

static Jxta_boolean lease_is_valid(_relay_lease* relay_lease)
{
	if (relay_lease->issue_time+relay_lease->lease_duration>jpr_time_now())
	{
		return TRUE;
	} else {
		return FALSE;
	}
}

static Jxta_status lease_delete(_jxta_transport_relay* self, JString* lease_uid)
{
	unsigned int i;
	_relay_lease* relay_lease;

	/* find the lease for the pid */
	for (i=0; i<jxta_vector_size(self->leases); i++) {
		jxta_vector_get_object_at(self->leases,JXTA_OBJECT_PPTR(&relay_lease),i);

		if (!strcmp(jstring_get_string(relay_lease->lease_requestor_id),jstring_get_string(lease_uid))) {
			break;
		}

		JXTA_OBJECT_RELEASE(relay_lease);
	}

	if (jxta_vector_size(self->leases)>i) {
		JXTA_OBJECT_RELEASE(relay_lease);
		return jxta_vector_remove_object_at(self->leases,NULL,i);
	} else {
		return JXTA_ITEM_NOTFOUND;
	}

}

/************************************************************************
* Goes through all current leases and removes the ones that have expired
*
*
* @param   self the relay
* @return  JXTA_SUCCESS if not errors have occurred
*************************************************************************/
static Jxta_status leases_remove_expired(_jxta_transport_relay* self)
{
	unsigned int i;
	_relay_lease* relay_lease;
	Jxta_vector *current_leases;

	current_leases=self->leases;
	self->leases=jxta_vector_new(jxta_vector_size(current_leases));

	/* go through all leases and only keep the ones that have not expired */
	for (i=0; i<jxta_vector_size(current_leases); i++) {
		jxta_vector_get_object_at(current_leases,JXTA_OBJECT_PPTR(&relay_lease),i);

		if (lease_is_valid(relay_lease)) {
			jxta_vector_add_object_last(self->leases,JXTA_OBJECT(relay_lease));
		}

		JXTA_OBJECT_RELEASE(relay_lease);
	}

	JXTA_OBJECT_RELEASE(current_leases);

	return JXTA_SUCCESS;
}

/* Code to manage messages stored by relay
 *
 */

/************************************************************************
 * Add message to the relay message archive
 *
 *
 * @param  self the relay
 * @param  msg the message to add
 * @param  dest_pid the PID to which the message is directed
 * @return  the new object
 *************************************************************************/
static Jxta_status archive_put_message(_jxta_transport_relay* self, Jxta_message * msg, JString* dest_pid)
{
	_relay_lease* relay_lease;

	relay_lease=find_relay_lease(self, dest_pid);

	/*check that the lease exists*/
	if (!relay_lease) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay lease does not exist\n");
		return JXTA_ITEM_NOTFOUND;
	}

	return archive_put_message_1(relay_lease, msg);	
}

/************************************************************************
 * Add message to the relay message archive
 *
 *
 * @param  jxta_lease_object the lease object which keeps the messages
 * @param  msg the message to add
 * @return  the new object
 *************************************************************************/
static Jxta_status archive_put_message_1(_relay_lease* jxta_lease_object, Jxta_message * msg)
{
	_stored_message* stored_message=NULL;

	/*make sure that the vector is allocated*/
	/*if (!jxta_lease_object->stored_messages) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message storage does not exist in the lease object\n");
		return JXTA_ITEM_NOTFOUND;
	}*/
	apr_thread_mutex_lock(jxta_lease_object->mutex);

    if (jxta_lease_object->total_stored>MAX_NUMBER_MESSAGES_PER_CLIENT) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Number of messages for this client it greater than the allowed maximum\n");
        apr_thread_mutex_unlock(jxta_lease_object->mutex);
        return JXTA_FAILED;
    }

	/*try to add the message to the storage*/
	stored_message=stored_message_new(msg,DEFAULT_MESSAGE_STORAGE_INTERVAL);
	
	if (linked_list_add_last(JXTA_LINKED_LIST_NODE(stored_message),
		JXTA_LINKED_LIST_NODE_PPTR(&jxta_lease_object->stored_messages))==NULL) {
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot to add message to the list\n");
			JXTA_OBJECT_RELEASE(stored_message);
			apr_thread_mutex_unlock(jxta_lease_object->mutex);
			return JXTA_FAILED;
    } else {
        jxta_lease_object->total_stored++;
    }

	jxta_lease_object->messages_new_additions=TRUE;

	apr_thread_mutex_unlock(jxta_lease_object->mutex);
	/*JXTA_OBJECT_RELEASE(stored_message);*/
	return JXTA_SUCCESS;
}

/************************************************************************
 * Get all the messages for the specific pid
 *
 *
 * @param  self the relay
 * @param  dest_pid the PID to which the message is directed
 * @return  the vector with messages
 *************************************************************************/
static Jxta_vector* archive_get_all_messages(_jxta_transport_relay* self, JString* dest_pid)
{
	_relay_lease* relay_lease;
	Jxta_vector* messages=NULL;
	_stored_message* next=NULL;

	relay_lease=find_relay_lease(self, dest_pid);

	/*check that the lease exists*/
	if (!relay_lease) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay lease does not exist\n");
		return NULL;
	}

	messages=jxta_vector_new(10);

	next=(_stored_message*)linked_list_right(JXTA_LINKED_LIST_NODE(relay_lease->stored_messages));

	do {
		_stored_message* current=next;
		next=(_stored_message*)linked_list_right(JXTA_LINKED_LIST_NODE(next));

		jxta_vector_add_object_last(messages,JXTA_OBJECT(current));

	} while (relay_lease->stored_messages!=next);

	return messages;
}

/************************************************************************
 * Goes through the whole message archive and removes all expired messages
 * this function is fairly expensive and should not be ran frequently
 *
 *
 * @param  self the relay
 * @return  the status of the operation, failure means something went very
 *          wrong and the program should be terminated
 *************************************************************************/
static Jxta_status archive_check_timeout(_jxta_transport_relay* self)
{
	unsigned int i;/*,k;*/
	_relay_lease* relay_lease;
	/*Jxta_vector* new_message_storage;
	_stored_message* stored_message;*/
	_stored_message* next=NULL;
	_stored_message* head=NULL;

	/* go through all leases */
	for (i=0; i<jxta_vector_size(self->leases); i++) {
		jxta_vector_get_object_at(self->leases,JXTA_OBJECT_PPTR(&relay_lease),i);

		if (relay_lease->stored_messages) {

			apr_thread_mutex_lock(relay_lease->mutex);
			/* allocate a new vector where the non expired messages will be placed */
			/*new_message_storage=jxta_vector_new(10);

			if (!new_message_storage) {
				jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create new vector\n");
				apr_thread_mutex_unlock(relay_lease->mutex);
				return JXTA_FAILED;
			}*/

			/*for (k=0; k<jxta_vector_size(relay_lease->stored_messages); k++) {*/

			head=relay_lease->stored_messages;
			next=(_stored_message*)linked_list_right(JXTA_LINKED_LIST_NODE(head));
			do {
				_stored_message* current=next;
				next=(_stored_message*)linked_list_right(JXTA_LINKED_LIST_NODE(current));
				/*jxta_vector_get_object_at(relay_lease->stored_messages,JXTA_OBJECT_PPTR(&stored_message),k);*/

				if (message_has_expired(current)) {
					linked_list_remove(JXTA_LINKED_LIST_NODE(current),JXTA_LINKED_LIST_NODE_PPTR(&relay_lease->stored_messages));
					relay_lease->total_stored--;
                    JXTA_OBJECT_RELEASE(current);
				}	
			} while (next!=head);

			/*JXTA_OBJECT_RELEASE(relay_lease->stored_messages);

			relay_lease->stored_messages=new_message_storage;*/

			apr_thread_mutex_unlock(relay_lease->mutex);
		}

		JXTA_OBJECT_RELEASE(relay_lease);
	}


	return JXTA_SUCCESS;
}

/************************************************************************
 * Checks if a message has expired
 *
 *
 * @param  stored_message the message
 * @return  true is expired
 *************************************************************************/
static Jxta_boolean message_has_expired(const _stored_message* stored_message)
{
	if (stored_message->time_stored+stored_message->storage_interval_length < jpr_time_now()) {
		return TRUE;
	} else {
		return FALSE;
	}
}


/* Code to create message storage object
 *
 */

/************************************************************************
 * Generate a new stored message object
 *
 *
 * @param  msg the message to store
 * @param  storage_interval_length the amount of time the message should be stored
 * @return  the new object
 *************************************************************************/
static _stored_message* stored_message_new(Jxta_message * msg,
										   Jxta_time_diff storage_interval_length)
{
	_stored_message* stored_message_obj;

    stored_message_obj = (_stored_message *) calloc(1, sizeof(_stored_message));

    if (NULL == stored_message_obj)
        return NULL;

	stored_message_obj->msg=JXTA_OBJECT_SHARE(msg);

	stored_message_obj->storage_interval_length=storage_interval_length;
	stored_message_obj->time_stored=jpr_time_now();
	JXTA_LINKED_LIST_NODE(stored_message_obj)->left=NULL;
	JXTA_LINKED_LIST_NODE(stored_message_obj)->right=NULL;

    JXTA_OBJECT_INIT(stored_message_obj, stored_message_delete, 0);

	return stored_message_obj;
}

/************************************************************************
 * Deallocates an existing stored message object
 *
 * @param  the object to dealocate
 *************************************************************************/
static void stored_message_delete(Jxta_object * obj)
{
    _stored_message *stored_message_obj = (_stored_message *) obj;

    if (NULL == stored_message_obj)
        return;

	JXTA_OBJECT_RELEASE(stored_message_obj->msg);

	free(stored_message_obj);
}

/* Code to create lease objects
 *
 */


/************************************************************************
 * Generate a new lease object
 *
 *
 * @param  
 * @return  the new object
 *************************************************************************/
static _relay_lease* relay_lease_new()
{
	_relay_lease* relay_lease_obj;
	Jxta_status status;

    relay_lease_obj = (_relay_lease *) calloc(1, sizeof(_relay_lease));

    if (NULL == relay_lease_obj)
        return NULL;

	relay_lease_obj->lease_requestor_id=NULL;
	relay_lease_obj->lease_requestor_address=NULL;
	relay_lease_obj->lease_duration=DEFAULT_LEASE_DURATION;
	relay_lease_obj->issue_time=jpr_time_now();

	/* mutex */
	status = apr_pool_create(&relay_lease_obj->pool, NULL);
	if (APR_SUCCESS != status) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create messenger apr pool\n");
		free(relay_lease_obj);
		return NULL;
	}

	status = apr_thread_mutex_create(&relay_lease_obj->mutex, APR_THREAD_MUTEX_NESTED, relay_lease_obj->pool);
	if (APR_SUCCESS != status) {
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create messenger mutex\n");
		apr_pool_destroy(relay_lease_obj->pool);
		free(relay_lease_obj);
		return NULL;
	}

	/*relay_lease_obj->stored_messages=jxta_vector_new(10);*/
	relay_lease_obj->stored_messages=NULL;

	relay_lease_obj->messages_new_additions=FALSE;

    relay_lease_obj->total_stored=0;

    JXTA_OBJECT_INIT(relay_lease_obj, relay_lease_delete, 0);

	return relay_lease_obj;
}

/************************************************************************
 * Deallocates an existing lease object
 *
 * @param  the object to deallocate
 *************************************************************************/
static void relay_lease_delete(Jxta_object * obj)
{
    _relay_lease *relay_lease_obj = (_relay_lease *) obj;

    if (NULL == relay_lease_obj)
        return;

	if (relay_lease_obj->lease_requestor_id!=NULL)
		JXTA_OBJECT_RELEASE(relay_lease_obj->lease_requestor_id);

	if (relay_lease_obj->lease_requestor_address!=NULL)
		JXTA_OBJECT_RELEASE(relay_lease_obj->lease_requestor_address);

	/* release the vector, the objects inside the vector will be release
	 * automatically */
	/*if (relay_lease_obj->stored_messages!=NULL)
		JXTA_OBJECT_RELEASE(relay_lease_obj->stored_messages);*/

	/* if the linked list of messages is not empty, release it */
	if (relay_lease_obj->stored_messages!=NULL) {
		linked_list_clear_and_free(JXTA_LINKED_LIST_NODE(relay_lease_obj->stored_messages));
	}

	apr_thread_mutex_destroy(relay_lease_obj->mutex);
	apr_pool_destroy(relay_lease_obj->pool);

	free(relay_lease_obj);
}


/************************************************************************
 * Accepts a msg and checks whether this message contains a lease request
 * if so tries to issue a lease
 *
 *
 * @param  msg the incoming message
 * @param  self the relay transport used to get the message
 * @return  
 *************************************************************************/

static void check_for_lease_request(Jxta_message *msg, _jxta_transport_relay *self)
{
	Jxta_message_element *el = NULL;
    JString *response = NULL;
    Jxta_bytevector *bytes = NULL;
	JString *lease_request_params=NULL;
	JString *lease_requestor_id=NULL;
    /* need for compatibility with Java clients */
    Jxta_boolean jxta_c_request_style=FALSE;

    /* all jxta-c lease requests contain RELAY_PEER_ID_MESSAGE_ELEMENT */
    jxta_message_get_element_2(msg, RELAY_NS, RELAY_PEER_ID_MESSAGE_ELEMENT, &el);

    if (el) {
        jxta_c_request_style=TRUE;
        bytes = jxta_message_element_get_value(el);
        lease_requestor_id = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        JXTA_OBJECT_RELEASE(el);        
    }
    

	jxta_message_get_element_2(msg, RELAY_NS, RELAY_REQUEST_MESSAGE_ELEMENT, &el);
	
	/* if this message contains a request message element, this element is
     * contained in all C peer requests and in some Java peer requests*/
	if (el) {
		/* extract the lease parameters */
		bytes = jxta_message_element_get_value(el);
		lease_request_params = jstring_new_3(bytes);
		JXTA_OBJECT_RELEASE(bytes);
		JXTA_OBJECT_RELEASE(el);

        /* if there is a request message element, but not a peer_id message element
         * then the peer id would be appended to the destination address 
         */
        if (!jxta_c_request_style) {
            Jxta_endpoint_address *address_relay_server = NULL;
            char *peer_id_tmp=NULL;

            /* extract the address of the relay */
            jxta_message_get_element_2(msg, JXTA_NS, ENDPOINT_DESTINATION_ADDRESS, &el);
            bytes = jxta_message_element_get_value(el);
            response = jstring_new_3(bytes);
            address_relay_server=jxta_endpoint_address_new_1(response);
            JXTA_OBJECT_RELEASE(bytes);
            JXTA_OBJECT_RELEASE(el);
            JXTA_OBJECT_RELEASE(response);

            /* get the lease parameters,the parameters should be attached to the lease */
            peer_id_tmp=strrchr(jxta_endpoint_address_get_service_params(address_relay_server),'/');

            if (peer_id_tmp) {
                ++peer_id_tmp;
                lease_requestor_id=jstring_new_2(peer_id_tmp);
            } else {
                JXTA_OBJECT_RELEASE(address_relay_server);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to extract peer id from Java style lease request\n");
                return;
            }		

            JXTA_OBJECT_RELEASE(address_relay_server);

        }

    } else {
        Jxta_endpoint_address *address_relay_server = NULL;
        char *lease_request_params_tmp=NULL;

        /* if there is no request element than the parameters would be
         * contained in the destination address */

        /* extract the address of the relay */
        jxta_message_get_element_2(msg, JXTA_NS, ENDPOINT_DESTINATION_ADDRESS, &el);
        bytes = jxta_message_element_get_value(el);
        response = jstring_new_3(bytes);
        address_relay_server=jxta_endpoint_address_new_1(response);
        JXTA_OBJECT_RELEASE(bytes);
        JXTA_OBJECT_RELEASE(el);
        JXTA_OBJECT_RELEASE(response);

        /* get the lease parameters,the parameters should be attached to the lease */
        lease_request_params_tmp=strrchr(jxta_endpoint_address_get_service_params(address_relay_server),'/');

        if (lease_request_params_tmp) {
            ++lease_request_params_tmp;
            lease_request_params=jstring_new_2(lease_request_params_tmp);
        } else {
            JXTA_OBJECT_RELEASE(address_relay_server);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to extract lease request parameters from Java style lease request\n");
            return;
        }		

        JXTA_OBJECT_RELEASE(address_relay_server);
    }

    /* if there is not peer_id element and the peer_id has still not been
     * located then this is coming from a welcome message
     */
    if (!jxta_c_request_style && !lease_requestor_id) {
        char *str_tmp = NULL;
        Jxta_endpoint_address *addr = NULL;

        /*{
            Jxta_vector* v;
            int i;
            Jxta_message_element* e;

            v=jxta_message_get_elements(msg);

            for (i=0; i<jxta_vector_size(v); i++) {
                jxta_vector_get_object_at(v,&e,i);
                bytes = jxta_message_element_get_value(el);
                response = jstring_new_3(bytes);
                printf("%s/n",jstring_get_string(response));
            }
        }*/

        /* still need to locate peer id */
        addr = jxta_message_get_source(msg);
        str_tmp = (char *)jxta_endpoint_address_get_service_params(addr);

        if (!str_tmp) {
            JXTA_OBJECT_RELEASE(addr);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to extract peer id from Java style lease request\n");
            return;
        }

        str_tmp = strrchr(str_tmp,'/');
        
        if(str_tmp){
            ++str_tmp;
        } else {
            JXTA_OBJECT_RELEASE(addr);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to extract peer id from Java style lease request\n");
            return;
        }

        lease_requestor_id = jstring_new_2(str_tmp);

        JXTA_OBJECT_RELEASE(addr);
    }

	if (!lease_request_params || !lease_requestor_id) {
	    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Not a lease request\n");
		return;
	}
	
	/*if (lease_request_params && lease_requestor_id) {*/
    /* if all parameters were extracted can try to issue a lease request */
    {
		_relay_lease *relay_lease=NULL;
		Jxta_endpoint_address* relay_client_address=NULL;


		if (!self->Is_Server) {
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Not a relay server but received a relay request\n");
			JXTA_OBJECT_RELEASE(lease_requestor_id);
			JXTA_OBJECT_RELEASE(lease_request_params);
			return;
		}


		if (strstr(jstring_get_string(lease_request_params),RELAY_DISCONNECT_REQUEST)) {
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Disconnect lease request detected\n");
			
			if (lease_delete(self,lease_requestor_id)!=JXTA_SUCCESS) {
				jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Disconnect request for a non existing lease\n");
			}

			JXTA_OBJECT_RELEASE(lease_requestor_id);
			JXTA_OBJECT_RELEASE(lease_request_params);

			return;
		}

		/* FIX FIX FIX FIX FIX */
		/* for now simply make sure that parameters contain a connect request
		* ignore all other requests
		*/

		if (!strstr(jstring_get_string(lease_request_params),RELAY_CONNECT_REQUEST)) {
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Only lease requests of type connect and disconnect are supported\n");

			JXTA_OBJECT_RELEASE(lease_requestor_id);
			JXTA_OBJECT_RELEASE(lease_request_params);
			return;
		}
		
		/*get the source address of the request*/
		jxta_message_get_element_2(msg, JXTA_NS, ENDPOINT_SOURCE_ADDRESS, &el);
		bytes = jxta_message_element_get_value(el);
        response = jstring_new_3(bytes);
		relay_client_address=jxta_endpoint_address_new_1(response);

		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "relay lease request detected from %s with parameters %s",jstring_get_string(response),jstring_get_string(lease_request_params));
		JXTA_OBJECT_RELEASE(bytes);
		JXTA_OBJECT_RELEASE(response);

		/* if reached the limit on the number of leases, ignore the request */
		if (jxta_vector_size(self->leases)>=self->max_leases_allowed) {
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Max number of leases reached, ignoring request\n");
		}
		/* try to send a lease */
		else if (send_relay_lease(self,relay_client_address)==JXTA_SUCCESS) {

			/* try to retrieve existing relay lease */
			relay_lease=find_relay_lease(self,lease_requestor_id);

			/* if the lease has not already been issued */
			if (!relay_lease) {
				relay_lease=relay_lease_new();
				relay_lease->lease_requestor_id=JXTA_OBJECT_SHARE(lease_requestor_id);
				relay_lease->lease_requestor_address=JXTA_OBJECT_SHARE(relay_client_address);
				/* lease issued add to the list of leases */
				if (jxta_vector_add_object_last(self->leases,JXTA_OBJECT(relay_lease))!=JXTA_SUCCESS) {
					JXTA_OBJECT_RELEASE(lease_requestor_id);
					JXTA_OBJECT_RELEASE(lease_request_params);
					JXTA_OBJECT_RELEASE(relay_client_address);
					JXTA_OBJECT_RELEASE(relay_lease);
					jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to add lease to storage\n");
					return;
				}
				JXTA_OBJECT_RELEASE(relay_lease);
			} else {
				/* this is a subsequent lease request */
				relay_lease->issue_time=jpr_time_now();
			}
		}
		else
		{
			jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to issue a lease\n");
		}

		
		JXTA_OBJECT_RELEASE(relay_client_address);
		JXTA_OBJECT_RELEASE(lease_request_params);
	} /*if (lease_request_params && lease_requestor_id)*/

	JXTA_OBJECT_RELEASE(lease_requestor_id);

}

/************************************************************************
 * Accepts a relay lease request and issues a lease if possible
 *
 * ToDo:
 *       1. Check the max number of leases before issuing a lease
 *
 * @param  self The relay transport used to get the message
 * @param  lease_request the char representation of the request
 * @return Jxta_status
 *************************************************************************/
static Jxta_status send_relay_lease(_jxta_transport_relay *self, Jxta_endpoint_address *client_address)
{
	Jxta_message *msg_response = NULL; 
    JString *service_name = NULL;
    JString *service_params = NULL;
	Jxta_endpoint_address *destAddr = NULL;
	Jxta_message_element *message_element=NULL;
	Jxta_RdvAdvertisement* rdv_advertisement=NULL; 
	JString* rdv_advertisement_string=NULL;
	JString* local_jstring=NULL;
	Jxta_status status;

	JString *addr_tmp=NULL;

    /**
     ** Constructs the destination address of the message.
     **/
    service_name = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    jstring_append_2(service_name, ":");
    jstring_append_2(service_name, self->groupid);


    service_params = jstring_new_2(self->assigned_id);
    jstring_append_2(service_params, "/");
    jstring_append_2(service_params, self->peerid);


	/**
	 ** construct complete client address together with the service name and params
	 **/
	destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(client_address),
                                           jxta_endpoint_address_get_protocol_address(client_address),
                                           jstring_get_string(service_name), jstring_get_string(service_params));


	/**
	 ** Construct the accept lease message
	 **/

    /**
     ** Create a message for the response
     **/
    msg_response = jxta_message_new();

	/*relay sends a response*/
	message_element=jxta_message_element_new_2("relay","response","text/plain;charset=UTF-8","connected",9,NULL);
	jxta_message_add_element(msg_response, message_element);
	JXTA_OBJECT_RELEASE(message_element);
	/*relay gives a lease*/
	/*FIX: Magic number 3600000*/
	message_element=jxta_message_element_new_2("relay","lease","text/plain;charset=UTF-8","3600000",7,NULL);
	jxta_message_add_element(msg_response, message_element);
	JXTA_OBJECT_RELEASE(message_element);
	/*rdv advertisement*/
	local_jstring=jstring_new_2(self->assigned_id);
	rdv_advertisement=jxta_relay_build_rdva(self->group,local_jstring);
	JXTA_OBJECT_RELEASE(local_jstring);
	jxta_RdvAdvertisement_get_xml(rdv_advertisement,&rdv_advertisement_string);
	message_element=jxta_message_element_new_2("relay","relayAdv","text/xml;charset=UTF-8",jstring_get_string(rdv_advertisement_string),jstring_length(rdv_advertisement_string),NULL);
	jxta_message_add_element(msg_response, message_element);
	JXTA_OBJECT_RELEASE(message_element);
	JXTA_OBJECT_RELEASE(rdv_advertisement);
	JXTA_OBJECT_RELEASE(rdv_advertisement_string);

	/* THIS NEEDS TO BE REMOVED WHEN THE BUG IN THE INITIALIZATION CODE IS FIXED */
	/* sleep before sending in case the client is not fully initialized*/
	apr_sleep(ONE_SECOND_MICRO*10);

	status=send_message_to_peer(self, destAddr, msg_response);

	JXTA_OBJECT_RELEASE(destAddr);
	JXTA_OBJECT_RELEASE(service_name);
	JXTA_OBJECT_RELEASE(service_params);
	JXTA_OBJECT_RELEASE(msg_response);

	return status;
}

Jxta_boolean jxta_relay_is_server(Jxta_transport_relay_public* jxta_transport_relay)
{
	if (jxta_transport_relay && jxta_transport_relay->Is_Server) {
		return TRUE;
	} else {
		return FALSE;
	}

}

/* Functions for managing doubly linked list
 * Since there is not linked list objects, only nodes
 * connected to each other, the linked list functions
 * do not do any memory management, they simply manage links
 * For all functions, the return values are always not shared,
 * the caller need not do an extra release on any of the return values
 */

/************************************************************************
* Initializes a single node to point to itself, this is internal
* function, should not be used by any function except linked_list_*
*
*
* @param  node the node to operate on
* @return node
*************************************************************************/
Jxta_Linked_List_Node* linked_list_init_single_item(Jxta_Linked_List_Node *node)
{
	if (!node) {
		return NULL;
	}

	node->left=node;
	node->right=node;

	return node;
}

/************************************************************************
* Add the node to the left of the head of the list
*
*
* @param  node the node to operate on
* @param  head double pointer to the head of the list
* @return the head of the list
*************************************************************************/
Jxta_Linked_List_Node* linked_list_add_last(Jxta_Linked_List_Node *node, Jxta_Linked_List_Node **head)
{
	if (!(*head) && !node) {
		return NULL;
	}

	if (!(*head)) {
		*head=linked_list_init_single_item(node);
	}else if (node) {
		node->left=(*head)->left;
		node->right=(*head);
		

		/* in case list contained only one node */
		if ((*head)->right==(*head)) {
			(*head)->right=node;
		} else {
			(*head)->left->right=node;
		}

		(*head)->left=node;
	}

	return (*head);
}

/************************************************************************
* Removes the node from the list, if the removed node is the head of the
* list, *head is modified to reflect this. If after removal the list
* becomes empty *head is set to NULL
*
*
* @param  node the node to operate on
* @param  head double pointer to the head of the list
* @return the node to the right of the deleted node, or NULL if the
*         list becomes empty
*************************************************************************/
Jxta_Linked_List_Node* linked_list_remove(Jxta_Linked_List_Node *node, Jxta_Linked_List_Node **head)
{
	if (!node) {
		return NULL;
	}


	/* if the last item on the list is being deleted */
	if (node->left==node->right && node->right==node) {
		*head=NULL;
		return NULL;
	}

	/* if a head is being deleted */
	if (node==*head) {

		*head=node->right;
	}

	node->right->left=node->left;
	node->left->right=node->right;

	return node->right;
}

/************************************************************************
* Removes and releases all nodes on the list
*
*
* @param  head the head of the list
* @return
*************************************************************************/
void linked_list_clear_and_free(Jxta_Linked_List_Node* head)
{
	Jxta_Linked_List_Node* current=NULL;

	if (!head) {
		return;
	}

	current=head->right;

	do {
		Jxta_Linked_List_Node* previous=current;
		current=current->right;
		JXTA_OBJECT_RELEASE(previous);
	} while (current!=head);
}

/************************************************************************
* Returns true if a node has been initialized to be a member of a list
*
*
* @param  node the node to operate on
* @return TRUE if the node had been initialized, false otherwise
*************************************************************************/
Jxta_boolean linked_list_node_in_list(Jxta_Linked_List_Node* node)
{
	if (node->left==NULL || node->right==NULL) {
		return FALSE;
	}

	return TRUE;
}

/************************************************************************
* Returns the node to the left of the given node
*
*
* @param  node the node to operate on
* @return the node to the left
*************************************************************************/
Jxta_Linked_List_Node* linked_list_left(Jxta_Linked_List_Node *node)
{
	if (!node) {
		return NULL;
	}

	return node->left;
}

/************************************************************************
* Returns the node to the right of the given node
*
*
* @param  node the node to operate on
* @return the node to the right
*************************************************************************/
Jxta_Linked_List_Node* linked_list_right(Jxta_Linked_List_Node *node)
{
	if (!node) {
		return NULL;
	}

	return node->right;
}

static Jxta_RdvAdvertisement *jxta_relay_build_rdva(Jxta_PG * group, JString * serviceName)
{
    Jxta_PG *parent;
    Jxta_PGID *pgid;
    Jxta_PID *peerid;
    JString *name;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_PA *padv;
    Jxta_RdvAdvertisement *rdva = NULL;
    Jxta_svc *svc = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Building relay advertisement\n");

    jxta_PG_get_parentgroup(group, &parent);

    if (NULL != parent) {
        jxta_PG_get_PA(parent, &padv);
        JXTA_OBJECT_RELEASE(parent);
    } else {
        jxta_PG_get_PA(group, &padv);
    }

    rdva = jxta_RdvAdvertisement_new();

    name = jxta_PA_get_Name(padv);
    jxta_RdvAdvertisement_set_Name(rdva, jstring_get_string(name));
    JXTA_OBJECT_RELEASE(name);

    jxta_RdvAdvertisement_set_Service(rdva, jstring_get_string(serviceName));

    pgid = jxta_PA_get_GID(padv);
    jxta_RdvAdvertisement_set_RdvGroupId(rdva, pgid);
    JXTA_OBJECT_RELEASE(pgid);

    peerid = jxta_PA_get_PID(padv);
    jxta_RdvAdvertisement_set_RdvPeerId(rdva, peerid);
    JXTA_OBJECT_RELEASE(peerid);

    jxta_PA_get_Svc_with_id(padv, jxta_endpoint_classid , &svc);
    JXTA_OBJECT_RELEASE(padv);
    
    if (svc == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not find the endpoint service params\n");
        JXTA_OBJECT_RELEASE(rdva);
        return NULL;
    }

    route = jxta_svc_get_RouteAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);
    if (route == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        FILEANDLINE "Could not find the route advertisement in service params\n");
        JXTA_OBJECT_RELEASE(rdva);
        return NULL;
    }

    jxta_RdvAdvertisement_set_Route(rdva, route);
    JXTA_OBJECT_RELEASE(route);

    return rdva;
}


/* vim: set ts=4 sw=4 tw=130 et: */
