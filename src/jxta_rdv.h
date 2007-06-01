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
 * $Id: jxta_rdv.h,v 1.11 2005/09/21 21:16:49 slowhog Exp $
 */


#ifndef __RdvAdvertisement_H__
#define __RdvAdvertisement_H__

#include "jxta_advertisement.h"
#include "jxta_routea.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 ** Jxta_RdvAdvertisement
 **
 ** Definition of the opaque type of the JXTA RendezVous Advertisement
 **/
typedef struct _jxta_RdvAdvertisement Jxta_RdvAdvertisement;

/**
 ** Creates a new rendezvous advertisement object. The object is created
 ** empty, and is a Jxta_advertisement.
 **
 ** @return a pointer to a new empty Jxta_RdvAdvertisement. Returns NULL if the
 ** system is out of memory.
 **/
JXTA_DECLARE(Jxta_RdvAdvertisement *) jxta_RdvAdvertisement_new(void);

/**
 ** Builds and returns the XML (wire format) of the pipe advertisement.
 **
 ** @param adv a pointer to the advertisement object.
 ** @param xml return value: a pointer to a pointer which will the generated XML
 ** @return a status: JXTA_SUCCESS when the call was successful,
 **                   JXTA_NOMEM when the system has ran out of memeory
 **                   JXTA_INVALID_PARAMETER when an argument was invalid.
 **/
JXTA_DECLARE(Jxta_status) jxta_RdvAdvertisement_get_xml(Jxta_RdvAdvertisement * adv, JString ** xml);

/**
 ** Parse an XML (wire format) document containing a JXTA advertisement from
 ** a buffer.
 **
 ** @param adv a pointer to the advertisement object.
 ** @param buffer a pointer to the buffer where the advertisement is stored.
 ** @param len size of the buffer
 ** @return a status: JXTA_SUCCESS when the call was successful,
 **                   JXTA_NOMEM when the system has ran out of memeory
 **                   JXTA_INVALID_PARAMETER when an argument was invalid.
 **/
JXTA_DECLARE(Jxta_status) jxta_RdvAdvertisement_parse_charbuffer(Jxta_RdvAdvertisement * adv, const char *buffer, size_t len);

/**
 ** Parse an XML (wire format) document containing a JXTA advertisement from
 ** a FILE.
 **
 ** @param adv a pointer to the advertisement object.
 ** @param stream the stream to get the advertisment from.
 ** @return a status: JXTA_SUCCESS when the call was successful,
 **                   JXTA_NOMEM when the system has ran out of memeory
 **                   JXTA_INVALID_PARAMETER when an argument was invalid.
 **/
JXTA_DECLARE(Jxta_status) jxta_RdvAdvertisement_parse_file(Jxta_RdvAdvertisement * adv, FILE * stream);


/**
 ** Gets the name of the JXTA Advertisement.
 **
 ** @param adv a pointer to the JXTA Advertisement.
 ** @returns a newly malloced pointer to a null terminated string containing the name.
 **/
JXTA_DECLARE(char *) jxta_RdvAdvertisement_get_Name(Jxta_RdvAdvertisement * adv);

/**
 ** Sets the name of the JXTA Advertisement.
 **
 ** @param adv a pointer to the JXTA Advertisement.
 ** @param name a pointer to a null terminated string containing the name.
 ** @return a status: JXTA_SUCCESS when the call was successful,
 **                   JXTA_NOMEM when the system has ran out of memeory
 **                   JXTA_INVALID_PARAMETER when an argument was invalid.
 **/
JXTA_DECLARE(void) jxta_RdvAdvertisement_set_Name(Jxta_RdvAdvertisement * adv, const char *name);

/**
 ** Gets the identifier of the PeerGroup this rendezvous advertisement refers
 ** to.
 **
 ** @param adv a pointer to the Pipe Advertisement.
 ** @returns a pointer to a JString containing the identifier.
 **/
JXTA_DECLARE(Jxta_id *) jxta_RdvAdvertisement_get_RdvGroupId(Jxta_RdvAdvertisement * adv);

/**
 ** Sets the identifier of the PeerGroup this rendezvous advertisement refers
 ** to.
 **
 ** @param adv a pointer to the RDV Advertisement.
 ** @param id a pointer to a JString containing the identifier
 ** @return a status: JXTA_SUCCESS when the call was successful,
 **                   JXTA_NOMEM when the system has ran out of memeory
 **                   JXTA_INVALID_PARAMETER when an argument was invalid.
 **/
JXTA_DECLARE(void) jxta_RdvAdvertisement_set_RdvGroupId(Jxta_RdvAdvertisement * adv, Jxta_id * id);

/**
 ** Gets the identifier of the rendezvous Peer described by this advertisement
 **
 ** @param adv a pointer to the RDV Advertisement.
 ** @returns a pointer to a JString containing the identifier.
 **/
JXTA_DECLARE(Jxta_id *) jxta_RdvAdvertisement_get_RdvPeerId(Jxta_RdvAdvertisement * adv);

/**
** Sets the identifier of the rendezvous Peer described by this advertisement
**
** @param adv a pointer to the RDV Advertisement.
** @param id a pointer to a JString containing the identifier
** @return a status: JXTA_SUCCESS when the call was successful,
**                   JXTA_NOMEM when the system has ran out of memeory
**                   JXTA_INVALID_PARAMETER when an argument was invalid.
**/
JXTA_DECLARE(void) jxta_RdvAdvertisement_set_RdvPeerId(Jxta_RdvAdvertisement * adv, Jxta_id * id);

/**
 ** Gets the identifier of the rendezvous service described by this advertisement
 **
 ** @param adv a pointer to the RDV Advertisement.
 ** @returns a newly malloced pointer to a null terminated string containing the service.
 **/
JXTA_DECLARE(char *) jxta_RdvAdvertisement_get_Service(Jxta_RdvAdvertisement * adv);

/**
 ** Sets the identifier of the rendezvous Peer described by this advertisement
 **
 ** @param adv a pointer to the RDV Advertisement.
 ** @param id a pointer to a string containing the identifier
 ** @return a status: JXTA_SUCCESS when the call was successful,
 **                   JXTA_NOMEM when the system has ran out of memeory
 **                   JXTA_INVALID_PARAMETER when an argument was invalid.
 **/
JXTA_DECLARE(void) jxta_RdvAdvertisement_set_Service(Jxta_RdvAdvertisement * adv, const char *id);

/**
 ** Gets the route advertisement of the rendezvous described by this advertisement
 **
 ** @param adv a pointer to the RDV Advertisement.
 ** @returns a pointer to a RouteAdvertisement containing the identifier.
 **/
JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_RdvAdvertisement_get_Route(Jxta_RdvAdvertisement * adv);

/**
** Sets the route of the rendezvous Peer described by this advertisement
**
** @param adv a pointer to the RDV Advertisement.
** @param route a pointer to a route advertisement
** @return a status: JXTA_SUCCESS when the call was successful,
**                   JXTA_NOMEM when the system has ran out of memeory
**                   JXTA_INVALID_PARAMETER when an argument was invalid.
**/
JXTA_DECLARE(void) jxta_RdvAdvertisement_set_Route(Jxta_RdvAdvertisement * adv, Jxta_RouteAdvertisement * route);


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
JXTA_DECLARE(Jxta_vector *) jxta_RendezvousAdvertisement_get_indexes(Jxta_advertisement *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __RdvAdvertisement_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
