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
 * $Id: jxta_mca.h,v 1.6 2005/09/21 21:16:47 slowhog Exp $
 */


#ifndef __MCA_H__
#define __MCA_H__

#include "jxta_advertisement.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _MCA MCA;

/*
 * transition to new type naming style.
 */
typedef struct _MCA Jxta_MCA;

JXTA_DECLARE(MCA *) MCA_new(void);
JXTA_DECLARE(void) MCA_set_handlers(MCA *, XML_Parser, void *);
void MCA_delete(MCA *);
JXTA_DECLARE(Jxta_status) MCA_get_xml(MCA *, JString **);
JXTA_DECLARE(void) MCA_parse_charbuffer(MCA *, const char *, int len);
JXTA_DECLARE(void) MCA_parse_file(MCA *, FILE * stream);

JXTA_DECLARE(char *) MCA_get_MCA(MCA *);
JXTA_DECLARE(void) MCA_set_MCA(MCA *, char *);

JXTA_DECLARE(char *) MCA_get_MCID(MCA *);
JXTA_DECLARE(void) MCA_set_MCID(MCA *, char *);

JXTA_DECLARE(char *) MCA_get_Name(MCA *);
JXTA_DECLARE(void) MCA_set_Name(MCA *, char *);

JXTA_DECLARE(char *) MCA_get_Desc(MCA *);
JXTA_DECLARE(void) MCA_set_Desc(MCA *, char *);

JXTA_DECLARE(Jxta_vector *) MCA_get_indexes(Jxta_advertisement *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __MCA_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
