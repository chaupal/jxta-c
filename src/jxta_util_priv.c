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
 * $Id: jxta_util_priv.c,v 1.6 2006/03/11 02:35:02 slowhog Exp $
 */

static const char *__log_cat = "UTIL";

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "jxta_util_priv.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_log.h"
#include "jxta_id.h"

Jxta_vector *getPeerids(Jxta_vector * peers)
{
    Jxta_vector *peerIds = NULL;
    unsigned int i = 0;
    JString *jPeerId;
    Jxta_hashtable *peersHash;
    if (NULL == peers)
        return NULL;
    if (jxta_vector_size(peers) > 0) {
        peerIds = jxta_vector_new(jxta_vector_size(peers));
    } else {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "----- check %d peerids: \n", jxta_vector_size(peers));
    peersHash = jxta_hashtable_new(jxta_vector_size(peers));
    for (i = 0; i < jxta_vector_size(peers); i++) {
        const char *pid;
        JString *temp;

        jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&jPeerId), i);
        pid = jstring_get_string(jPeerId);
        if (jxta_hashtable_get(peersHash, pid, strlen(pid), JXTA_OBJECT_PPTR(&temp)) != JXTA_SUCCESS) {
            Jxta_id *jid;
            jxta_hashtable_put(peersHash, pid, strlen(pid), (Jxta_object *) jPeerId);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "----- add peerid to result set: %s\n", pid);
            jxta_id_from_jstring(&jid, jPeerId);
            jxta_vector_add_object_last(peerIds, (Jxta_object *) jid);
            JXTA_OBJECT_RELEASE(jid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "----- peerid is in result set already: %s\n", pid);
            if (temp != NULL) {
                JXTA_OBJECT_RELEASE(temp);
            }
        }
        JXTA_OBJECT_RELEASE(jPeerId);
    }
    JXTA_OBJECT_RELEASE(peersHash);
    return peerIds;
}

/* vim: set ts=4 sw=4 et tw=130: */
