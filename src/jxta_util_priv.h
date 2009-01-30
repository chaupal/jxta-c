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
 * $Id$
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

#ifdef JXTA_PERFORMANCE_TRACKING_ENABLED
/**
* Create a performance tracking log entry
* A start time will be saved when the tracking starts
* When the tracking ends a log message will be output
* with the elapsed time from start to end.
*/
    #define PERF_ENTRY \
        Jxta_time perf_start_time; \
        Jxta_time perf_end_time; \
        const char *perf_desc=NULL; \
        const char *perf_func=NULL; \
        Jxta_time_diff perf_diff_time;

/**
* Start the performance entry
* a - description
* b - function name
*/
    #define PERF_START(a, b) \
        perf_start_time = jpr_time_now(); \
        perf_desc = a; \
        perf_func = b;

/**
* end the performance entry
* a - number of items
* b - log level of the log entry
*/
    #define PERF_END(a, b) \
        perf_end_time = jpr_time_now(); \
        perf_diff_time = perf_end_time - perf_start_time; \
        assert(NULL != perf_desc && NULL != perf_func); \
        if (perf_diff_time > 0) { \
            jxta_log_append(__log_cat, b, "desc:%s func:%s i:%d t:" JPR_DIFF_TIME_FMT "\n" \
                        , perf_desc, perf_func, a, perf_diff_time); \
        }

#endif /* JXTA_PERFORMANCE_TRACKING_ENABLED */

/**
 * Utility function.
 * From a vector of JString returns a vector of Jxta_id.
 * The user should still release the input vector
 *
 * @param peers The input vector
 * @param returns the output vector
 */
Jxta_vector *getPeerids(Jxta_vector * peers);

char* get_service_key(const char * svc_name, const char * svc_param);

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
Jxta_status qos_setting_to_xml(apr_hash_t * setting, char ** result, apr_pool_t * p);

/**
 * Parse a jxta:QoS_Setting XML paragraph into a hash table of QoS setting
 */
Jxta_status xml_to_qos_setting(const char * xml, apr_hash_t ** dest, apr_pool_t * p);

/**
 * Convert the current capability into a jxta:QoS_Support XML paragraph. A sample is like:
 *  <QoS_Support>
 *    <QoS>lifespan</QoS>
 *    <QoS>priority</QoS>
 *    <QoS>TTL</QoS>
 *  </QoS_Support>
 * The string will be allocated from the pool assigned when create the QoS object
 */
Jxta_status qos_support_to_xml(const char ** capability_list, char ** result, apr_pool_t * p);

/**
 * Parse a jxta:QoS_Support XML paragraph into a list of QoS capabilities
 */
Jxta_status xml_to_qos_support(const char * xml, char *** result, apr_pool_t * p);

Jxta_status ep_tcp_socket_listen(apr_socket_t ** me, const char * addr, apr_port_t port, apr_int32_t backlog, apr_pool_t * p);

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
Jxta_status query_all_advs(const char *query,Jxta_credential *scope[], int threshold, Jxta_vector ** advertisements);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
