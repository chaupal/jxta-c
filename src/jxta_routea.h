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
 * $Id: jxta_routea.h,v 1.7 2005/08/03 05:51:18 slowhog Exp $
 */


#ifndef JXTA_ROUTEADVERTISEMENT_H__
#define JXTA_ROUTEADVERTISEMENT_H__

#include "jxta_advertisement.h"

#include "jxta_apa.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
#define ROUTEADV_DEFAULT_LIFETIME   (Jxta_expiration_time)  (1000L * 60L * 15L)
#define ROUTEADV_DEFAULT_EXPIRATION  (Jxta_expiration_time)  (1000L * 60L * 15L)
typedef struct _jxta_RouteAdvertisement Jxta_RouteAdvertisement;

/**
 * Allocate a new route advertisement
 *
 * @param void takes no arguments
 * @return pointer to route advertisement Jxta_RouteAdvertisement
 */
JXTA_DECLARE(Jxta_RouteAdvertisement *)
    jxta_RouteAdvertisement_new(void);

/**
 * Constructs a representation of a route advertisement in
 * xml format.
 *
 * @param Jxta_RouteAdvetisement * pointer to route advertisement
 * @param JString ** address of pointer to JString that 
 *        accumulates xml representation of route advertisement.
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status)
    jxta_RouteAdvertisement_get_xml(Jxta_RouteAdvertisement *, JString **);

/**
 * Wrapper for jxta_advertisement_parse_charbuffer,
 * when call completes, the xml representation of a 
 * buffer is loaded into a route advertisement structure.
 *
 * @param Jxta_RouteAdvertisement * route advertisement to contain the data in 
 *        the xml.
 * @param const char * buffer containing characters 
 *        with xml syntax.
 * @param int len length of character buffer.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_RouteAdvertisement_parse_charbuffer(Jxta_RouteAdvertisement *, const char *, int len);

/**
 * Wrapper for jxta_advertisement_parse_file,
 * when call completes, the xml representation of a 
 * data is loaded into a route advertisement structure.
 *
 * @param Jxta_RouteAdvertisement * route advertisement to contain the data in 
 *        the xml.
 * @param FILE * an input stream.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_RouteAdvertisement_parse_file(Jxta_RouteAdvertisement *, FILE * stream);

/**
 * This function is an artifact of the generating script.
 */
JXTA_DECLARE(char *) jxta_RouteAdvertisement_get_Jxta_RouteAdvertisement(Jxta_RouteAdvertisement *);

/**
 * This function is an artifact of the generating script.
 */
JXTA_DECLARE(void) jxta_RouteAdvertisement_set_Jxta_RouteAdvertisement(Jxta_RouteAdvertisement *, char *);
  /**
   ** Return a vector of indexes to be applied to advertisement tags and attributes. 
   **
   ** The vector returns element/attribute pairs contained in a Jxta_index entry. If the
   ** attribute is NULL then the index is applied to the element. If an attribute is 
   ** desired the element name and attribute name are specified.
   **
   ** @return Jxta_vector: return a vector of element/attribute pairs in Jxta_index struct.
   **
  **/

JXTA_DECLARE(Jxta_vector *)
    jxta_RouteAdvertisement_get_indexes(Jxta_advertisement *);

/**
 * Function gets the Destination AccessPoint Advertisement associated with the route 
 * advertisement.
 *
 * @param Jxta_RouteAdvertisement * route advertisement.
 *
 * @return Jxta_AccessPointAdvertisement access point associated with the route 
 *         advertisement destination.
 */
JXTA_DECLARE(Jxta_AccessPointAdvertisement *)
    jxta_RouteAdvertisement_get_Dest(Jxta_RouteAdvertisement *);

/**
 * Function sets the Destination AccessPoint Advertisement associated with the route 
 * advertisement.
 *
 * @param Jxta_RouteAdvertisement * route advertisement.
 * @param  Jxta_AccessPointAdvertisement access point associated with the route 
 *         advertisement destination.
 * @return void
 */
JXTA_DECLARE(void) jxta_RouteAdvertisement_set_Dest(Jxta_RouteAdvertisement *, Jxta_AccessPointAdvertisement *);
/**
 * Function gets the hops associated with the route 
 * advertisement.
 *
 * @param Jxta_RouteAdvertisement * route advertisement.
 *
 * @return vector of Jxta_AccessPointAdvertisement access point associated with each
 * hop of the route 
 *         advertisement destination.
 */

JXTA_DECLARE(Jxta_vector *)
    jxta_RouteAdvertisement_get_Hops(Jxta_RouteAdvertisement *);

/**
 * Function sets the hops associated with the route 
 * advertisement.
 *
 * @param Jxta_RouteAdvertisement * route advertisement.
 *
 * @param vector of Jxta_AccessPointAdvertisement access point associated with each
 * hop of the route  advertisement.
 */
JXTA_DECLARE(void) jxta_RouteAdvertisement_set_Hops(Jxta_RouteAdvertisement *, Jxta_vector *);

/**
 * Function sets the PID of the dest access point of the route 
 * advertisement.
 *
 * @param Jxta_RouteAdvertisement * route advertisement.
 * @param Jxta_id of the destination access point of the route.
 *
 * @return void
 */
JXTA_DECLARE(void) jxta_RouteAdvertisement_set_DestPID(Jxta_RouteAdvertisement * ad, Jxta_id * id);

/**
 * Function gets the PID of the dest access point of the route 
 * advertisement.
 *
 * @param Jxta_RouteAdvertisement * route advertisement.
 * @return Jxta_id of the destination access point of the route.
 *
 * @return void
 */
JXTA_DECLARE(Jxta_id *)
    jxta_RouteAdvertisement_get_DestPID(Jxta_RouteAdvertisement * ad);


/**
 * Function to add a hop to an existing route. The hop is
 * added as the first element of the route
 *
 * @param Jxta_RouteAdvertisement * route advertisement.
 * @param AccesspointAdvertisement of the hop to add
 *
 * @return void
 */
JXTA_DECLARE(void) jxta_RouteAdvertisement_add_first_hop(Jxta_RouteAdvertisement * ad, Jxta_AccessPointAdvertisement * ap);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif
#endif /* JXTA_ROUTEADVERTISEMENT_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
