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
 * $Id: jxta_proffer.h,v 1.9 2006/09/19 18:45:47 exocetrick Exp $
 */


#ifndef JXTA_PROFFERADVERTISEMENT_H__
#define JXTA_PROFFERADVERTISEMENT_H__

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_ProfferAdvertisement Jxta_ProfferAdvertisement;

/**
* Create a new proffer advertisement 
*
* @return a proffer adv object
**/
JXTA_DECLARE(Jxta_ProfferAdvertisement *) jxta_ProfferAdvertisement_new(void);

/**
 * Create a new proffer advertisement
 *
 * @param ns NameSpace of the advertisement
 * @param advid Advertisement ID
 * @param peerid PeerID
 *
 * @return a proffer adv object
**/
JXTA_DECLARE(Jxta_ProfferAdvertisement *) jxta_ProfferAdvertisement_new_1(const char *ns, const char * advid, const char *peerid);

/**
* The namespace is the xxx:yyyy of the advertisement.
* 
* @param prof - proffer advertisement
* @param ns - name space in the format ns:advertisement
* 
**/
JXTA_DECLARE(Jxta_status) jxta_proffer_adv_set_nameSpace(Jxta_ProfferAdvertisement * prof, const char *ns);

JXTA_DECLARE(JString *) jxta_proffer_adv_get_nameSpace(Jxta_ProfferAdvertisement * prof);

/**
* Set the unique identifier of this advertisement. It is used to match other entries.
* 
* @param prof -  proffer advertisement
* @param id - unique id of the advertisement
**/

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_set_advId(Jxta_ProfferAdvertisement * prof, const char *id);

/**
* Get the unique identifier of this advertisement. 
*
* @param prof - proffer advertisement
* @param id - unique id of the advertisement
*
**/

JXTA_DECLARE(const char *) jxta_proffer_adv_get_advId(Jxta_ProfferAdvertisement * prof);

/**
*
* @param prof - proffer advertisement
* 
* @return unique identifier of this advertisement 
*
**/

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_set_peerId(Jxta_ProfferAdvertisement * prof, const char *id);
/**
*
* @param prof - proffer advertisement
*
* @return peerid - peerid where this advertisement is located
**/

JXTA_DECLARE(const char *) jxta_proffer_adv_get_peerId(Jxta_ProfferAdvertisement * prof);
/**
*
* @param prof - proffer advertisement
* @param name - element/attribute - attributes are concatenated to the element name seperated by a space
* @param value - value of the entry
*
**/

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_add_item(Jxta_ProfferAdvertisement * prof, const char *name, const char *value);
/**
*
* @param prof - proffer advertisement
* @param out - destination of the advertisement string
**/

JXTA_DECLARE(Jxta_status) jxta_proffer_adv_print(Jxta_ProfferAdvertisement * prof, JString * out);



#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_PROFFERADVERTISEMENT_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
