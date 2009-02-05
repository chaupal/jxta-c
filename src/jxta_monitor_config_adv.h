/* 
 * Copyright (c) 2008 Sun Microsystems, Inc.  All rights reserved.
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


#ifndef JXTA_MONITORCONFIGADVERTISEMENT_H__
#define JXTA_MONITORCONFIGADVERTISEMENT_H__

#include "jxta_types.h"
#include "jxta_advertisement.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif
typedef struct _jxta_MonitorConfigAdvertisement Jxta_MonitorConfigAdvertisement;


JXTA_DECLARE(Jxta_MonitorConfigAdvertisement *) jxta_MonitorConfigAdvertisement_new(void);
JXTA_DECLARE(Jxta_status) jxta_MonitorConfigAdvertisement_get_xml(Jxta_MonitorConfigAdvertisement *, JString **);
JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_parse_charbuffer(Jxta_MonitorConfigAdvertisement *, const char *, size_t len);
JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_parse_file(Jxta_MonitorConfigAdvertisement *, FILE * stream);
JXTA_DECLARE(Jxta_vector *) jxta_MonitorConfigAdvertisement_get_indexes(void);

JXTA_DECLARE(Jxta_boolean)  jxta_MonitorConfig_is_service_enabled(Jxta_MonitorConfigAdvertisement * me);

JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_set_config_name(Jxta_MonitorConfigAdvertisement * ad, const char *buf);
JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_get_config_name(Jxta_MonitorConfigAdvertisement * ad, char **fn);

JXTA_DECLARE(apr_interval_time_t) jxta_MonitorConfig_broadcast_interval(Jxta_MonitorConfigAdvertisement * me);

JXTA_DECLARE(int) monitor_config_threads_init(Jxta_MonitorConfigAdvertisement * me);
JXTA_DECLARE(int) monitor_config_threads_maximum(Jxta_MonitorConfigAdvertisement * me);

JXTA_DECLARE(void) jxta_MonitorConfig_get_disabled_types(Jxta_MonitorConfigAdvertisement * me, Jxta_vector ** types);
/**
*   For other advertisement types which want to parse MonitorConfig as a sub-section.    
**/
void handleJxta_MonitorConfigAdvertisement(void *userdata, const XML_Char * cd, int len);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_MONITORCONFIGADVERTISEMENT_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
