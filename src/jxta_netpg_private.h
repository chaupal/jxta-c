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
 * $Id: jxta_netpg_private.h,v 1.14 2005/09/21 21:16:48 slowhog Exp $
 */

#ifndef JXTA_NETPG_PRIVATE_H
#define JXTA_NETPG_PRIVATE_H

#include "jxta_stdpg_private.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/*
 * Note: Jxta_netpg does not normaly need to be public, unless the
 * group has some extra features available to applications. That's why
 * there is no separate incomplete type declaration in a public
 * header file. Here we only export this type to subclassers.
 */
typedef struct _jxta_netpg Jxta_netpg;

struct _jxta_netpg {
    Extends(Jxta_stdpg);


    /*
     * Netpg provides at least these transports.
     */
    Jxta_transport *http;
    Jxta_transport *tcp;
    Jxta_transport *router;
    Jxta_transport *relay;
};


/*
 * We do not need to subtype the base methods table, we're not adding to it.
 * However, typedef it to be nice to subclassers that expect the naming
 * to follow the regular scheme. Hopefully we should be able to get away
 * with just that and not be forced to always subclass the vtbl.
 *
 */
typedef Jxta_stdpg_methods Jxta_netpg_methods;

/*
 * export our methods table to subclassers so that they can copy it
 * at run-time if usefull.
 */
extern Jxta_netpg_methods jxta_netpg_methods;

/*
 * export the constructor to subclassers.
 */
extern void jxta_netpg_construct(Jxta_netpg * self, Jxta_netpg_methods * methods);

/*
 * export the destructor to subclassers.
 */
extern void jxta_netpg_destruct(Jxta_netpg * self);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_NETPG_PRIVATE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
