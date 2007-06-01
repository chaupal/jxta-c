/*
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights
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
 * $Id: jxta_id_priv.h,v 1.14 2005/11/06 00:59:50 slowhog Exp $
 */


#include "jxta_id.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
struct _JXTAIDFormat {
    char const *fmt;

    /* constructors */

     Jxta_status(*fmt_newPeergroupid1) (Jxta_id ** pg);

     Jxta_status(*fmt_newPeergroupid2) (Jxta_id ** pg, unsigned char const *seed, size_t len);

     Jxta_status(*fmt_newPeerid1) (Jxta_id ** peer, Jxta_id * pg);

     Jxta_status(*fmt_newPeerid2) (Jxta_id ** peer, Jxta_id * pg, unsigned char const *seed, size_t len);

     Jxta_status(*fmt_newCodatid1) (Jxta_id ** codat, Jxta_id * pg);

     Jxta_status(*fmt_newCodatid2) (Jxta_id ** codat, Jxta_id * pg, ReadFunc read_func, void *stream);

     Jxta_status(*fmt_newPipeid1) (Jxta_id ** pipe, Jxta_id * pg);

     Jxta_status(*fmt_newPipeid2) (Jxta_id ** pipe, Jxta_id * pg, unsigned char const *seed, size_t len);

     Jxta_status(*fmt_newModuleclassid1) (Jxta_id ** mcid);

     Jxta_status(*fmt_newModuleclassid2) (Jxta_id ** mcid, Jxta_id * base);

     Jxta_status(*fmt_newModulespecid) (Jxta_id ** mcid, Jxta_id * msid);

    Jxta_status(*fmt_from_str) (Jxta_id ** id, const char * str, size_t len);

    /* mutators */

     Jxta_status(*fmt_getUniqueportion) (Jxta_id * me, JString ** string);

     Jxta_boolean(*fmt_equals) (Jxta_id * jid1, Jxta_id * jid2);

    unsigned int (*fmt_hashcode) (Jxta_id * jid);
};

typedef struct _JXTAIDFormat const JXTAIDFormat;

struct _jxta_id {
    JXTA_OBJECT_HANDLE;
    JXTAIDFormat *formatter;

    /*  formats will add local stuff to this. */
};

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
struct _jxta_id_jxta {

    struct _jxta_id common;

    char const *uniquevalue;
};

typedef struct _jxta_id_jxta _jxta_id_jxta;


/**
  We need these as externs to make pointers to them for static initialization.
  We put them in priv though. The "public" pointers to them are in jxta_id.h  
*/
JXTA_DECLARE_DATA _jxta_id_jxta jxta_nullID;

JXTA_DECLARE_DATA _jxta_id_jxta jxta_worldNetPeerGroupID;

JXTA_DECLARE_DATA _jxta_id_jxta jxta_defaultNetPeerGroupID;


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vim: set ts=4 sw=4 et tw=130: */
