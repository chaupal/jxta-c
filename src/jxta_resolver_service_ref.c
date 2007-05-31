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
 * $Id: jxta_resolver_service_ref.c,v 1.65 2005/04/02 00:26:10 slowhog Exp $
 */


#include "jxtaapr.h"
#include "jpr/jpr_excep.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_id.h"
#include "jxta_rr.h"
#include "jxta_rq.h"
#include "jxta_rsrdi.h"
#include "jxta_hashtable.h"
#include "jxta_peergroup.h"
#include "jxta_message.h"

#include "jxta_private.h"
#include "jxta_resolver_service_private.h"
#include "jxta_discovery_service.h"
#include "jxta_routea.h"

#define HAVE_POOL

typedef struct {

    Extends(Jxta_resolver_service);
    JString*                instanceName;
    Jxta_boolean            running;
    Jxta_PG*                group;
    Jxta_rdv_service*       rendezvous;
    Jxta_endpoint_service*  endpoint;
    Jxta_discovery_service* discovery;
    Jxta_hashtable*         queryhandlers;
    Jxta_hashtable*         responsehandlers;
    Jxta_PID*               localPeerId;
    Jxta_id*                assigned_id;
    Jxta_advertisement*     impl_adv;
    JString*                inque;
    JString*                outque;
    JString*                srdique;
    apr_thread_mutex_t*     mutex;
    long                    query_id;
    apr_pool_t*             pool;

    /* hold listener to stop, no API to retrieve the listener */
    Jxta_listener *o_listener;
    Jxta_listener *i_listener;
}
Jxta_resolver_service_ref;

/* used to make an adress out of a peerid */
static Jxta_endpoint_address* get_endpoint_address_from_peerid (Jxta_id* peerid, char * name, char * que);
static void  resolver_service_query_listener (Jxta_object* msg, void* arg);
static void  resolver_service_response_listener (Jxta_object* msg, void* arg);
static long getid(Jxta_resolver_service_ref* resolver);

static const char *  INQUENAMESHORT = "IRes";
static const char *  OUTQUENAMESHORT = "ORes";
static const char *  SRDIQUENAMESHORT = "Srdi";
static const char *  JXTA_NAMESPACE = "jxta:";

/*
 * module methods
 */

/**
 * Initializes an instance of the Resolver Service.
 * 
 * @param service a pointer to the instance of the Resolver Service.
 * @param group a pointer to the PeerGroup the Resolver Service is 
 * initialized for.
 * @return Jxta_status
 *
 */

Jxta_status
jxta_resolver_service_ref_init(Jxta_module* resolver, Jxta_PG* group,
                                Jxta_id* assigned_id, Jxta_advertisement* impl_adv) {
    Jxta_listener* listener = NULL;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;

    /* Test arguments first */
    if ((resolver == NULL) || (group == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }


    apr_pool_create (&self->pool, NULL);
    self->queryhandlers = jxta_hashtable_new(3);
    self->responsehandlers = jxta_hashtable_new(3);
    self->query_id = 0;
    /* store a copy of our assigned id */
    if (assigned_id != 0) {
        JXTA_OBJECT_SHARE(assigned_id);
        self->assigned_id = assigned_id;
    }

    /* keep a reference to our impl adv */
    if (impl_adv != 0) {
        JXTA_OBJECT_SHARE(impl_adv);
    }
    self->group = group;
    self->impl_adv = impl_adv;

    jxta_PG_get_endpoint_service (group, &(self->endpoint));
    jxta_PG_get_rendezvous_service (group, &(self->rendezvous));
    jxta_PG_get_PID (group, &(self->localPeerId));
    /* Create the mutex */
    apr_thread_mutex_create (&self->mutex,
                             APR_THREAD_MUTEX_NESTED, /* nested */
                             self->pool);

    /**
     ** Build the local name of the instance
     **/
    self->instanceName = NULL;
    jxta_id_to_jstring( self->assigned_id, &self->instanceName );

    {       /* don't release this object */
        JString * grp_str = NULL;
        Jxta_id* gid;

        jxta_PG_get_GID(self->group, &gid);
        jxta_id_get_uniqueportion ( gid, &grp_str );
        JXTA_OBJECT_RELEASE(gid);

        self->inque  = jstring_new_2(jstring_get_string(grp_str));
        self->outque = jstring_new_2(jstring_get_string(grp_str));
        self->srdique = jstring_new_2(jstring_get_string(grp_str));

        /* append queue identifier */
        jstring_append_2(self->inque,INQUENAMESHORT);
        jstring_append_2(self->outque,OUTQUENAMESHORT);
        jstring_append_2(self->srdique,SRDIQUENAMESHORT);
    }

    return status;
}

/**
 * Initializes an instance of the Resolver Service. (exception variant).
 * 
 * @param service a pointer to the instance of the Resolver Service.
 * @param group a pointer to the PeerGroup the Resolver Service is 
 * initialized for.
 *
 */

static void
init_e (Jxta_module* resolver, Jxta_PG* group, Jxta_id* assigned_id,
        Jxta_advertisement* impl_adv, Throws) {
    Jxta_status s =
        jxta_resolver_service_ref_init(resolver, group, assigned_id, impl_adv);
    if (s != JXTA_SUCCESS)
        Throw(s);
}

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param this a pointer to the instance of the Resolver Service
 * @param argv a vector of string arguments.
 */
static Jxta_status
start(Jxta_module* resolver, char* argv[]) {
    Jxta_status status = JXTA_SUCCESS;
    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;

    jxta_PG_get_discovery_service (self->group, &(self->discovery));

    self->o_listener = jxta_listener_new ((Jxta_listener_func) resolver_service_query_listener,
                                  self,
                                  1,
                                  200);
    status = jxta_rdv_service_add_propagate_listener (self->rendezvous,
                    (char*) jstring_get_string(self->instanceName),
                    (char*) jstring_get_string(self->outque),
                    self->o_listener);
    if (status != JXTA_SUCCESS ) {
        return status;
    }

    jxta_listener_start(self->o_listener);

    self->i_listener = jxta_listener_new ((Jxta_listener_func) resolver_service_response_listener,
                                   self,
                                   1,
                                   200);
    status = jxta_rdv_service_add_propagate_listener (self->rendezvous,
                     (char*) jstring_get_string(self->instanceName),
                     (char*) jstring_get_string(self->inque),
                     self->i_listener);
    jxta_listener_start(self->i_listener);
           
    return status;
}

/**
 * Stops an instance of the Resolver Service.
 * 
 * @param service a pointer to the instance of the Resolver Service
 * @return Jxta_status
 */
static void
stop(Jxta_module* resolver) {
    Jxta_status status = JXTA_SUCCESS;
    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;

    if (NULL != self->i_listener) {
        status = jxta_rdv_service_remove_propagate_listener(self->rendezvous,
                     (char*) jstring_get_string(self->instanceName),
                     (char*) jstring_get_string(self->inque),
                     self->i_listener);
        jxta_listener_stop(self->i_listener);
        JXTA_OBJECT_RELEASE(self->i_listener);
        self->i_listener = NULL;
    }

    if (NULL != self->o_listener) {
        status = jxta_rdv_service_remove_propagate_listener(self->rendezvous,
                     (char*) jstring_get_string(self->instanceName),
                     (char*) jstring_get_string(self->outque),
                     self->o_listener);
        jxta_listener_stop(self->o_listener);
        JXTA_OBJECT_RELEASE(self->o_listener);
        self->o_listener = NULL;
    }

    if (NULL != self->discovery) {
        JXTA_OBJECT_RELEASE(self->discovery);
        self->discovery = NULL;
    }

    JXTA_LOG("Stopped.\n");
    /* nothing special to stop */
}

/*
 * service methods
 */

/**
 * return this service's own impl adv. (a service method).
 *
 * @param service a pointer to the instance of the Resolver Service
 * @return the advertisement.
 */
static void
get_mia(Jxta_service* resolver,  Jxta_advertisement** mia) {

    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
    PTValid(self, Jxta_resolver_service_ref);

    JXTA_OBJECT_SHARE(self->impl_adv);
    *mia = self->impl_adv;
}

/**
 * return an object that proxies this service. (a service method).
 *
 * @return this service object itself.
 */
static void
get_interface(Jxta_service* self, Jxta_service** svc) {
    PTValid(self, Jxta_resolver_service_ref);

    JXTA_OBJECT_SHARE(self);
    *svc = self;
}


/*
 * Resolver methods proper.
 */

/**
 * Registers a given ResolveHandler.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 */
Jxta_status
registerQueryHandler(Jxta_resolver_service* resolver,
                     JString* name,
                     Jxta_listener* handler) {
    char const *  hashname;
    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
    PTValid(self, Jxta_resolver_service_ref);
    hashname = jstring_get_string(name);

    if ( strlen(hashname) != 0) {
        jxta_hashtable_put(self->queryhandlers, hashname, strlen(hashname), (Jxta_object *)handler);
        return JXTA_SUCCESS;
    } else  {
        return JXTA_INVALID_ARGUMENT;
    }
}

/**
  * unregisters a given ResolveHandler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
static Jxta_status
unregisterQueryHandler(Jxta_resolver_service * resolver,
                       JString* name) {
    Jxta_object * ignore = 0;
    char const *  hashname;
    Jxta_status status;

    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
    PTValid(self, Jxta_resolver_service_ref);

    hashname = jstring_get_string(name);

    status = jxta_hashtable_del(self->queryhandlers, hashname, strlen(hashname), & ignore);
    if (ignore != 0)
        JXTA_OBJECT_RELEASE(ignore);
    return status;
}
/**
 * Registers a given ResolveHandler.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 */
Jxta_status
registerResponseHandler(Jxta_resolver_service* resolver,
                        JString * name,
                        Jxta_listener* handler) {
    char const *  hashname;
    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
    PTValid(self, Jxta_resolver_service_ref);
    hashname = jstring_get_string(name);

    if ( strlen(hashname) != 0) {
        jxta_hashtable_put(self->responsehandlers, hashname, strlen(hashname), (Jxta_object *)handler);
        return JXTA_SUCCESS;
    } else  {
        return JXTA_INVALID_ARGUMENT;
    }
}

/**
  * unregisters a given ResolveHandler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
static Jxta_status
unregisterResponseHandler(Jxta_resolver_service * resolver,
                          JString* name) {
    Jxta_object * ignore = 0;
    char const *  hashname;
    Jxta_status status;

    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
    PTValid(self, Jxta_resolver_service_ref);

    hashname = jstring_get_string(name);

    status = jxta_hashtable_del(self->responsehandlers, hashname, strlen(hashname), & ignore);
    if (ignore != 0)
        JXTA_OBJECT_RELEASE(ignore);
    return status;
}


static Jxta_status
sendQuery(Jxta_resolver_service* resolver,
           ResolverQuery* query,
           Jxta_id* peerid) {

        Jxta_message* msg = NULL;
	Jxta_message_element* msgElem;
	Jxta_endpoint_address* address;
	char * tmp;
	JString * doc;
	Jxta_status status;
	JString* el_name;
	Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
	
	PTValid(self, Jxta_resolver_service_ref);
	/* Test arguments first */
	if ((self == NULL) ||  (query == NULL) )
	{
		/* Invalid args. */
		return JXTA_INVALID_ARGUMENT;
	}
	
        jxta_resolver_query_set_queryid (query, getid(self) );
	status = jxta_resolver_query_get_xml(query, & doc);
	if (status != JXTA_SUCCESS ) {
		return status;
	}

	/**
	 ** XXX fixme hamada@jxta.org 3/09/2002
	 ** FIXME 3/8/2002 lomax@jxta.org
	 ** Allocate a buffer than can be freed by the message 
	 ** This is currentely needed because the message API does not yet
	 ** support sharing of data. When that will be completed, we will be
	 ** able to avoid this extra copy.
	 **/	

	tmp = (char *)  jstring_get_string (doc);
	tmp = malloc (jstring_length(doc) + 1);
	tmp[jstring_length(doc)] = 0;
	memcpy (tmp, jstring_get_string(doc), jstring_length(doc));

	msg = jxta_message_new ();

	el_name = jstring_new_2(JXTA_NAMESPACE);
	jstring_append_1(el_name, self->outque);
	msgElem =  jxta_message_element_new_1 (jstring_get_string(el_name),
	                                       "text/xml",
	                                       tmp,
	                                       strlen(tmp),
	                                       NULL);
	JXTA_OBJECT_RELEASE(el_name);
	if (msgElem == NULL) {
		JXTA_LOG ("failed to create message element.\n");
		free (tmp);
		JXTA_OBJECT_RELEASE(msg);
		return JXTA_INVALID_ARGUMENT;
	}
	jxta_message_add_element (msg, msgElem);

	if (peerid == NULL) {
		jxta_rdv_service_propagate(self->rendezvous,
		                           msg,
		                           (char *)jstring_get_string(self->instanceName),
		                           (char*) jstring_get_string(self->outque),
		                           1);
	} else  {
		address = get_endpoint_address_from_peerid (peerid,
		                                     (char *)jstring_get_string(self->instanceName),
						     (char*) jstring_get_string(self->outque));
		status = jxta_endpoint_service_send (self->group, self->endpoint, msg, address);
		if (status != JXTA_SUCCESS) {
		  JXTA_LOG("Failed to send resolver message\n");
		}
		JXTA_OBJECT_RELEASE(address);
		JXTA_OBJECT_RELEASE(msg);
	}	
	JXTA_OBJECT_RELEASE(msgElem);
	JXTA_OBJECT_RELEASE(doc);
	free (tmp);

  return JXTA_SUCCESS;

}
/* XXX fixme hamada@jxta.org breakout the sending part out into static function both
   send query, and response will use 
 */

static Jxta_status
sendResponse(Jxta_resolver_service* resolver,
              ResolverResponse* response,
              Jxta_id* peerid)
{
	Jxta_message* msg;
	Jxta_message_element* msgElem;
	Jxta_endpoint_address* address;
	JString * doc;
	char * tmp;
	Jxta_status status;
        JString * pid;
	JString* el_name;

	Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
	PTValid(self, Jxta_resolver_service_ref);

	/* Test arguments first */
	if ((resolver == NULL) ||  (response == NULL) )
	{
		/* Invalid args. */
		return JXTA_INVALID_ARGUMENT;
	}
	status = jxta_resolver_response_get_xml(response, & doc);
	if (status != JXTA_SUCCESS ) {
		return status;
	}

	/**
	 ** XXX fixme hamada@jxta.org 3/09/2002
	 ** FIXME 3/8/2002 lomax@jxta.org
	 ** Allocate a buffer than can be freed by the message 
	 ** This is currentely needed because the message API does not yet
	 ** support sharing of data. When that will be completed, we will be
	 ** able to avoid this extra copy.
	 **/	

	tmp = (char *)  jstring_get_string (doc);
	tmp = malloc (jstring_length(doc) + 1);
	tmp[jstring_length(doc)] = 0;
	memcpy (tmp, jstring_get_string(doc), jstring_length(doc));

	JXTA_OBJECT_RELEASE (doc);
	msg = jxta_message_new ();

	el_name = jstring_new_2(JXTA_NAMESPACE);
	jstring_append_1(el_name, self->inque);
	msgElem =  jxta_message_element_new_1 (jstring_get_string(el_name),
	                                       "text/xml",
	                                       tmp,
	                                       strlen(tmp),
	                                       NULL);
	JXTA_OBJECT_RELEASE(el_name);

	if (msgElem == NULL) {
		JXTA_LOG ("failed to create message element.\n");
		free (tmp);
		JXTA_OBJECT_RELEASE(msg);
		return JXTA_INVALID_ARGUMENT;
	}
	jxta_message_add_element (msg, msgElem);

	if (peerid != NULL) {
	  jxta_id_to_jstring ( peerid, &pid );
	  JXTA_LOG ("Send resolver response to peer %s\n", jstring_get_string(pid));
	}

	if (peerid == NULL) {
		jxta_rdv_service_propagate(self->rendezvous,
		                           msg,
		                           (char *)jstring_get_string(self->instanceName),
		                           (char*) jstring_get_string(self->inque),
		                           1);
	} else  {
		/* unicast */
		address = get_endpoint_address_from_peerid (peerid,
		                                     (char *)jstring_get_string(self->instanceName),
						     (char*) jstring_get_string(self->inque));

		JXTA_LOG ("address message to %s://%s/%s/%s\n",
			  jxta_endpoint_address_get_protocol_name (address),
			  jxta_endpoint_address_get_protocol_address (address),
			  jxta_endpoint_address_get_service_name (address),
			  jxta_endpoint_address_get_service_params (address));
		
		status = jxta_endpoint_service_send (self->group, self->endpoint, msg, address);
		if (status != JXTA_SUCCESS) {
		  JXTA_LOG("Failed to send resolver message\n");
		}
		JXTA_OBJECT_RELEASE(address);
		JXTA_OBJECT_RELEASE(msg);
	}
	
	JXTA_OBJECT_RELEASE(msgElem);
	free (tmp);
	return JXTA_SUCCESS;
}

static Jxta_status
sendSrdi(Jxta_resolver_service* resolver,
           ResolverSrdi* message,
           Jxta_id* peerid) {

    Jxta_message* msg = NULL;
    Jxta_message_element* msgElem = NULL;
    Jxta_endpoint_address* address = NULL;
    char * tmp;
    JString * doc = NULL;
    Jxta_status status;
    JString* el_name = NULL;

    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*) resolver;
    
    JXTA_LOG("Send SRDI resolver message \n");

    PTValid(self, Jxta_resolver_service_ref);
    /* Test arguments first */
    if ((self == NULL) ||  (message == NULL) ) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    status = jxta_resolver_srdi_get_xml(message, &doc);
    if (status != JXTA_SUCCESS ) {
        return status;
    }

    tmp = malloc(jstring_length(doc) + 1);
    tmp[jstring_length(doc)] = 0;
    memcpy(tmp, jstring_get_string(doc), jstring_length(doc));

    msg = jxta_message_new();
    el_name = jstring_new_2(JXTA_NAMESPACE);
    jstring_append_1(el_name, self->srdique);
    msgElem =  jxta_message_element_new_1 (jstring_get_string(el_name),
                                           "text/xml",
                                           tmp,
                                           strlen(tmp),
                                           NULL);
    JXTA_OBJECT_RELEASE(el_name);

    if (msgElem == NULL) {
        JXTA_LOG ("failed to create message element.\n");
        free (tmp);
        JXTA_OBJECT_RELEASE(msg);
        return JXTA_INVALID_ARGUMENT;
    }
    jxta_message_add_element(msg, msgElem);

    if (peerid == NULL) {
        jxta_rdv_service_propagate(self->rendezvous,
                                   msg,
                                   (char*) jstring_get_string(self->instanceName),
                                   (char*) jstring_get_string(self->srdique),
                                   1);
    } else  { 
        address = get_endpoint_address_from_peerid (peerid,
                  (char *)jstring_get_string(self->instanceName),
                  (char*) jstring_get_string(self->srdique));
        status = jxta_endpoint_service_send (self->group, self->endpoint, msg, address);
	if (status != JXTA_SUCCESS) {
	  JXTA_LOG("Failed to send srdi message\n");
	}
        JXTA_OBJECT_RELEASE(address);
	JXTA_OBJECT_RELEASE(msg);
    }
   
    JXTA_OBJECT_RELEASE(msgElem);
    JXTA_OBJECT_RELEASE(doc);
    free (tmp);
    return JXTA_SUCCESS;
}


/* BEGINING OF STANDARD SERVICE IMPLEMENTATION CODE */

/**
 * This implementation's methods.
 * The typedef could go in jxta_resolver_service_private.h if we wanted
 * to support subclassing of this class. We don't have to and so the
 * alias Jxta_resolver_service_methods -> Jxta_resolver_service_ref_methods
 * is purely academic, but that'll be less work to do later if someone
 * wants to subclass.
 */

typedef Jxta_resolver_service_methods Jxta_resolver_service_ref_methods;

Jxta_resolver_service_ref_methods jxta_resolver_service_ref_methods = {
            {
                {
                    "Jxta_module_methods",
                    jxta_resolver_service_ref_init, /* temp long name, for testing */
                    init_e,
                    start,
                    stop
                },
                "Jxta_service_methods",
                get_mia,
                get_interface
            },
            "Jxta_resolver_service_methods",
            registerQueryHandler,
            unregisterQueryHandler,
            registerResponseHandler,
            unregisterResponseHandler,
            sendQuery,
            sendResponse,
	    sendSrdi
        };

void
jxta_resolver_service_ref_construct(Jxta_resolver_service_ref* self,
                                    Jxta_resolver_service_ref_methods* methods) {
    /*
     * we do not extend Jxta_resolver_service_methods; so the type string
     * is that of the base table
     */
    PTValid(methods, Jxta_resolver_service_methods);

    jxta_resolver_service_construct((Jxta_resolver_service*) self,
                                    (Jxta_resolver_service_methods*) methods);

    /* Set our rt type checking string */
    self->thisType = "Jxta_resolver_service_ref";
}

void jxta_resolver_service_ref_destruct(Jxta_resolver_service_ref* self) {
    PTValid(self, Jxta_resolver_service_ref);

    /* release/free/destroy our own stuff */

    if (self->endpoint != 0) {
        JXTA_OBJECT_RELEASE (self->endpoint);
    }
    if (self->discovery != 0) {
        JXTA_OBJECT_RELEASE (self->discovery);
    }
    if (self->localPeerId != 0) {
        JXTA_OBJECT_RELEASE (self->localPeerId);
    }
    if (self->queryhandlers != 0) {
        JXTA_OBJECT_RELEASE (self->queryhandlers);
    }
    if (self->responsehandlers != 0) {
        JXTA_OBJECT_RELEASE (self->responsehandlers);
    }
    if (self->instanceName != 0) {
        JXTA_OBJECT_RELEASE(self->instanceName);
    }

    self->group = NULL;

    if (self->impl_adv != 0) {
        JXTA_OBJECT_RELEASE (self->impl_adv);
    }
    if (self->assigned_id != 0) {
        JXTA_OBJECT_RELEASE(self->assigned_id);
    }
    if (self->pool != 0) {
        apr_pool_destroy (self->pool);
    }

    /* call the base classe's dtor. */
    jxta_resolver_service_destruct((Jxta_resolver_service*) self);

    JXTA_LOG("Destruction finished\n");
}

/**
 * Frees an instance of the Resolver Service.
 * Note: freeing of memory pool is the responsibility of the caller.
 * 
 * @param service a pointer to the instance of the Resolver Service to free.
 */
static void
resol_free (void* service) {
    /* call the hierarchy of dtors */
    jxta_resolver_service_ref_destruct((Jxta_resolver_service_ref*) service);

    /* free the object itself */
    free (service);
}


/**
 * Creates a new instance of the Resolver Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_resolver_service_init().
 *
 * @param pool a pointer to the pool of memory that needs to be used in order
 * to allocate the Resolver Service.
 * @return a non initialized instance of the Resolver Service
 */

Jxta_resolver_service_ref*
jxta_resolver_service_ref_new_instance (void) {
    /* Allocate an instance of this service */
    Jxta_resolver_service_ref* self = (Jxta_resolver_service_ref*)
                                      malloc (sizeof (Jxta_resolver_service_ref));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset (self, 0, sizeof (Jxta_resolver_service_ref));
    JXTA_OBJECT_INIT(self, (JXTA_OBJECT_FREE_FUNC) resol_free, 0);

    /* call the hierarchy of ctors */
    jxta_resolver_service_ref_construct(self,
                                        &jxta_resolver_service_ref_methods);

    /* return the new object */
    return self;
}

/* END OF STANDARD SERVICE IMPLEMENTATION CODE */



static void
resolver_service_query_listener (Jxta_object* obj, void* arg) {

	Jxta_message*              msg = (Jxta_message*) obj;
	Jxta_message_element * element = NULL;
	Jxta_object *          handler = NULL;
	ResolverQuery *             rq = NULL;
	JString  *         handlername = NULL;
	Jxta_status status;
	Jxta_resolver_service_ref * resolver = (Jxta_resolver_service_ref *)arg;
	JString* pid = NULL;
	JString* el_name;
	Jxta_RouteAdvertisement* route = NULL;
	Jxta_AccessPointAdvertisement* dest = NULL;

	JXTA_LOG ("Resolver Query Listener, query received\n");
        /*jxta_message_print(msg);*/

	JXTA_OBJECT_CHECK_VALID (msg);
	JXTA_OBJECT_CHECK_VALID (resolver);

	el_name = jstring_new_2(JXTA_NAMESPACE);
	jstring_append_1(el_name, resolver->outque);
	status = jxta_message_get_element_1 (msg, (char*)
	                                     jstring_get_string(el_name),
	                                     &element );
	JXTA_OBJECT_RELEASE(el_name);

	if (element) {
		Jxta_bytevector*       bytes = NULL;
		JString*               string = NULL;
		
		rq = jxta_resolver_query_new();
		bytes = jxta_message_element_get_value (element);
		string = jstring_new_3( bytes );
		JXTA_OBJECT_RELEASE(bytes); bytes = NULL;
		jxta_resolver_query_parse_charbuffer(rq, jstring_get_string (string), jstring_length(string) );
		JXTA_OBJECT_RELEASE(string); string = NULL;
		JXTA_OBJECT_RELEASE (element);
		JXTA_OBJECT_RELEASE (msg);
	} else {
	        JXTA_OBJECT_RELEASE (msg);
		return;
	}

	jxta_id_to_jstring ( jxta_resolver_query_get_src_peer_id (rq), &pid );
	JXTA_LOG ("Resolver response to peer %s\n", jstring_get_string(pid));

	/*
	 * check if we have a route info as part of the query to respond
	 */
	route = jxta_resolver_query_get_src_peer_route(rq);
	if (route != NULL) {
	  /* 
	   * check if we do have a valid destination, we may have been
	   * send a NULL route info
	   */
	  dest = jxta_RouteAdvertisement_get_Dest(route);
	  if (dest != 0) {
	    if (resolver->discovery == NULL) {
	      jxta_PG_get_discovery_service (resolver->group, &(resolver->discovery));
	    }
	    
	    if (resolver->discovery != NULL) {
	      status = discovery_service_publish(resolver->discovery,
						 (Jxta_advertisement *)  route,
						 DISC_ADV,
						 (Jxta_expiration_time) ROUTEADV_DEFAULT_LIFETIME,
						 (Jxta_expiration_time) ROUTEADV_DEFAULT_EXPIRATION);
	      if (status != JXTA_SUCCESS) {
		JXTA_LOG("Failed to publish sender route %d\n", status);
	      }	  
	    }
	    JXTA_OBJECT_RELEASE(dest);
	  }
	  JXTA_OBJECT_RELEASE(route);
	}

	jxta_resolver_query_get_handlername (rq, &handlername);

	if (handlername != NULL) {
	  status = jxta_hashtable_get(
				      resolver->queryhandlers,
				      jstring_get_string(handlername),
				      jstring_length(handlername),
				      & handler);
	  /* call the query handler with the query */
	  if (handler != NULL) {
	    status = jxta_listener_schedule_object ((Jxta_listener*) handler,
						    (Jxta_object* ) rq);
	    JXTA_OBJECT_RELEASE (handler);
	  }
	  JXTA_OBJECT_RELEASE (rq);
	  JXTA_OBJECT_RELEASE(handlername);
	  handlername=NULL;	
	}
}

static void
resolver_service_response_listener (Jxta_object* obj, void* arg) {

	Jxta_message*             msg = (Jxta_message*) obj;
	Jxta_message_element* element = NULL;
	Jxta_object *         handler = NULL;
	JString  * handlername        = NULL;
	ResolverResponse * rr         = NULL;
	Jxta_status status;
	JString* el_name;

	Jxta_resolver_service_ref * resolver = (Jxta_resolver_service_ref *)arg;

	JXTA_LOG ("Resolver Response Listener, response received\n");
        /*jxta_message_print(msg);*/

	el_name = jstring_new_2(JXTA_NAMESPACE);
	jstring_append_1(el_name, resolver->inque);
	status = jxta_message_get_element_1 (msg, (char*)
	                                     jstring_get_string(el_name),
	                                     &element );
	JXTA_OBJECT_RELEASE(el_name);

	if (element) {
		Jxta_bytevector*       bytes = NULL;
		JString*               string = NULL;

		rr = jxta_resolver_response_new();
		bytes = jxta_message_element_get_value (element);
		string = jstring_new_3( bytes );
		JXTA_OBJECT_RELEASE(bytes); bytes = NULL;
		jxta_resolver_response_parse_charbuffer(rr, jstring_get_string (string), jstring_length(string) );
		JXTA_OBJECT_RELEASE(string); string = NULL;
		JXTA_OBJECT_RELEASE (element);
		JXTA_OBJECT_RELEASE (msg);
	} else {
	        JXTA_OBJECT_RELEASE (msg);
		return;
	}
	
	jxta_resolver_response_get_handlername (rr, &handlername);

	if (handlername != NULL) {
	status = jxta_hashtable_get(
	                 resolver->responsehandlers,
	                 jstring_get_string(handlername),
	                 jstring_length(handlername),
	                 & handler);

	if (handler != NULL) {
		status = jxta_listener_schedule_object ((Jxta_listener*) handler,
		                                        (Jxta_object* ) rr);
		JXTA_OBJECT_RELEASE (handler);
	}
	JXTA_OBJECT_RELEASE (rr);
	JXTA_OBJECT_RELEASE(handlername);
	handlername=NULL;
	}


}

/**
 * generate a unique id within this instance of this service on this peer
 *
 */
static long getid(Jxta_resolver_service_ref * resolver) {

    PTValid(resolver, Jxta_resolver_service_ref);
    apr_thread_mutex_lock (resolver->mutex);
    resolver->query_id ++;
    JXTA_LOG("querid :%ld\n",resolver->query_id);
    apr_thread_mutex_unlock (resolver->mutex);
    return resolver->query_id;
}

static Jxta_endpoint_address*
get_endpoint_address_from_peerid (Jxta_id* peerid, char * name, char * que) {
    JString* tmp = NULL;
    Jxta_endpoint_address* addr = NULL;
    char* pt;

    JXTA_OBJECT_CHECK_VALID (peerid);

    tmp = NULL;
    jxta_id_get_uniqueportion ( peerid, &tmp );

    JXTA_OBJECT_CHECK_VALID (tmp);

    if (tmp == NULL) {
        JXTA_LOG ("Cannot get the unique portion of the peer id\n");
        return NULL;
    }

    pt = (char*) jstring_get_string (tmp);
    addr = jxta_endpoint_address_new2 ((char*) "jxta", pt, name, que);

    JXTA_OBJECT_CHECK_VALID (addr);

    JXTA_OBJECT_RELEASE (tmp);
    return addr;
}

/* vim: set ts=4 sw=4 et tw=130: */
