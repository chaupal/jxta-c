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


#ifndef __RENDEZVOUSWALKMESSAGE_H__
#define __RENDEZVOUSWALKMESSAGE_H__

#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

enum Walk_directions {
    WALK_UP = 1,
    WALK_DOWN = 2,
    WALK_BOTH = 3
};

typedef enum Walk_directions Walk_direction;

typedef struct _LimitedRangeRdvMessage LimitedRangeRdvMessage;

JXTA_DECLARE(LimitedRangeRdvMessage *) LimitedRangeRdvMessage_new(void);
JXTA_DECLARE(void) LimitedRangeRdvMessage_set_handlers(LimitedRangeRdvMessage *, XML_Parser, void *);
JXTA_DECLARE(Jxta_status) LimitedRangeRdvMessage_get_xml(LimitedRangeRdvMessage *, JString ** xml);
JXTA_DECLARE(Jxta_status) LimitedRangeRdvMessage_parse_charbuffer(LimitedRangeRdvMessage *, const char *, int len);
JXTA_DECLARE(Jxta_status) LimitedRangeRdvMessage_parse_file(LimitedRangeRdvMessage *, FILE * stream);

JXTA_DECLARE(int) LimitedRangeRdvMessage_get_TTL(LimitedRangeRdvMessage * ad);
JXTA_DECLARE(void) LimitedRangeRdvMessage_set_TTL(LimitedRangeRdvMessage * ad, int ttl);

JXTA_DECLARE(Walk_direction) LimitedRangeRdvMessage_get_direction(LimitedRangeRdvMessage * ad);
JXTA_DECLARE(void) LimitedRangeRdvMessage_set_direction(LimitedRangeRdvMessage * ad, Walk_direction dir);

JXTA_DECLARE(const char *) LimitedRangeRdvMessage_get_SrcPeerID(LimitedRangeRdvMessage * ad);
JXTA_DECLARE(void) LimitedRangeRdvMessage_set_SrcPeerID(LimitedRangeRdvMessage * ad, const char *src_peer_id);

JXTA_DECLARE(const char *) LimitedRangeRdvMessage_get_SrcSvcName(LimitedRangeRdvMessage * ad);
JXTA_DECLARE(void) LimitedRangeRdvMessage_set_SrcSvcName(LimitedRangeRdvMessage * ad, const char *svc_name);

JXTA_DECLARE(const char *) LimitedRangeRdvMessage_get_SrcSvcParams(LimitedRangeRdvMessage * ad);
JXTA_DECLARE(void) LimitedRangeRdvMessage_set_SrcSvcParams(LimitedRangeRdvMessage * ad, const char *svc_params);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __RENDEZVOUSWALKMESSAGE_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
