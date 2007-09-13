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
 * $Id$
 */

#ifndef DUMMYPG_TEST_PRIVATE_H
#define DUMMYPG_TEST_PRIVATE_H

/*
 * This file is to serve as an example of what a peergroup
 * implementation must export so that it can be subclassed
 */

/*
 * We're in the test directory but this kind of things is supposed
 * to reside in the src dir and have access to non-public header
 * files.
 */
#include "jxta_peergroup_private.h"

#ifdef __cpluplus
extern "C" {
#if 0
}
#endif
#endif

/*
 * Note: Jxta_dummypg does not normaly need to be public, unless the
 * group has some extra features available to applications. That's why
 * there is no separate incomplete type declaration in a public
 * header file. Here we only export this type to subclassers.
 */

typedef struct _jxta_dummypg Jxta_dummypg;
struct _jxta_dummypg {
    Extends(Jxta_PG);
    /* Add data members here */
};


/*
 * export our methods table to subclassers so that they can copy it
 * at run-time if usefull.
 */
typedef Jxta_PG_methods Jxta_dummypg_methods;

/*
 * export the constructor to subclassers.
 */
extern void jxta_dummypg_construct(Jxta_dummypg* this,
				   Jxta_dummypg_methods const * methods);

/*
 * export the destructor to subclassers.
 */
extern void jxta_dummypg_destruct(Jxta_dummypg* this);

#ifdef __cpluplus
}
#endif

#endif /* DUMMYPG_TEST_PRIVATE_H */
