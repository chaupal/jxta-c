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
 * $Id: jxta_pa.h,v 1.15 2006/08/24 22:25:39 bondolo Exp $
 */

#ifndef JXTA_PA_H
#define JXTA_PA_H

#include "jstring.h"
#include "jxta_advertisement.h"
#include "jxta_id.h"
#include "jxta_rdv.h"
#include "jxta_svc.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
};
#endif

typedef struct _jxta_PA Jxta_PA;

#define PA_EXPIRATION_LIFE  (Jxta_expiration_time)  (1000L * 60L * 60L * 24L)   /* 1 day */
#define PA_REMOTE_EXPIRATION (Jxta_expiration_time) (1000L * 60L * 60L * 2L)    /* 2 hours */

/**
 * Allocate a new peer advertisement Jxta_PA.
 *
 * @param void takes no arguments.
 *
 * @return Pointer to peer advertisement Jxta_PA.
 */
JXTA_DECLARE(Jxta_PA *) jxta_PA_new(void);

/**
 * Constructs a representation of a peer advertisement in
 * xml format.
 *
 * @param Jxta_PA * pointer to peer advertisement
 * @param JString ** address of pointer to JString that 
 *        accumulates xml representation of peer advertisement.
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status) jxta_PA_get_xml(Jxta_PA *, JString **);

    /**
     * Constructs a representation of a peer advertisement in
     * xml format.                                        
     *
     * <p/>This version begins output at the root node and is suitable for inclusion
     * in other documents
     *
     * @param ad pointer to peer advertisement
     * @param JString ** address of pointer to JString that 
     *        accumulates xml representation of peer advertisement.
     *
     * @return Jxta_status 
     */
JXTA_DECLARE(Jxta_status) jxta_PA_get_xml_1(Jxta_PA * ad, JString ** xml, const char* element_name, const char **atts );

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

JXTA_DECLARE(Jxta_vector *) jxta_PA_get_indexes(Jxta_advertisement *);

/**
 * Wrapper for jxta_advertisement_parse_charbuffer,
 * when call completes, the xml representation of a 
 * buffer is loaded into a peer advertisement structure.
 *
 * @param Jxta_PA * peer advertisement to contain the data in 
 *        the xml.
 * @param const char * buffer containing characters 
 *        with xml syntax.
 * @param int len length of character buffer.
 *
 * @return Jxta_status JXTA_SUCCESS if successful.
 */
JXTA_DECLARE(Jxta_status) jxta_PA_parse_charbuffer(Jxta_PA *, const char *, int len);

/**
 * Wrapper for jxta_advertisement_parse_file,
 * when call completes, the xml representation of a 
 * data is loaded into a peer advertisement structure.
 *
 * @param Jxta_PA * peer advertisement to contain the data in 
 *        the xml.
 * @param FILE * an input stream.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_PA_parse_file(Jxta_PA *, FILE * stream);

/**
 * Function gets the Jxta_id associated with the peer 
 * advertisement.
 *
 * @param Jxta_PA * peer advertisement.
 *
 * @return Jxta_id Jxta_id associated with peer advertisement.
 */
JXTA_DECLARE(Jxta_id *) jxta_PA_get_PID(Jxta_PA *);

/**
 * Sets a Jxta_id associated with a peer advertisement.
 *
 * @param Jxta_PA * peer advertisement
 * @param Jxta_id * a Jxta_id to be set for the peer advertisement.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_PA_set_PID(Jxta_PA *, Jxta_id *);

/**
 * Gets the group Jxta_id associated with the peer advertisement.
 *
 * @param Jxta_PA * peer advertisement
 *
 * @return Jxta_id * pointer to peer group id.
 */
JXTA_DECLARE(Jxta_id *) jxta_PA_get_GID(Jxta_PA *);

/**
 * Sets the peer group id associated with the peer advertisement.
 *
 * @param Jxta_PA * peer advertisement
 * @param Jxta_id * pointer to Jxta_id for peer group id
 *        associated with the peer advertisement.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_PA_set_GID(Jxta_PA *, Jxta_id *);

/**
 * Gets the name of the peer.
 *
 * @param Jxta_PA * peer advertisement
 *
 * @return JString * containing peer name.
 */
JXTA_DECLARE(JString *) jxta_PA_get_Name(Jxta_PA *);

/**
 * Sets the peer name.
 *
 * @param Jxta_PA * peer advertisement
 * @param JString * peer name is contained here.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_PA_set_Name(Jxta_PA *, JString *);

/**
 * Gets the peer description.
 *
 * @param Jxta_Jxta_PA * peer advertisement
 *
 * @return JString * peer description.
 */
JXTA_DECLARE(JString *) jxta_PA_get_Desc(Jxta_PA * ad);

/**
 * Sets the peer description.
 *
 * @param Jxta_Jxta_PGA * peer advertisement
 * @param JString * peer description.
 *
 * return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_PA_set_Desc(Jxta_PA * ad, JString * desc);

/**
 * Gets the debugging status for the peer advertisement.
 *
 * @deprecated This is an element of the PlatformConfig, not the peer advertisement.
 *
 * @param Jxta_PA * peer advertisement.
 *
 * @return JString * containing peer debugging status.
 */
JXTA_DECLARE(JString *) jxta_PA_get_Dbg(Jxta_PA *);

/**
 * Sets the debugging status for the peer advertisement.
 *
 * @deprecated This is an element of the PlatformConfig, not the peer advertisement.
 *
 * @param Jxta_PA * peer advertisement.
 * @param JString * containing peer debugging status.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_PA_set_Dbg(Jxta_PA *, JString *);

/**
 * Sets the services parameters.
 *
 * @param Jxta_PA * peer advertisement.
 *
 * @return Jxta_vector* The current vector of service params.
 */
JXTA_DECLARE(Jxta_vector *) jxta_PA_get_Svc(Jxta_PA *);

/**
 * Get a particular service with the service ID
 *
 * @param Jxta_PA * peer advertisement.
 * @param Jxta_id * Id requested
 * @param Jxta_svc ** location to store the service
 * @return JXTA_SUCCESS if found otherwise JXTA_ITEM_NOTFOUND.
 */
JXTA_DECLARE(Jxta_status) jxta_PA_get_Svc_with_id(Jxta_PA *, Jxta_id *, Jxta_svc **);

/**
 * Sets the services parameters.
 *
 * @param Jxta_PA * peer advertisement
 * @param Jxta_vector * a vector of Svc objects, each describing
 * parameters for one service. The supplied vector replaces the current one,
 * which is released.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_PA_set_Svc(Jxta_PA *, Jxta_vector *);

/**
 * Add first hop to route advertisement of the endpoint service
 *
 * @param Jxta_PA * peer advertisement
 * @param RdvAdvertisement of the relay which will be added as the first hop
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_PA_add_relay_address(Jxta_PA *, Jxta_RdvAdvertisement *);

/**
 * Remove the endpoint relay address
 *
 * @param Jxta_PA * peer advertisement
 * @param Jxta_id of the relay address which will be removed
 *
 * @return Jxta_status
 */
JXTA_DECLARE(Jxta_status) jxta_PA_remove_relay_address(Jxta_PA *, Jxta_id *);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_PA_H */

/* vi: set ts=4 sw=4 tw=130 et: */
