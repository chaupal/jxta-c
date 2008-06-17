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


#ifndef __JXTA_PIPE_SERVICE_H__
#define __JXTA_PIPE_SERVICE_H__

#include "jxta_pipe_service_impl.h"

/* #include "jxta_types.h" */
/* #include "jxta_listener.h" */
/* #include "jxta_vector.h" */
/* #include "jxta_pipe_adv.h" */
/* #include "jxta_peer.h" */


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/****************************************************
 ** Jxta_pipe_connect_event API
****************************************************/

/**
 ** The following is the predefined standard event types.
 ** Specific pipe implementation can use types higher than
 ** JXTA_INPUTPIPE_USER_BASE.
 **/
#define JXTA_PIPE_CONNECT_EVENT_STANDARD_SERVICE_BASE 100
#define JXTA_PIPE_CONNECT_EVENT_USER_BASE             1000
#define JXTA_PIPE_CONNECT_INCOMING_REQUEST (JXTA_PIPE_CONNECT_EVENT_STANDARD_SERVICE_BASE + 1)
#define JXTA_PIPE_CONNECTED (JXTA_PIPE_CONNECT_EVENT_STANDARD_SERVICE_BASE + 2)
#define JXTA_PIPE_CONNECTION_FAILED (JXTA_PIPE_CONNECT_EVENT_STANDARD_SERVICE_BASE + 3)
typedef struct _jxta_pipe_connect_event Jxta_pipe_connect_event;


  /**
   ** Create a Jxta_pipe_connect_event
   **
   ** @param ev type of the event
   ** @param pipe an optional pointer to a pipe that can be added to the event
   ** @return a new Jxta_pipe_connect_event.
   **/
JXTA_DECLARE(Jxta_pipe_connect_event *) jxta_pipe_connect_event_new(int ev, Jxta_pipe * pipe);

  /**
   ** Return the event type of the event
   **
   ** @param ev type of the event
   ** @return the type of the event.
   **/
JXTA_DECLARE(int) jxta_pipe_connect_event_get_event(Jxta_pipe_connect_event * self);

  /**
   ** Return the Jxta_pipe associated with the event (if any).
   **
   ** @param ev type of the event
   ** @param pipe returned value: a pointer to a pointer that will contain the pipe.
   ** @return JXTA_SUCCESS when successful.
   **         JXTA_FAILED when the even was not associated to any pipe.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_connect_event_get_pipe(Jxta_pipe_connect_event * self, Jxta_pipe ** pipe);



  /********************************************************************
   **    PIPE SERVICE APPLICATION API
   **
   ** The Pipe Service API is used in order to create a Pipe from its
   ** advertisement. It is also used to let actual implementation of a 
   ** particular type of pipe to register to the Pipe Service.
   ********************************************************************/

  /**
   ** Name of the various pipe types.
   **/

#define JXTA_UNICAST_PIPE          "JxtaUnicast"
#define JXTA_UNICAST_SECURE_PIPE   "JxtaUnicastSecure"
#define JXTA_PROPAGATE_PIPE        "JxtaPropagate"

  /**
   ** Try to synchronously connect to the remote end of a pipe.
   ** If peers is set to a vector containing a list of Jxta_peers*,
   ** the connection will be attempted to be established with the listed
   ** peer(s). The number of peers that the vector may contain depends on
   ** the type of type. Unicast typically can only connect to a single peer.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisement of the pipe to connect to.
   ** @param timeout timeout in micro-seconds
   ** @param peers optional vector of pointers to Jxta_peer.
   ** @param pipe returned value, a pipe connected to a remote end.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_BUSY when the pipe is already in use.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_TIMEOUT when the timeout has been reached and no connection
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_timed_connect(Jxta_pipe_service * service,
                                                          Jxta_pipe_adv * adv,
                                                          Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe);

  /**
   ** Try to asynchronously connect to the remote end of a pipe.
   ** If peers is set to a vector containing a list of Jxta_peers*,
   ** the connection will be attempted to be established with the listed
   ** peer(s). The number of peers that the vector may contain depends on
   ** the type of type. Unicast typically can only connect to a single peer.
   **
   ** The listener which is associated to the connect request is guaranteed to
   ** be called exactly once. If the timeout is reached before a connection
   ** was established, the listener is invoked with an error event.
   ** Connection events are objects (that need to be released after used),
   ** of type Jxta_pipe_connect_event.
   **
   ** After the listener is invoked, the Pipe Service removes it automatically.
   ** (the listener object is released).
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisement of the pipe to connect to.
   ** @param timeout timeout in micro-seconds
   ** @param peers optional vector of pointers to Jxta_peer.
   ** @param listener a Jxta_listener which will handle the connect events.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_BUSY when the pipe is already in use.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_connect(Jxta_pipe_service * service,
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
   ** @param adv Pipe Advertisement of the pipe to connect to.
   ** @param timeout timeout in micro-seconds
   ** @param pipe returned value, a pipe connected to a remote end.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_BUSY when the pipe is already receiving messages.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_timed_accept(Jxta_pipe_service * service,
                                                         Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_pipe ** pipe);



  /**
   ** Deny incoming connection requests.
   **
   ** The pipe is marked as not being able to receive connections.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisement of the pipe to connect to.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_deny(Jxta_pipe_service * service, Jxta_pipe_adv * adv);


  /**
   ** Accept by setting a connection listener.
   ** As long as the listener is set, any connection request on the pipe
   ** will generate a Jxta_pipe_connect_event.
   **
   ** Depending on the semantics of the type of pipe, zero, one or several
   ** connect event can be generated.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisement of the pipe to connect to.
   ** @param listener a Jxta_listener which will handle the connect events.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_BUSY when the pipe is already accepting messages.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_add_accept_listener(Jxta_pipe_service * service,
                                                                Jxta_pipe_adv * adv, Jxta_listener * listener);

  /**
   ** Remove a connection listener.
   **
   ** The listener is removed, and the pipe is set to deny incoming connection.
   **
   ** @param service pointer to the Pipe Service
   ** @param adv Pipe Advertisement of the pipe to connect to.
   ** @param listener a Jxta_listener which will handle the connect events.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_INVALID_ARGUMENT when the pipe advertisement is invalid
   **                          JXTA_NOTIMP when the pipe advertisement is for a unsupported type of pipe
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_remove_accept_listener(Jxta_pipe_service * service,
                                                                   Jxta_pipe_adv * adv, Jxta_listener * listener);


  /**
   ** This part of the Pipe Service API is dedicated to the applications that
   ** desire finer control on the behavior of the pipe, and for the implementation
   ** of the pipes themselves.
   **
   ** Regular applications do not need to use it.
   **/

  /**
   ** Return, if any, the implementation of a particular type of pipe.
   ** The type name is exactly what is returned by the pipe advertisement.
   **
   ** @param service pointer to the Pipe Service
   ** @param type_name is a null terminated string that contains the type name.
   ** @param impl returned value: a pointer to a pointer to a Jxta_pipe_service_impl.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_NOTIMP when the type_name is not a supported type of pipe.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_lookup_impl(Jxta_pipe_service * service,
                                                        const char *type_name, Jxta_pipe_service_impl ** impl);

  /**
   ** Add a new pipe implementation to the Pipe Service.
   ** The type name is exactly what is returned by the pipe advertisement.
   ** 
   ** @param service pointer to the Pipe Service
   ** @param type_name is a null terminated string that contains the type name.
   ** @param impl a pointer to a valid Jxta_pipe_service_impl.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_INVALID_ARGUMENT if impl is invalid
   **                          JXTA_BUSY when there is already an implementation for the given
   **                                    type of pipe.
   **                          JXTA_VIOLATION when adding an implementation is not authorized
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_add_impl(Jxta_pipe_service * service,
                                                     const char *name, Jxta_pipe_service_impl * impl);


  /**
   ** Remove a pipe implementation to the Pipe Service.
   ** The type name is exactly what is returned by the pipe advertisement.
   ** 
   ** @param service pointer to the Pipe Service
   ** @param type_name is a null terminated string that contains the type name.
   ** @param impl a pointer to a valid Jxta_pipe_service_impl to remove.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_INVALID_ARGUMENT if impl is invalid
   **                          JXTA_BUSY when the pipe implementation cannot be
   **                                    removed.
   **                          JXTA_VIOLATION when removing an implementation is not authorized
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_remove_impl(Jxta_pipe_service * service,
                                                        const char *name, Jxta_pipe_service_impl * impl);


  /**
   ** Get the default (peergroup) Pipe Resolver.
   ** 
   ** @param service pointer to the Pipe Service
   ** @param resolver returned value:  a pointer to a pointer to a Jxta_pipe_resolver
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_get_resolver(Jxta_pipe_service * service, Jxta_pipe_resolver ** resolver);

  /**
   ** Set the default (peergroup) Pipe Resolver.
   ** 
   ** @param service pointer to the Pipe Service
   ** @param new a pointer to the new default resolver.
   ** @param old returned value: if non NULL, old will contain a pointer
   **                            to the previous pipe resolver.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_VIOLATION when changing the default pipe is not authorized
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_service_set_resolver(Jxta_pipe_service * service,
                                                         Jxta_pipe_resolver * jnew, Jxta_pipe_resolver ** old);



  /***********************************************************************
   **      Jxta_pipe API
   ***********************************************************************/

  /**
   ** Get the output pipe of the pipe.
   **
   ** @param pipe pointer to the instance of the pipe.
   ** @param op a returned value: the output pipe
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_VIOLATION when this pipe is not capable of getting an
   **                                         output pipe.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_get_outputpipe(Jxta_pipe * pipe, Jxta_outputpipe ** op);


  /**
   ** Get the input pipe of the pipe.
   **
   ** @param pipe pointer to the instance of the pipe.
   ** @param op a returned value: the input pipe
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_VIOLATION when this pipe is not capable of getting an
   **                                         output pipe.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_get_inputpipe(Jxta_pipe * pipe, Jxta_inputpipe ** ip);

  /**
   ** Get the list of remote peers on the other end of the pipe
   ** Note that the number of remote peers varies depending on the type
   ** of pipes.
   **
   ** @param pipe pointer to the instance of the pipe.
   ** @param vector a returned value: the vector containing a list of pointers to Jxta_peer.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_get_remote_peers(Jxta_pipe * pipe, Jxta_vector ** vector);

  /**
   ** Get the pipe advertisement on the other end of the pipe
   **
   ** @param pipe pointer to the instance of the pipe.
   ** @param pipeadv a returned value: the pipeadv containing pipe advertisement.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **/
JXTA_DECLARE(Jxta_status) jxta_pipe_get_pipeadv(Jxta_pipe * pipe, Jxta_pipe_adv ** pipeadv);



  /***********************************************************************
   **      Jxta_inputpipe API
   ***********************************************************************/

  /**
   ** The following is the predefined standard event types.
   ** Specific pipe implementation can use types higher than
   ** JXTA_INPUTPIPE_USER_BASE.
   **/

#define JXTA_INPUTPIPE_STANDARD_SERVICE_BASE 100
#define JXTA_INPUTPIPE_USER_BASE             1000

#define JXTA_INPUTPIPE_INCOMING_MESSAGE_EVENT (JXTA_INPUTPIPE_STANDARD_SERVICE_BASE + 1)
#define JXTA_INPUTPIPE_CLOSED                 (JXTA_INPUTPIPE_STANDARD_SERVICE_BASE + 2)


typedef struct _jxta_inputpipe_event Jxta_inputpipe_event;

  /**
   ** Create a Jxta_inputpipe_event
   **
   ** @param ev type of the event
   ** @param object an optional pointer to a object that can be added to the event
   ** @return a new Jxta_inputpipe_event.
   **/
JXTA_DECLARE(Jxta_inputpipe_event *) jxta_inputpipe_event_new(int ev, Jxta_object * object);

  /**
   ** Return the event type of the event
   **
   ** @param ev type of the event
   ** @return the type of the event.
   **/
JXTA_DECLARE(int) jxta_inputpipe_event_get_event(Jxta_inputpipe_event * self);

  /**
   ** Return the Jxta_object associated with the event (if any).
   **
   ** @param ev type of the event
   ** @param object returned value: a pointer to a pointer that will contain the object.
   ** @return JXTA_SUCCESS when successful.
   **         JXTA_FAILED when the even was not associated to any object.
   **/
JXTA_DECLARE(Jxta_status) jxta_inputpipe_event_get_object(Jxta_inputpipe_event * self, Jxta_object ** object);

  /**
   ** Block until a message is received.
   **
   ** @param ip the input pipe.
   ** @param timeout maximum time to wait in micro-seconds
   ** @param msg returned value: the received message.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **                          JXTA_BUSY when a listener has already been added.
   **                          JXTA_TIMEOUT when the timeout has been reached and no
   **                                       message was received
   **/
JXTA_DECLARE(Jxta_status) jxta_inputpipe_timed_receive(Jxta_inputpipe * ip, Jxta_time_diff timeout, Jxta_message ** msg);

  /**
   ** Add a receive listener to a pipe. The listener is invoked for each received
   ** message, with a Jxta_inputpipe_event object.
   **
   ** @param ip the input pipe.
   ** @param listener listener to add.
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_NOMEM when the system is running out of memory.
   **/
JXTA_DECLARE(Jxta_status) jxta_inputpipe_add_listener(Jxta_inputpipe * ip, Jxta_listener * listener);

  /**
   ** Remove a receive listener to a pipe.
   **
   ** @param ip the input pipe.
   ** @param listener listener to remove
   **
   ** @return an error status. JXTA_SUCCESS when successful.
   **                          JXTA_INVALID_ARGUMENT when the listener is not valid.
   **/
JXTA_DECLARE(Jxta_status) jxta_inputpipe_remove_listener(Jxta_inputpipe * ip, Jxta_listener * listener);

  /***********************************************************************
   **      Jxta_outpipe API
   ***********************************************************************/

  /**
   ** The following is the predefined standard event types.
   ** Specific pipe implementation can use types higher than
   ** JXTA_OUTPIPE_USER_BASE.
   **/

#define JXTA_OUTPUTPIPE_STANDARD_SERVICE_BASE 100
#define JXTA_OUTPUTPIPE_USER_BASE             1000

#define JXTA_OUTPUTPIPE_MESSAGE_SENT_EVENT (JXTA_OUTPUTPIPE_STANDARD_SERVICE_BASE + 1)
#define JXTA_OUTPUTPIPE_MESSAGE_FAILED_EVENT (JXTA_OUTPUTPIPE_STANDARD_SERVICE_BASE + 2)

typedef struct _jxta_outputpipe_event Jxta_outputpipe_event;

/**
 * Create a Jxta_outputpipe_event
 *
 * @param ev type of the event
 * @param object an optional pointer to a object that can be added to the event
 * @return a new Jxta_outputpipe_event.
 */
JXTA_DECLARE(Jxta_outputpipe_event *) jxta_outputpipe_event_new(int ev, Jxta_object * object);

/**
 * Return the event type of the event
 *
 * @param ev type of the event
 * @return the type of the event.
 */
JXTA_DECLARE(int) jxta_outputpipe_event_get_event(Jxta_outputpipe_event * self);

/**
 * Return the Jxta_object associated with the event (if any).
 *
 * @param ev type of the event
 * @param object returned value: a pointer to a pointer that will contain the object.
 * @return JXTA_SUCCESS when successful.
 *         JXTA_FAILED when the even was not associated to any object.
 */
JXTA_DECLARE(Jxta_status) jxta_outputpipe_event_get_object(Jxta_outputpipe_event * self, Jxta_object ** object);


/**
 * Send a message onto a Jxta_outputpipe.
 *
 * @param op the output pipe.
 * @param msg the message to send
 *
 * @return an error status. JXTA_SUCCESS when successful.
 *                          JXTA_NOMEM when the system is running out of memory.
 *                          JXTA_FAILED when the send failed.
 */
JXTA_DECLARE(Jxta_status) jxta_outputpipe_send(Jxta_outputpipe * op, Jxta_message * msg);


/**
 * Add a status listener to the output pipe.
 * Some type of Pipe Service can asynchronously send event concerning
 * the state of a sent message. Setting a status listener allow the application
 * to receive them. The type of events received by the listener is Jxta_outputpipe_event
 *
 * @param op the output pipe.
 * @param listener listener to add.
 *
 * @return an error status. JXTA_SUCCESS when successful.
 *                          JXTA_NOMEM when the system is running out of memory.
 *                          JXTA_FAILED when the pipe service is not capable of throwing
 *                          Jxta_outpipe_event.
 */
JXTA_DECLARE(Jxta_status) jxta_outputpipe_add_listener(Jxta_outputpipe * op, Jxta_listener * listener);

/**
 * Remove a status listener to a pipe.
 *
 * @param op the output pipe.
 * @param listener listener to remove
 *
 * @return an error status. JXTA_SUCCESS when successful.
 *                          JXTA_INVALID_ARGUMENT when the listener is not valid.
 */
JXTA_DECLARE(Jxta_status) jxta_outputpipe_remove_listener(Jxta_outputpipe * op, Jxta_listener * listener);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /*__JXTA_PIPE_SERVICE_H__*/

/* vi: set ts=4 sw=4 tw=130 et: */
