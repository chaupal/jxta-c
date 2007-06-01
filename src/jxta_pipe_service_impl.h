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
 * $Id: jxta_pipe_service_impl.h,v 1.7 2005/06/16 23:11:47 slowhog Exp $
 */


#ifndef __JXTA_PIPE_SERVICE_IMPL_H__
#define __JXTA_PIPE_SERVICE_IMPL_H__

#include "jxta_service.h"

/* #include "jxta_types.h" */
#include "jxta_listener.h"
#include "jxta_vector.h"
#include "jxta_pipe_adv.h"
#include "jxta_peer.h"
#include "jxta_message.h"


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
/**
   ** Type of an instance of a pipe
   ** A Jxta_pipe is a Jxta_object.
   **/ typedef struct _jxta_pipe Jxta_pipe;


  /**
   ** Type of an input pipe
   **/
typedef struct _jxta_inputpipe Jxta_inputpipe;

  /**
   ** Type of an output pipe
   **/
typedef struct _jxta_outputpipe Jxta_outputpipe;

  /**
   ** Type of a Pipe Resolver
   **/
typedef struct _jxta_pipe_resolver Jxta_pipe_resolver;

  /**
   ** Type of a Pipe Service Implementation
   **/
typedef struct _jxta_pipe_service_impl Jxta_pipe_service_impl;



  /**
   ** Type of the JXTA Pipe Service.
   ** A Jxta_pipe_service is a Jxta_object.
   **/
typedef struct _jxta_pipe_service Jxta_pipe_service;

struct _jxta_pipe_service_impl {

    JXTA_OBJECT_HANDLE;

    char *(*get_name) (Jxta_pipe_service_impl * service);

     Jxta_status(*timed_connect) (Jxta_pipe_service_impl * service,
                                  Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe);

     Jxta_status(*connect) (Jxta_pipe_service_impl * service,
                            Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_listener * listener);

     Jxta_status(*timed_accept) (Jxta_pipe_service_impl * service,
                                 Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_pipe ** pipe);


     Jxta_status(*deny) (Jxta_pipe_service_impl * service, Jxta_pipe_adv * adv);

     Jxta_status(*add_accept_listener) (Jxta_pipe_service_impl * service, Jxta_pipe_adv * adv, Jxta_listener * listener);

     Jxta_status(*remove_accept_listener) (Jxta_pipe_service_impl * service, Jxta_pipe_adv * adv, Jxta_listener * listener);

     Jxta_status(*get_pipe_resolver) (Jxta_pipe_service_impl * service, Jxta_pipe_resolver ** resolver);

     Jxta_status(*set_pipe_resolver) (Jxta_pipe_service_impl * service, Jxta_pipe_resolver * jnew, Jxta_pipe_resolver ** old);
};



struct _jxta_pipe_resolver {

    JXTA_OBJECT_HANDLE;

    Jxta_status(*local_resolve) (Jxta_pipe_resolver * resolver, Jxta_pipe_adv * adv, Jxta_vector ** peers);

    Jxta_status(*timed_remote_resolve) (Jxta_pipe_resolver * resolver,
                                        Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_peer * dest, Jxta_vector ** peers);

    Jxta_status(*remote_resolve) (Jxta_pipe_resolver * resolver,
                                  Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_peer * dest, Jxta_listener * listener);

    Jxta_status(*send_srdi) (Jxta_pipe_resolver * resolver, Jxta_pipe_adv * adv, Jxta_id * dest, Jxta_boolean adding);
};


struct _jxta_pipe {

    JXTA_OBJECT_HANDLE;

    Jxta_status(*get_outputpipe) (Jxta_pipe * pipe, Jxta_outputpipe ** op);


    Jxta_status(*get_inputpipe) (Jxta_pipe * pipe, Jxta_inputpipe ** ip);

    Jxta_status(*get_remote_peers) (Jxta_pipe * pipe, Jxta_vector ** vector);
};


struct _jxta_inputpipe {

    JXTA_OBJECT_HANDLE;

    Jxta_status(*timed_receive) (Jxta_inputpipe * ip, Jxta_time_diff timeout, Jxta_message ** msg);

    Jxta_status(*add_listener) (Jxta_inputpipe * ip, Jxta_listener * listener);

    Jxta_status(*remove_listener) (Jxta_inputpipe * ip, Jxta_listener * listener);
};


struct _jxta_outputpipe {

    JXTA_OBJECT_HANDLE;

    Jxta_status(*send) (Jxta_outputpipe * op, Jxta_message * msg);

    Jxta_status(*add_listener) (Jxta_outputpipe * op, Jxta_listener * listener);

    Jxta_status(*remove_listener) (Jxta_outputpipe * op, Jxta_listener * listener);
};

  /****************************************************
   ** Jxta_pipe_resolver_event API
   ****************************************************/

  /**
   ** The following is the predefined standard event type
   ** Specific pipe resolver implementation can use types higher than
   ** JXTA_PIPE_RESOLVER_BASE.
   **/

#define JXTA_PIPE_RESOLVER_EVENT_STANDARD_SERVICE_BASE 100
#define JXTA_PIPE_RESOLVER_EVENT_USER_BASE             1000

#define JXTA_PIPE_RESOLVER_RESOLVED (JXTA_PIPE_RESOLVER_EVENT_STANDARD_SERVICE_BASE + 1)
#define JXTA_PIPE_RESOLVER_TIMEOUT  (JXTA_PIPE_RESOLVER_EVENT_STANDARD_SERVICE_BASE + 2)

typedef struct _jxta_pipe_resolver_event Jxta_pipe_resolver_event;


  /**
   ** Create a Jxta_pipe_resolver_event
   **
   ** @param ev type of the event
   ** @param adv a Jxta_pipe_adv associated with the event.
   ** @param peers an optional pointer to vector containing the list of Jxta_peer that have
   ** been found.
   ** @return a new Jxta_pipe_resolver_event.
   **/
Jxta_pipe_resolver_event *jxta_pipe_resolver_event_new(int ev, Jxta_pipe_adv * adv, Jxta_vector * peers);

  /**
   ** Return the event type of the event
   **
   ** @param ev type of the event
   ** @return the type of the event.
   **/
int jxta_pipe_resolver_event_get_event(Jxta_pipe_resolver_event * self);

  /**
   ** Return the Jxta_pipe associated with the event (if any).
   **
   ** @param self the event
   ** @param peers returned value: a vector that contains the list of Jxta_peer that have been found.
   ** @return JXTA_SUCCESS when succesfull.
   **         JXTA_FAILED when the event was not associated to any pipe.
   **/
Jxta_status jxta_pipe_resolver_event_get_peers(Jxta_pipe_resolver_event * self, Jxta_vector ** peers);

  /**
   ** Return the Jxta_pipe_adv associated with the event (if any).
   **
   ** @param self the event
   ** @param adv returned value: a Jxta_pipe_adv.
   ** @return JXTA_SUCCESS when succesfull.
   **         JXTA_FAILED when the event was not associated to any advertisement.
   **/
Jxta_status jxta_pipe_resolver_event_get_adv(Jxta_pipe_resolver_event * self, Jxta_pipe_adv ** adv);

  /**
   ** Query the Pipe Service Resolver for a particular pipe adverisement. The
   ** Pipe Resolver will only search in its own local cache.
   ** 
   ** @param service instance of the Pipe Service
   ** @param adv a pointer to the pipe advertismeent
   ** @param peers returned value: a vector containing a list of pointers to Jxta_peers
   **                              which are listening for the given pipe advertisement.
   **                              The vector can be empty or NULL.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid.
   **                          JXTA_FAILED when there is no local listener.
   **/
Jxta_status jxta_pipe_resolver_local_resolve(Jxta_pipe_resolver * resolver, Jxta_pipe_adv * adv, Jxta_vector ** peers);

  /**
   ** Remote query the Pipe Service Resolver for a particular pipe adverisement. The
   ** Pipe Resolver will issue a query and send it to other peers. This function will
   ** block until the timeout is reached.
   ** 
   ** @param service instance of the Pipe Service
   ** @param adv a pointer to the pipe advertismeent
   ** @param timeout waiting time in micro-seconds. If timeout is set to zero, the function
   **                will return immediately, and peers will be set to NULL. This is typical
   **                when a revolve listener has been set, and the query has to be sent.
   ** @param dest if non NULL, dest is the destination of the pipe resolver query.
   ** @param peers returned value: a vector containing a list of pointers to Jxta_peers
   **                              which are listening for the given pipe advertisement.
   **                              The vector can be empty or NULL.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid.
   **                          JXTA_TIMEOUT when the pipe could not be resolved.
   **/
Jxta_status jxta_pipe_resolver_timed_remote_resolve(Jxta_pipe_resolver * resolver,
                                                    Jxta_pipe_adv * adv,
                                                    Jxta_time_diff timeout, Jxta_peer * dest, Jxta_vector ** peers);

  /**
   ** Remote query the Pipe Service Resolver for a particular pipe adverisement. The
   ** Pipe Resolver will issue a query and send it to other peers. This function will
   ** block until the timeout is reached.
   ** 
   ** @param service instance of the Pipe Service
   ** @param adv a pointer to the pipe advertismeent
   ** @param timeout waiting time in micro-seconds. If timeout is set to zero, the function
   **                will return immediately, and peers will be set to NULL. This is typical
   **                when a revolve listener has been set, and the query has to be sent.
   ** @param dest if non NULL, dest is the destination of the pipe resolver query.
   ** @param listener a pointer to a listener which will receive resolver events of type
   **                 jxta_pipe_resolver_event. At least one event will be received at the
   **                 timeout is reached.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid.
   **/
Jxta_status jxta_pipe_resolver_remote_resolve(Jxta_pipe_resolver * resolver,
                                              Jxta_pipe_adv * adv,
                                              Jxta_time_diff timeout, Jxta_peer * dest, Jxta_listener * listener);


  /**
   ** Send a SrdiMessage of the pipe instance to a rendezvous peer
   **
   ** @param service instance of the Pipe Service
   ** @param adv a pointer to the pipe advertismeent
   ** @param dest if non NULL, dest is the destination of the pipe resolver query.
   ** @param adding TRUE when creating a new pipe, FALSE when closing the pipe
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid.
   **/
Jxta_status jxta_pipe_resolver_send_srdi_message(Jxta_pipe_resolver * resolver,
                                                 Jxta_pipe_adv * adv, Jxta_id * dest, Jxta_boolean adding);

  /**
   ** Get the name of the pipe implementation.
   ** Note that the returned char* is a pointer to the internal string.
   ** This pointer must be considered read-only and is not to be freed.
   **
   ** @param service instance of the Pipe Service Impl
   ** @return the name of the Pipe Service Impl.
   **/
char *jxta_pipe_service_impl_get_name(Jxta_pipe_service_impl * service);


  /**
   ** Try to synchronously connect to the remote end of a pipe.
   ** If peers is set to a vector containing a list of Jxta_peers*,
   ** the connection will be attempted to be established with the listed
   ** peer(s). The number of peers that the vector may contain depends on
   ** the type of type. Unicast typically can only connect to a single peer.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisment of the pipe to connect to.
   ** @param timeout timeout in micro-seconds
   ** @param peers optional vector of pointers to Jxta_peer.
   ** @param pipe returned value, a pipe connected to a remote end.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_TIMEOUT when the timeout has been reached and no connection
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
Jxta_status jxta_pipe_service_impl_timed_connect(Jxta_pipe_service_impl * service,
                                                 Jxta_pipe_adv * adv,
                                                 Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe);

  /**
   ** Deny incoming connection requests.
   **
   ** The pipe is marked as not being able to receive connections.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisment of the pipe to connect to.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
Jxta_status jxta_pipe_service_impl_deny(Jxta_pipe_service_impl * service, Jxta_pipe_adv * adv);

  /**
   ** Try to asynchronously connect to the remote end of a pipe.
   ** If peers is set to a vector containing a list of Jxta_peers*,
   ** the connection will be attempted to be established with the listed
   ** peer(s). The number of peers that the vector may contain depends on
   ** the type of type. Unicast typically can only connect to a single peer.
   **
   ** The listener which is associated to the connect request is guaranteed to
   ** be called exactely once. If the timeout is reached before a connection
   ** was established, the listener is invoked with an error event.
   ** Connection events are objects (that need to be released after used),
   ** of type Jxta_pipe_connect_event.
   **
   ** After the listener is invoked, the Pipe Service removes it automatically.
   ** (the listener object is released).
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisment of the pipe to connect to.
   ** @param timeout timeout in micro-seconds
   ** @param peers optional vector of pointers to Jxta_peer.
   ** @param pipe returned value, a pipe connected to a remote end.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
Jxta_status jxta_pipe_service_impl_connect(Jxta_pipe_service_impl * service,
                                           Jxta_pipe_adv * adv,
                                           Jxta_time_diff timeout, Jxta_vector * peers, Jxta_listener * listener);

  /**
   ** Accept an incoming connection.
   ** This function waits until the pipe is ready to receive messages, or until
   ** the timeout is reached. The semantics of being ready to receive messages
   ** depends on the type of pipe. After the first call to connect, the pipe
   ** is set to wait for connection request. If a connection request arrives after
   ** the timeout has been reached, it will be queued up, and a following call to
   ** jxta_pipe_service_timed_accept() will retrieve the connection request, until
   ** the pipe is released, or jxta_pipe_service_deny() is called.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisment of the pipe to connect to.
   ** @param timeout timeout in micro-seconds
   ** @param pipe returned value, a pipe connected to a remote end.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_BUSY when the pipe is already receiving messages.
   **/
Jxta_status jxta_pipe_service_impl_timed_accept(Jxta_pipe_service_impl * service,
                                                Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_pipe ** pipe);


  /**
   ** Accept by setting a connection listener.
   ** As long as the listener is set, any connection request on the pipe
   ** will generate a Jxta_pipe_connect_event.
   **
   ** Depending on the semantics of the type of pipe, zero, one or several
   ** connect event can be generated.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisment of the pipe to connect to.
   ** @param listener a Jxta_listener which will handle the connect events.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_BUSY when the pipe is already accepting messages.
   **/
Jxta_status jxta_pipe_service_impl_add_accept_listener(Jxta_pipe_service_impl * service,
                                                       Jxta_pipe_adv * adv, Jxta_listener * listener);

  /**
   ** Remove a connection listener.
   **
   ** The listener is removed, and the pipe is set to deny incoming connection.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisment of the pipe to connect to.
   ** @param listener a Jxta_listener which will handle the connect events.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
Jxta_status jxta_pipe_service_impl_remove_accept_listener(Jxta_pipe_service_impl * service,
                                                          Jxta_pipe_adv * adv, Jxta_listener * listener);

  /**
   ** Get the default (peergroup) Pipe Resolver.
   ** 
   ** @param service pointer to the Pipe Service
   ** @param resolver returned value:  a pointer to a pointer to a Jxta_pipe_resolver
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **/
Jxta_status jxta_pipe_service_impl_get_pipe_resolver(Jxta_pipe_service_impl * service, Jxta_pipe_resolver ** resolver);

  /**
   ** Set the default (peergroup) Pipe Resolver.
   ** 
   ** @param service pointer to the Pipe Service
   ** @param new a pointer to the new default resolver.
   ** @param old returned value: if non NULL, old will contain a pointer
   **                            to the previous pipe resolver.
   **
   ** @return an error status. JXTA_SUCCESS when successfull.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_VIOLATION when changing the default pipe is not authorized
   **/
Jxta_status jxta_pipe_service_impl_set_pipe_resolver(Jxta_pipe_service_impl * service,
                                                     Jxta_pipe_resolver * jnew, Jxta_pipe_resolver ** old);




#endif

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vim: set ts=4 sw=4 tw=130 et: */
