/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: jxta_service.h,v 1.8 2005/06/16 23:11:51 slowhog Exp $
 */

#ifndef JXTA_SERVICE_H
#define JXTA_SERVICE_H

#include "jxta_module.h"


#ifdef __cplusplus
extern "C" {
    /* avoid confusing indenters */
#if 0
}
#endif
#endif
typedef struct _jxta_service Jxta_service;

/**
 * Service objects are not manipulated directly to protect usage
 * of the service. A Service interface is returned to access the service
 * methods.
 * NOTE: Returning a reference to the object that actually holds the state
 * is permitted. This interface is there to leave room for more complex
 * schemes if needed in the future.
 *
 * @param self handle of the Jxta_service object to which the operation is
 * applied.
 * @param service A location where to return (a ptr to) an object that implements
 * the public interface of the service.
 */
JXTA_DECLARE(void) jxta_service_get_interface(Jxta_service * self, Jxta_service ** service);

/**
 * Returns the implementation advertisement for this service.
 *
 * @param self handle of the Jxta_service object to which the operation is
 * applied.
 * @param mia A location where to return (a ptr to) the advertisement.
 * NULL may be returned if the implementation is incomplete and has no impl adv
 * to return.
 */
JXTA_DECLARE(void) jxta_service_get_MIA(Jxta_service * self, Jxta_advertisement ** mia);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_SERVICE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
