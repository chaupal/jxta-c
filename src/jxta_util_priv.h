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
 * $Id: jxta_util_priv.h,v 1.8 2006/09/27 23:09:15 exocetrick Exp $
 */

#include "jxta_vector.h"
#include "jxta_apr.h"
#include "jxta_cred.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0
};
#endif

/**
 * Utility function.
 * From a vector of JString returns a vector of Jxta_id.
 * The user should still release the input vector
 *
 * @param peers The input vector
 * @param returns the output vector
 */
Jxta_vector *getPeerids(Jxta_vector * peers);

/**
 * Load a list of seeds from the provided seeding URI.
 *
 * @param seeding_uri The seeding URI. 
 *
 * @return A vector of Jxta_endpoint_address for seeds, NULL if error occurs.
 */
Jxta_vector *load_seeds_from_uri(const apr_uri_t *, const seeding_uri);

/**
 * Load a list of seeds from the provided seeding URI.
 *
 * @param seeding_uri The seeding URI as a NULL-terminated string.
 * @param pool The memory pool used during the process. NULL will cause a temporary memory pool to be created.
 *
 * @return A vector of Jxta_endpoint_address for seeds, NULL if error occurs.
 */
Jxta_vector *load_seeds(const char *seeding_uri, apr_pool_t * pool);

char *get_service_key(const char *svc_name, const char *svc_param);

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
 */
Jxta_status qos_setting_to_xml(apr_hash_t * setting, char **result, apr_pool_t * p);

/**
 * Parse a jxta:QoS_Setting XML paragraph into a hash table of QoS setting
 */
Jxta_status xml_to_qos_setting(const char *xml, apr_hash_t ** dest, apr_pool_t * p);

/**
 * Convert the current capability into a jxta:QoS_Support XML paragraph. A sample is like:
 *  <QoS_Support>
 *    <QoS>lifespan</QoS>
 *    <QoS>priority</QoS>
 *    <QoS>TTL</QoS>
 *  </QoS_Support>
 * The string will be allocated from the pool assigned when create the QoS object
 */
Jxta_status qos_support_to_xml(const char **capability_list, char **result, apr_pool_t * p);

/**
 * Parse a jxta:QoS_Support XML paragraph into a list of QoS capabilities
 */
Jxta_status xml_to_qos_support(const char *xml, char ***result, apr_pool_t * p);

Jxta_status ep_tcp_socket_listen(apr_socket_t ** me, const char *addr, apr_port_t port, apr_int32_t backlog, apr_pool_t * p);

/* ReadFunc with STREAM to be apr_brigade */
Jxta_status JXTA_STDCALL brigade_read(void *b, char *buf, apr_size_t len);

/**
 * Query all advertisements within the local peer's cache
 * @param query XPath type query
 * @param scope Array of credentials for subgroups.
 * @param threshold Maximum number of groups returning results
 * @param results Location to store the results vector.
 *
 * @return Jxta_status
 * @see Jxta_status
 *
**/
Jxta_status query_all_advs(const char *query, Jxta_credential * scope[], int threshold, Jxta_vector ** advertisements);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
