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
 * $Id: jxta_qos.h,v 1.4 2006/08/19 01:16:43 slowhog Exp $
 */

#ifndef JXTA_QOS_H
#define JXTA_QOS_H

#include "jxta_apr.h"
#include "jxta_types.h"

#ifdef cplusplus
extern "C" {
#if 0 
};
#endif
#endif

typedef struct jxta_qos Jxta_qos;

/**
 * Create a QoS structure
 * @param me The pointer to receive the created structure
 * @param p The apr_pool_t structure which to be used with the structure, this pool must out-live the created QoS structure.
 * 
 * @return JXTA_SUCCESS if the operation succeed. 
 */
JXTA_DECLARE(Jxta_status) jxta_qos_create(Jxta_qos ** me, apr_pool_t *p);

/**
 * Create a QoS structure, and initiliaze the structure by parsing from the XML.
 * @param me The pointer to receive the created structure
 * @param xml The XML paragraph of QoS_Setting or QoS_Support
 * @param p The apr_pool_t structure which to be used with the structure
 * 
 * @return JXTA_SUCCESS if the operation succeed. JXTA_INVALID_ARGUMENT if the XML is not one of the type, JXTA_FAILED if other
 * error occurs.
 */
JXTA_DECLARE(Jxta_status) jxta_qos_create_1(Jxta_qos **me, const char *xml, apr_pool_t *p);

/**
 * Destroy a QoS structure
 */
JXTA_DECLARE(Jxta_status) jxta_qos_destroy(Jxta_qos * me);

/**
 * Clone a QoS structure
 * @param me The pointer to receive the created structure
 * @param src The Jxta_qos to be cloned
 * @param p The apr_pool_t structure which to be used with the structure, this pool must out-live the created QoS structure. If
 * pass in NULL, the same pool used for src will be used.
 * 
 * @return JXTA_SUCCESS if the operation succeed. 
 */
JXTA_DECLARE(Jxta_status) jxta_qos_clone(Jxta_qos ** me, const Jxta_qos * src, apr_pool_t *p);

/**
 * Set the immutable flag on the QoS structure. Once set, no further changes can be made.
 * @param me pointer to the Jxta_qos structure
 */
JXTA_DECLARE(Jxta_status) jxta_qos_set_immutable(Jxta_qos * me);

/**
 * Test if the immutable flag is set.
 * @param me pointer to the Jxta_qos structure
 * @return JXTA_TRUE if the Jxta_oqs structure is immutable. JXTA_FALSE otherwise.
 */
JXTA_DECLARE(Jxta_boolean) jxta_qos_is_immutable(Jxta_qos * me);

/**
 * Set a value of a QoS capability for the QoS structure
 * @param me pointer to the Jxta_qos structure
 * @param name The QoS capability to be set
 * @param value The QoS value to be set
 * @return JXTA_SUCCESS if the value was set. JXTA_FAIL if the Jxta_qos is immutable
 * @note set function always overwrite existing setting. If want to keep the existing value, use add function instead.
 */
JXTA_DECLARE(Jxta_status) jxta_qos_set(Jxta_qos * me, const char * name, const char * value);
JXTA_DECLARE(Jxta_status) jxta_qos_set_long(Jxta_qos * me, const char * name, long value);

/**
 * Set a value of a QoS capability for the QoS structure
 * @param me pointer to the Jxta_qos structure
 * @param name The QoS capability to be set
 * @param value The QoS value to be set
 * @return JXTA_SUCCESS if the value was set. JXTA_BUSY if the value has been set. JXTA_FAIL if the Jxta_qos is immutable
 * @note add function will not overwrite existing setting. If want to overwrite existing value, use set function instead.
 */
JXTA_DECLARE(Jxta_status) jxta_qos_add(Jxta_qos * me, const char * name, const char * value);
JXTA_DECLARE(Jxta_status) jxta_qos_add_long(Jxta_qos * me, const char * name, long value);

/**
 * Get a value of a QoS capability for the QoS structure
 * @param me pointer to the Jxta_qos structure
 * @param name The QoS capability to retrieve
 * @param value A pointer to receive the QoS value 
 * @return JXTA_SUCCESS with the value. JXTA_ITEM_NOTFOUND if the value is not set.
 */
JXTA_DECLARE(Jxta_status) jxta_qos_get(const Jxta_qos * me, const char * name, const char ** value);
JXTA_DECLARE(Jxta_status) jxta_qos_get_long(const Jxta_qos * me, const char * name, long * value);

/**
 * Convert the current setting into a jxta:QoS_Setting XML paragraph. A sample is like:
 *  <QoS_Setting>
 *    <QoS>lifespan
 *      <Value>1152583254</Value>
 *    </QoS>
 *    <QoS>priority
 *      <Value>128</Value>
 *    </Qos>
 *  </QoS_Setting>
 * The string will be allocated from the pool assigned when create the QoS object
 * @param me pointer to the Jxta_qos structure
 * @param result A pointer to receive the result NULL-terminate string.
 * @param p The apr_pool_t structure used in the operation, from which the string will be allocated. NULL is allowed, for which
 * case the same apr_pool_t used to create the Jxta_qos strcture will be used.
 * @return JXTA_SUCCESS with the value. JXTA_FAILED if error occurs.
 */
JXTA_DECLARE(Jxta_status) jxta_qos_setting_to_xml(const Jxta_qos * me, char ** result, apr_pool_t *p);

/**
 * Convert the current capability into a jxta:QoS_Support XML paragraph. A sample is like:
 *  <QoS_Support>
 *    <QoS>lifespan</QoS>
 *    <QoS>priority</QoS>
 *    <QoS>TTL</QoS>
 *  </QoS_Support>
 * The string will be allocated from the pool assigned when create the QoS object
 * @param me pointer to the Jxta_qos structure
 * @param result A pointer to receive the result NULL-terminate string.
 * @param p The apr_pool_t structure used in the operation, from which the string will be allocated. NULL is allowed, for which
 * case the same apr_pool_t used to create the Jxta_qos strcture will be used.
 * @return JXTA_SUCCESS with the value. JXTA_FAILED if error occurs.
 */
/* TODO: Does not make sense without a registration mechanism in place.
JXTA_DECLARE(Jxta_status) jxta_qos_support_to_xml(Jxta_qos * me, char ** result, apr_pool_t *p);
*/

#ifdef cplusplus
#if 0 
{
#endif
}
#endif

#endif /* JXTA_QOS_H */

/* vim: set tw=130 ts=4 sw=4 et: */

