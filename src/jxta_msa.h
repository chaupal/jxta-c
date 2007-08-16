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
 * $Id$
 */


#ifndef __jxta_MSA_H__
#define __jxta_MSA_H__

#include "jxta_advertisement.h"
#include "jxta_pipe_adv.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_MSA _jxta_MSA;
typedef struct _jxta_MSA Jxta_MSA;

/*
 * Transition to new type naming style.
 */

JXTA_DECLARE(Jxta_MSA *) jxta_MSA_new(void);

JXTA_DECLARE(Jxta_status) jxta_MSA_get_xml(Jxta_MSA *, JString **);
JXTA_DECLARE(void) jxta_MSA_parse_charbuffer(Jxta_MSA *, const char *, int len);
JXTA_DECLARE(void) jxta_MSA_parse_file(Jxta_MSA *, FILE * stream);

JXTA_DECLARE(char *) jxta_MSA_get_MSA(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_MSA(Jxta_MSA *, char *);

JXTA_DECLARE(Jxta_id *) jxta_MSA_get_MSID(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_MSID(Jxta_MSA *, Jxta_id *);

JXTA_DECLARE(JString *) jxta_MSA_get_Name(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_Name(Jxta_MSA *, JString *);

JXTA_DECLARE(JString *) jxta_MSA_get_Crtr(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_Crtr(Jxta_MSA *, JString *);

JXTA_DECLARE(JString *) jxta_MSA_get_SURI(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_SURI(Jxta_MSA *, JString *);

JXTA_DECLARE(JString *) jxta_MSA_get_Vers(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_Vers(Jxta_MSA *, JString *);

JXTA_DECLARE(JString *) jxta_MSA_get_Desc(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_Desc(Jxta_MSA *, JString *);

JXTA_DECLARE(JString *) jxta_MSA_get_Parm(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_Parm(Jxta_MSA *, JString *);

JXTA_DECLARE(Jxta_pipe_adv *) jxta_MSA_get_PipeAdvertisement(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_PipeAdvertisement(Jxta_MSA *, Jxta_pipe_adv *);

JXTA_DECLARE(JString *) jxta_MSA_get_Proxy(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_Proxy(Jxta_MSA *, JString *);

JXTA_DECLARE(JString *) jxta_MSA_get_Auth(Jxta_MSA *);
JXTA_DECLARE(void) jxta_MSA_set_Auth(Jxta_MSA *, JString *);

JXTA_DECLARE(Jxta_vector *) jxta_MSA_get_indexes(Jxta_advertisement *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __jxta_MSA_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
