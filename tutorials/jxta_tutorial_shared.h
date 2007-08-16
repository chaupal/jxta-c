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
 * $Id$
 */

/*
 * JXTA-C Tutorials: shared.h
 * Created by Brent Gulanowski on 10/31/04.
 */


#ifndef JXTA_TUTORIAL_SHARED_H
#define JXTA_TUTORIAL_SHARED_H


#include <jxta.h>
#include <jxta_types.h>
#include <jxta_peergroup.h>
#include <jxta_peer.h>

#include <stdio.h>


#include "jxta_tutorial.h"


/* Return or print a simple formatted description of group with name and ID */
extern JString *group_description(Jxta_PG * group);
extern void group_print_description(FILE * stream, Jxta_PG * group);

/* Produce a string version of the advertisement of the provided group. */
extern JString *group_ad_string(Jxta_PG * group);
extern void group_print_ad(FILE * stream, Jxta_PG * group);

/* Return or print a simple formatted description of the current peer with name and ID */
extern JString *group_my_description(Jxta_PG * group);
extern void group_print_my_description(FILE *, Jxta_PG * group);

/* Find the rendezvous peer for a group */
extern Jxta_peer *group_get_rendezvous_peer(Jxta_PG * group);

/* use a listener to wait for a connect event with a timeout of 10 seconds */
extern Jxta_boolean group_wait_for_network(Jxta_PG * group);

/* Return or print a simple formatted description of provided peer with name and ID  */
extern JString *peer_description(Jxta_peer * peer);
extern void peer_print_description(FILE * stream, Jxta_peer * peer);

/* Return or print a simple formatted description of Peer with name and ID */
JString *peer_ad_peer_description(Jxta_PA * peerAd);
void peer_ad_print_peer_description(FILE * stream, Jxta_PA * peerAd);

/* Return or print a simple formatted description of Group with name, ID and Description attributes */
JString *group_advertisement_description(Jxta_PGA * groupAd);
void group_advertisement_print_description(FILE * stream, Jxta_PGA * groupAd);

/* return FALSE if GID or MSID are invalid (or group is NULL), else return TRUE */
Jxta_boolean group_advertisement_is_valid(Jxta_PGA * groupAd);

/* Return or print a simple formatted description of Pipe with name and ID */
JString *pipe_advertisement_description(Jxta_pipe_adv * pipeAd);
void pipe_advertisement_print_description(FILE * stream, Jxta_pipe_adv * pipeAd);

/* remove the given object from the given vector, if it exists */
void vector_remove_object(Jxta_vector * self, Jxta_object * object);

#endif /* JXTA_TUTORIAL_SHARED_H */
