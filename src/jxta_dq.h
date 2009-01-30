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

#ifndef __Jxta_DiscoveryQuery_H__
#define __Jxta_DiscoveryQuery_H__

#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_qos.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct jxta_DiscoveryQuery Jxta_DiscoveryQuery;
typedef struct jxta_DiscoveryQuery Jxta_discovery_query;

typedef enum extended_query_states {
    DEQ_SUPRESS=0,            /* used to supress attribute for backward compatiblity */
    DEQ_INIT,               /* query initiated */
    DEQ_FWD_SRDI,           /* Query to SRDI */
    DEQ_FWD_REPLICA_FWD,    /* Query to Replica (Walk if needed) */
    DEQ_FWD_REPLICA_STOP,   /* Query to Replica (Don't walk) */
    DEQ_FWD_WALK,           /* Query being walked (Rdv Diffusion defines scope) */
    DEQ_REV_REPLICATING,    /* Query to Replicating */
    DEQ_REV_PUBLISHER,       /* Query to Publisher */
    DEQ_REV_REPLICATING_WALK /* Query to Replicating (result of walk) */
 } Jxta_discovery_ext_query_state;

/**
 * Allocate a new discovery query advertisement.
 *
 * @param void takes no arguments.
 *
 * @return Pointer to discovery query advertisement.
 */
JXTA_DECLARE(Jxta_DiscoveryQuery *) jxta_discovery_query_new(void);

JXTA_DECLARE(Jxta_DiscoveryQuery *) jxta_discovery_query_new_1(short type, const char *attr, const char *value, int threshold,
                                                               JString * peeradv);

JXTA_DECLARE(Jxta_DiscoveryQuery *) jxta_discovery_query_new_2(const char *query, int threshold, JString * peeradv);

/**
 * Constructs a representation of a discovery query advertisement in
 * xml format.
 *
 * @param Jxta_DiscoveryQuery * pointer to discovery query advertisement
 * @param JString ** address of pointer to JString that 
 *        accumulates xml representation of discovery query advertisement.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_xml( /*Jxta_advertisement * */ Jxta_DiscoveryQuery * adv, JString **);


/**
 * Wrapper for jxta_advertisement_parse_charbuffer,
 * when call completes, the xml representation of a 
 * buffer is loaded into a discovery query advertisement structure.
 *
 * @param Jxta_DiscoveryQuery * pointer to discovery query
 *        advertisement to contain the data in the xml.
 * @param const char * buffer containing characters 
 *        with xml syntax.
 * @param int len length of character buffer.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status)
    jxta_discovery_query_parse_charbuffer(Jxta_DiscoveryQuery *, const char *, int len);


/**
 * Wrapper for jxta_advertisement_parse_file,
 * when call completes, the xml representation of a 
 * data is loaded into a discovery query advertisement structure.
 *
 * @param Jxta_DiscoveryQuery * discovery response advertisement
 *        to contain the data in the xml.
 * @param FILE * an input stream.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_parse_file(Jxta_DiscoveryQuery *, FILE * stream);

/**
 * Gets the Type of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 *
 * @return short containing the discovery type, DISC_PEER = 0, DISC_GROUP = 1 or DISC_ADV = 2.
 */
JXTA_DECLARE(short) jxta_discovery_query_get_type(Jxta_DiscoveryQuery *);


/**
 * Sets the Type of the discovery response advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param short containing the Type. DISC_PEER = 0, DISC_GROUP = 1 or DISC_ADV = 2.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_type(Jxta_DiscoveryQuery *, short);


/**
 * Gets the Threshold of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 *
 * @return int containing the Threshold.
 */
JXTA_DECLARE(int) jxta_discovery_query_get_threshold(Jxta_DiscoveryQuery *);


JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_threshold(Jxta_DiscoveryQuery *, int);


/**
 * Gets the peer advertisement of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param JString ** a pointer to JString pointer to receive the peer advertisement as a JString*.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status)
    jxta_discovery_query_get_peeradv(Jxta_DiscoveryQuery * ad, JString ** padv);


/**
 * Sets the peer advertisement of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param JString * Pointer to the JString containing the peer advertisement.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_peeradv(Jxta_DiscoveryQuery *, JString *);


/**
 * Gets the Attr of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param JString ** a pointer to receive the JString pointer containing the Attr.
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_attr(Jxta_DiscoveryQuery *, JString **);


/**
 * Sets the Attr of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param JString * containing the Attr.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_attr(Jxta_DiscoveryQuery *, JString *);


/**
 * Gets the Value of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param JString ** a pointer to receive the JString pointer containing the Value.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_value(Jxta_DiscoveryQuery *, JString **);


/**
 * Sets the Value of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param JString * containing the Value.
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_value(Jxta_DiscoveryQuery *, JString *);

JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_extended_query(Jxta_DiscoveryQuery * ad, JString ** value);

JXTA_DECLARE(Jxta_status) jxta_discovery_query_attach_qos(Jxta_discovery_query * me, const Jxta_qos * qos);
JXTA_DECLARE(const Jxta_qos *) jxta_discovery_query_qos(Jxta_discovery_query * me);

JXTA_DECLARE(Jxta_discovery_ext_query_state) jxta_discovery_query_ext_get_state(Jxta_discovery_query * me);
JXTA_DECLARE(Jxta_status) jxta_discovery_query_ext_set_state(Jxta_discovery_query *me, Jxta_discovery_ext_query_state state);
JXTA_DECLARE(Jxta_status) jxta_discovery_query_ext_set_peerid(Jxta_discovery_query *me, JString *peerid);
JXTA_DECLARE(Jxta_status) jxta_discovery_query_ext_get_peerid(Jxta_discovery_query *me, JString **peerid);

JXTA_DECLARE(void) jxta_discovery_query_ext_print_state(Jxta_discovery_query *me, JString *print);
#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __Jxta_DiscoveryQuery_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
