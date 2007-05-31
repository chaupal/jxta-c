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
 * $Id: jxta_msa.h,v 1.3 2005/02/02 02:58:29 exocetrick Exp $
 */

   
#ifndef __MSA_H__
#define __MSA_H__

#include "jxta_advertisement.h"


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef struct _MSA MSA;

    /*
     * Transition to new type naming style.
     */
typedef struct _MSA Jxta_MSA;

MSA * MSA_new(void);
void MSA_set_handlers(MSA *, XML_Parser, void *);
void MSA_delete(MSA *);
Jxta_status MSA_get_xml(MSA *, JString **);
void MSA_parse_charbuffer(MSA *, const char *, int len); 
void MSA_parse_file(MSA *, FILE * stream);
 
char * MSA_get_MSA(MSA *);
void MSA_set_MSA(MSA *, char *);
 
char * MSA_get_MSID(MSA *);
void MSA_set_MSID(MSA *, char *);
 
char * MSA_get_Name(MSA *);
void MSA_set_Name(MSA *, char *);
 
char * MSA_get_Crtr(MSA *);
void MSA_set_Crtr(MSA *, char *);
 
char * MSA_get_SURI(MSA *);
void MSA_set_SURI(MSA *, char *);
 
char * MSA_get_Vers(MSA *);
void MSA_set_Vers(MSA *, char *);
 
char * MSA_get_Desc(MSA *);
void MSA_set_Desc(MSA *, char *);
 
char * MSA_get_Parm(MSA *);
void MSA_set_Parm(MSA *, char *);
 
char * MSA_get_PipeAdvertisement(MSA *);
void MSA_set_PipeAdvertisement(MSA *, char *);
 
char * MSA_get_Proxy(MSA *);
void MSA_set_Proxy(MSA *, char *);
 
char * MSA_get_Auth(MSA *);
void MSA_set_Auth(MSA *, char *);
 
Jxta_vector * MSA_get_indexes(void);
#endif /* __MSA_H__  */
#ifdef __cplusplus
}
#endif
