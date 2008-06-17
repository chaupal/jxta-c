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
 * JXTA-C Tutorials: shared.c
 * Created by Brent Gulanowski on 10/31/04.
 */


#include <assert.h>

#include "jxta_tutorial_shared.h"


#define TEN_SECONDS_IN_uS 10 * 1000 * 1000

/* simple print utility */
void jstring_print_and_release(FILE * stream, JString * str)
{
    if (str) {
        fprintf(stream, "%s\n", jstring_get_string(str));
        JXTA_OBJECT_RELEASE(str);
    }
}


JString *group_description(Jxta_PG * group)
{

    /* initialize all jxta objects to NULL in case there's an error */
    JString *groupName = NULL;
    Jxta_PGID *groupId = NULL;
    JString *idString = NULL;
    JString *descriptionString = NULL;
    Jxta_status status;

    assert(group);

    jxta_PG_get_groupname(group, &groupName);
    jxta_PG_get_GID(group, &groupId);

    status = jxta_id_to_jstring(groupId, &idString);
    assert(JXTA_SUCCESS == status);

    descriptionString = jstring_new_0();
    assert(descriptionString);
    jstring_append_2(descriptionString, "Group Name: ");
    jstring_append_1(descriptionString, groupName);
    jstring_append_2(descriptionString, "\nGroup ID: ");
    jstring_append_1(descriptionString, idString);

    /* remember to release what you're done with, but not what you're returning */
    JXTA_OBJECT_RELEASE(groupName);
    JXTA_OBJECT_RELEASE(groupId);
    JXTA_OBJECT_RELEASE(idString);

    return descriptionString;
}


void group_print_description(FILE * stream, Jxta_PG * group)
{
    jstring_print_and_release(stream, group_description(group));
}

JString *group_ad_string(Jxta_PG * group)
{

    Jxta_PGA *groupAd = NULL;
    JString *adString = NULL;
    Jxta_status status;

    assert(group);

    jxta_PG_get_PGA(group, &groupAd);
    status = jxta_PGA_get_xml(groupAd, &adString);
    assert(JXTA_SUCCESS == status);

    JXTA_OBJECT_RELEASE(groupAd);

    return adString;
}

void group_print_ad(FILE * stream, Jxta_PG * group)
{
    jstring_print_and_release(stream, group_ad_string(group));
}

JString *group_my_description(Jxta_PG * group)
{

    JString *descriptionString = NULL;
    JString *idString = NULL;
    JString *peerName = NULL;
    Jxta_id *peerId = NULL;

    assert(group);

    jxta_PG_get_peername(group, &peerName);
    assert(peerName);
    jxta_PG_get_PID(group, &peerId);
    jxta_id_to_jstring(peerId, &idString);

    descriptionString = jstring_new_0();
    assert(descriptionString);
    jstring_append_2(descriptionString, "Peer Name: ");
    jstring_append_1(descriptionString, peerName);
    jstring_append_2(descriptionString, "\nPeer ID: ");
    jstring_append_1(descriptionString, idString);

    JXTA_OBJECT_RELEASE(peerName);
    JXTA_OBJECT_RELEASE(peerId);
    JXTA_OBJECT_RELEASE(idString);

    return descriptionString;
}

void group_print_my_description(FILE * stream, Jxta_PG * group)
{
    jstring_print_and_release(stream, group_my_description(group));
}

Jxta_peer *group_get_rendezvous_peer(Jxta_PG * group)
{

    Jxta_peer *rdvPeer = NULL;
    Jxta_rdv_service *rdvSvc = NULL;
    Jxta_vector *peers = NULL;
    Jxta_peer *peer = NULL;
    unsigned size;
    unsigned i;
    Jxta_status status;

    assert(group);

    jxta_PG_get_rendezvous_service(group, &rdvSvc);
    assert(rdvSvc);

    status = jxta_rdv_service_get_peers(rdvSvc, &peers);
    assert(JXTA_SUCCESS == status);

    /* find a rendezvous -- always returns the first one found or NULL */
    size = jxta_vector_size(peers);
    for (i = 0; i < size; i++) {
        jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
        if (jxta_peer_get_expires(peer) > jpr_time_now()) {
            rdvPeer = peer;
            break;
        }
        if (peer)
            JXTA_OBJECT_RELEASE(peer);
    }

    if (rdvSvc)
        JXTA_OBJECT_RELEASE(rdvSvc);
    if (peers)
        JXTA_OBJECT_RELEASE(peers);

    return rdvPeer;
}

Jxta_boolean group_wait_for_network(Jxta_PG * group)
{

    Jxta_boolean success = TRUE;
    Jxta_rdv_service *rdv = NULL;
    Jxta_listener *networkListener;
    Jxta_rdv_event *event = NULL;
    Jxta_status status;

    /*
     * the listener captures rdv events in its own background thread;
     * we don't provide a listener function (or argument) because we want
     * to receive the next event synchronously, here in this thread
     */
    networkListener = jxta_listener_new(NULL, NULL, 1, 1);
    assert(networkListener);
    jxta_listener_start(networkListener);

    jxta_PG_get_rendezvous_service(group, &rdv);
    assert(rdv);

    jxta_rdv_service_add_event_listener(rdv, "NULL", NULL, networkListener);
    status = jxta_listener_wait_for_event(networkListener, TEN_SECONDS_IN_uS, JXTA_OBJECT_PPTR(&event));
    assert(JXTA_SUCCESS == status);

    /* if we don't connect after the timeout (ten seconds) then give up */
    if (JXTA_RDV_CONNECTED != event->event) {
        success = FALSE;
    }

    JXTA_OBJECT_RELEASE(event);
    JXTA_OBJECT_RELEASE(networkListener);
    JXTA_OBJECT_RELEASE(rdv);

    return success;
}


JString *peer_description(Jxta_peer * peer)
{

    JString *descriptionString = NULL;
    Jxta_PA *peerAd = NULL;
    Jxta_status status;

    assert(peer);

    status = jxta_peer_get_adv(peer, &peerAd);
    assert(JXTA_SUCCESS == status);
    descriptionString = peer_ad_peer_description(peerAd);

    JXTA_OBJECT_RELEASE(peerAd);

    return descriptionString;
}

void peer_print_description(FILE * stream, Jxta_peer * peer)
{
    jstring_print_and_release(stream, peer_description(peer));
}


JString *peer_ad_peer_description(Jxta_PA * peerAd)
{

    JString *descriptionString = NULL;
    Jxta_id *peerId = NULL;
    JString *idString = NULL;
    JString *peerName = NULL;

    assert(peerAd);

    peerId = jxta_PA_get_PID(peerAd);
    jxta_id_to_jstring(peerId, &idString);
    peerName = jxta_PA_get_Name(peerAd);

    descriptionString = jstring_new_0();
    assert(descriptionString);
    jstring_append_2(descriptionString, "Peer Advertisement\nPeer Name: ");
    jstring_append_1(descriptionString, peerName);
    jstring_append_2(descriptionString, "\nPeer ID: ");
    jstring_append_1(descriptionString, idString);

    JXTA_OBJECT_RELEASE(peerName);
    JXTA_OBJECT_RELEASE(peerId);
    JXTA_OBJECT_RELEASE(idString);

    return descriptionString;
}

void peer_ad_print_peer_description(FILE * stream, Jxta_PA * peerAd)
{
    jstring_print_and_release(stream, peer_ad_peer_description(peerAd));
}

JString *group_advertisement_description(Jxta_PGA * groupAd)
{

    /* initialize all jxta objects to NULL in case there's an error */

    /* note "description" as used in function name is generic; there is also description field
     * of advertisement, specified at time of its creation, here called "groupDesc" */
    JString *groupName = NULL;
    JString *groupDesc = NULL;
    Jxta_PGID *groupId = NULL;
    JString *idString = NULL;
    JString *descriptionString = NULL;

    assert(groupAd);

    groupName = jxta_PGA_get_Name(groupAd);
    groupId = jxta_PGA_get_GID(groupAd);
    groupDesc = jxta_PGA_get_Desc(groupAd);

    jxta_id_to_jstring(groupId, &idString);

    descriptionString = jstring_new_0();
    assert(descriptionString);
    jstring_append_2(descriptionString, "Group Name: ");
    jstring_append_1(descriptionString, groupName);
    jstring_append_2(descriptionString, "\nGroup ID: ");
    jstring_append_1(descriptionString, idString);
    jstring_append_2(descriptionString, "\nDescription: ");
    jstring_append_1(descriptionString, groupDesc);

    /* remember to release what you're done with, but not what you're returning */
    JXTA_OBJECT_RELEASE(groupName);
    JXTA_OBJECT_RELEASE(groupId);
    JXTA_OBJECT_RELEASE(idString);
    JXTA_OBJECT_RELEASE(groupDesc);

    return descriptionString;
}

void group_advertisement_print_description(FILE * stream, Jxta_PGA * groupAd)
{
    jstring_print_and_release(stream, group_advertisement_description(groupAd));
}

Jxta_boolean group_advertisement_is_valid(Jxta_PGA * groupAd)
{

    Jxta_id *groupId = NULL;
    Jxta_id *MSID = NULL;
    JString *idString = NULL;

    assert(groupAd);

    groupId = jxta_PGA_get_GID(groupAd);
    jxta_id_to_jstring(groupId, &idString);
    if (0 == strncmp(jstring_get_string(idString), "urn:jxta:jxta-Null", strlen("urn:jxta:jxta-Null"))) {
        JXTA_OBJECT_RELEASE(idString);
        JXTA_OBJECT_RELEASE(groupId);
        return FALSE;
    }

    MSID = jxta_PGA_get_MSID(groupAd);
    jxta_id_to_jstring(MSID, &idString);
    if (0 == strncmp(jstring_get_string(idString), "urn:jxta:jxta-Null", strlen("urn:jxta:jxta-Null"))) {
        JXTA_OBJECT_RELEASE(idString);
        JXTA_OBJECT_RELEASE(groupId);
        return FALSE;
    }

    return TRUE;
}


JString *pipe_advertisement_description(Jxta_pipe_adv * pipeAd)
{

    JString *descriptionString = NULL;
    char *adId = NULL;
    char *adName = NULL;

    assert(pipeAd);

    adId = jxta_pipe_adv_get_Id(pipeAd);
    adName = jxta_pipe_adv_get_Name(pipeAd);

    descriptionString = jstring_new_0();
    assert(descriptionString);
    jstring_append_2(descriptionString, "Pipe Advertisement\nPipe Name: ");
    jstring_append_2(descriptionString, adName);
    jstring_append_2(descriptionString, "\nPipe ID: ");
    jstring_append_2(descriptionString, adId);

    return descriptionString;
}

void pipe_advertisement_print_description(FILE * stream, Jxta_pipe_adv * pipeAd)
{
    jstring_print_and_release(stream, pipe_advertisement_description(pipeAd));
}


void vector_remove_object(Jxta_vector * self, Jxta_object * object)
{

    int size;
    int i;

    size = jxta_vector_size(self);
    for (i = 0; i < size; i++) {

        Jxta_object *garbage = NULL;

        jxta_vector_get_object_at(self, &garbage, i);
        JXTA_OBJECT_RELEASE(garbage);
        if (object == garbage) {
            jxta_vector_remove_object_at(self, &garbage, i);
            JXTA_OBJECT_RELEASE(garbage);
            return;
        }
    }
}
