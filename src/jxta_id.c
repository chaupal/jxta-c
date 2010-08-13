/*
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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


#include <ctype.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"

#include "jxta_id.h"

#include "jxta_id_priv.h"

const char *jxta_id_URIENCODINGNAME = "urn";
const char *jxta_id_URNNAMESPACE = "jxta";
const char *jxta_id_PREFIX = "urn:jxta:";
const size_t jxta_id_PREFIX_LENGTH = 9;

extern JXTAIDFormat uuid_format;
extern JXTAIDFormat jxta_format;
static JXTAIDFormat *newInstances = &uuid_format;
static JXTAIDFormat *idformats[] = {
    &uuid_format, &jxta_format, NULL
};

    /************************************************************************
    *************************************************************************
    ** JXTA ID INFORMATIONALS
    *************************************************************************
    *************************************************************************/

    /************************************************************************
    ** Returns name of the id format which will be used to construct new
    ** Jxta ids.
    **
    ** @return string containing the name of the default id format.
    *************************************************************************/
JXTA_DECLARE(char const *) jxta_id_get_default_id_format(void)
{
    return newInstances->fmt;
}

    /************************************************************************
    ** Returns names of all of the id formats which are supported by this
    ** implementation as a NULL terminated array of string pointers.
    **
    ** @return NULL terminated array of string pointers containing the names
    ** of all of the id formats supported by this Jxta implementation.
    *************************************************************************/
JXTA_DECLARE(char const **) jxta_id_get_id_formats(void)
{
    static Jxta_boolean init = FALSE;
    static char const **formats;
    if (!init) {
        int eachFormat = 0;
        formats = (const char **) malloc(sizeof(idformats));    /* minor memory leak */

        while (NULL != idformats[eachFormat]) {
            formats[eachFormat] = idformats[eachFormat]->fmt;
            eachFormat++;
        }

        formats[eachFormat] = NULL;
    }

    return formats;
}

JXTA_DECLARE(Jxta_status) jxta_id_peergroupid_new_1(Jxta_id ** pg)
{

    if (NULL == pg)
        return JXTA_INVALID_ARGUMENT;

    return (newInstances->fmt_newPeergroupid1) (pg);
}

JXTA_DECLARE(Jxta_status) jxta_id_peergroupid_new_2(Jxta_id ** pg, unsigned char const *seed, size_t len)
{

    if (NULL == pg)
        return JXTA_INVALID_ARGUMENT;

    if ((NULL == seed) || (len < 4))
        return JXTA_INVALID_ARGUMENT;

    return (newInstances->fmt_newPeergroupid2) (pg, seed, len);
}

JXTA_DECLARE(Jxta_status) jxta_id_peerid_new_1(Jxta_id ** peerid, Jxta_id * pg)
{

    if (NULL == peerid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(pg))
        return JXTA_INVALID_ARGUMENT;

    if (pg->formatter == &jxta_format)
        return (newInstances->fmt_newPeerid1) (peerid, pg);
    else
        return (pg->formatter->fmt_newPeerid1) (peerid, pg);
}

JXTA_DECLARE(Jxta_status) jxta_id_peerid_new_2(Jxta_id ** peerid, Jxta_id * pg, unsigned char const *seed, size_t len)
{

    if (NULL == peerid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(pg))
        return JXTA_INVALID_ARGUMENT;

    if ((NULL == seed) || (len < 4))
        return JXTA_INVALID_ARGUMENT;

    if (pg->formatter == &jxta_format)
        return (newInstances->fmt_newPeerid2) (peerid, pg, seed, len);
    else
        return (pg->formatter->fmt_newPeerid2) (peerid, pg, seed, len);
}

JXTA_DECLARE(Jxta_status) jxta_id_codatid_new_1(Jxta_id ** codatid, Jxta_id * pg)
{

    if (NULL == codatid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(pg))
        return JXTA_INVALID_ARGUMENT;

    if (pg->formatter == &jxta_format)
        return (newInstances->fmt_newCodatid1) (codatid, pg);
    else
        return (pg->formatter->fmt_newCodatid1) (codatid, pg);
}

JXTA_DECLARE(Jxta_status) jxta_id_codatid_new_2(Jxta_id ** codatid, Jxta_id * pg, ReadFunc read_func, void *stream)
{

    if (NULL == codatid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(pg))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == read_func)
        return JXTA_INVALID_ARGUMENT;

    if (pg->formatter == &jxta_format)
        return (newInstances->fmt_newCodatid2) (codatid, pg, read_func, stream);
    else
        return (pg->formatter->fmt_newCodatid2) (codatid, pg, read_func, stream);
}

JXTA_DECLARE(Jxta_status) jxta_id_pipeid_new_1(Jxta_id ** pipeid, Jxta_id * pg)
{

    if (NULL == pipeid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(pg))
        return JXTA_INVALID_ARGUMENT;

    if (pg->formatter == &jxta_format)
        return (newInstances->fmt_newPipeid1) (pipeid, pg);
    else
        return (pg->formatter->fmt_newPipeid1) (pipeid, pg);
}

JXTA_DECLARE(Jxta_status) jxta_id_pipeid_new_2(Jxta_id ** pipeid, Jxta_id * pg, unsigned char const *seed, size_t len)
{

    if (NULL == pipeid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(pg))
        return JXTA_INVALID_ARGUMENT;

    if ((NULL == seed) || (len < 4))
        return JXTA_INVALID_ARGUMENT;

    if (pg->formatter == &jxta_format)
        return (newInstances->fmt_newPipeid2) (pipeid, pg, seed, len);
    else
        return (pg->formatter->fmt_newPipeid2) (pipeid, pg, seed, len);
}

JXTA_DECLARE(Jxta_status) jxta_id_moduleclassid_new_1(Jxta_id ** mcid)
{

    if (NULL == mcid)
        return JXTA_INVALID_ARGUMENT;

    return (newInstances->fmt_newModuleclassid1) (mcid);
}

JXTA_DECLARE(Jxta_status) jxta_id_moduleclassid_new_2(Jxta_id ** mcid, Jxta_id * base)
{

    if (NULL == mcid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(base))
        return JXTA_INVALID_ARGUMENT;

    return (base->formatter->fmt_newModuleclassid2) (mcid, base);
}

JXTA_DECLARE(Jxta_status) jxta_id_modulespecid_new(Jxta_id ** msid, Jxta_id * mcid)
{

    if (NULL == msid)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(mcid))
        return JXTA_INVALID_ARGUMENT;

    return (mcid->formatter->fmt_newModulespecid) (msid, mcid);
}

JXTA_DECLARE(Jxta_status) jxta_id_from_str(Jxta_id ** id, const char * id_str, size_t len)
{
    Jxta_status err=JXTA_SUCCESS;
    const char *fmt = NULL;
    size_t offset;
    int eachFormat;

    if (id_str == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    if (jxta_id_PREFIX_LENGTH > len) {
        return JXTA_INVALID_ARGUMENT;
    }

    if (0 != strncasecmp(id_str, jxta_id_PREFIX, jxta_id_PREFIX_LENGTH)) {
        return JXTA_INVALID_ARGUMENT;
    }

    /*  now get the id format using the original string. */
    fmt = id_str + jxta_id_PREFIX_LENGTH;
    len -= jxta_id_PREFIX_LENGTH;
    for (offset = 0; offset < len; ++offset) {
        if (fmt[offset] == '-')
            break;
    }

    if (offset >= len) {
        return JXTA_INVALID_ARGUMENT;
    }
    len -= (offset + 1);

    for (eachFormat = 0; NULL != idformats[eachFormat]; eachFormat++) {
        if (0 == strncmp(idformats[eachFormat]->fmt, fmt, offset)) {
            err = idformats[eachFormat]->fmt_from_str(id, fmt + (offset + 1), len);
            break;
        }
    }

    if (NULL == idformats[eachFormat]) {
        *id = NULL;
        return JXTA_NOTIMP;
    }

    return err;
}

JXTA_DECLARE(Jxta_status) jxta_id_from_cstr(Jxta_id ** id, const char * id_cstr)
{
    return jxta_id_from_str(id, id_cstr, strlen(id_cstr));
}

JXTA_DECLARE(Jxta_status) jxta_id_from_jstring(Jxta_id ** id, JString * jid)
{
    if (NULL == id)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(jid))
        return JXTA_INVALID_ARGUMENT;

    return jxta_id_from_str(id, jstring_get_string(jid), jstring_length(jid));
}

JXTA_DECLARE(char const *) jxta_id_get_idformat(Jxta_id * jid)
{
    if (!JXTA_OBJECT_CHECK_VALID(jid))
        return NULL;

    return (jid->formatter->fmt);
}

JXTA_DECLARE(Jxta_status) jxta_id_get_uniqueportion(Jxta_id * jid, JString ** string)
{
    if (NULL == string)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(jid))
        return JXTA_INVALID_ARGUMENT;

    return (jid->formatter->fmt_getUniqueportion) (jid, string);
}

JXTA_DECLARE(Jxta_status) jxta_id_to_jstring(Jxta_id * jid, JString ** string)
{
    static char const *prefix = "urn:jxta:";
    Jxta_status result = JXTA_SUCCESS;
    JString *unique = NULL;

    if (NULL == string)
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(jid))
        return JXTA_INVALID_ARGUMENT;

    result = (jid->formatter->fmt_getUniqueportion) (jid, &unique);

    if (JXTA_SUCCESS != result)
        return result;

    *string = jstring_new_1(jstring_length(unique) + sizeof(prefix) + 1);

    if (NULL == *string) {
        result = JXTA_NOMEM;
        goto Common_Exit;
    }

    jstring_append_2(*string, prefix);
    jstring_append_1(*string, unique);

  Common_Exit:
    if (NULL != unique)
        JXTA_OBJECT_RELEASE(unique);
    unique = NULL;

    return result;
}

JXTA_DECLARE(Jxta_status) jxta_id_to_cstr(Jxta_id * id, char **p, apr_pool_t *pool)
{
    static char const *prefix = "urn:jxta:";
    Jxta_status result = JXTA_SUCCESS;
    JString *unique = NULL;

    result = (id->formatter->fmt_getUniqueportion) (id, &unique);
    if (JXTA_SUCCESS != result)
        return result;

    *p = apr_pstrcat(pool, prefix, jstring_get_string(unique), NULL);

    if (NULL == *p) {
        result = JXTA_NOMEM;
    }
    JXTA_OBJECT_RELEASE(unique);

    return result;
}

JXTA_DECLARE(Jxta_boolean) jxta_id_equals(Jxta_id * jid1, Jxta_id * jid2)
{
    if (jid1 == jid2)
        return TRUE;

    if ((NULL == jid1) || (NULL == jid2))
        return FALSE;

    if (!JXTA_OBJECT_CHECK_VALID(jid1))
        return FALSE;

    if (!JXTA_OBJECT_CHECK_VALID(jid2))
        return FALSE;

    return (jid1->formatter->fmt_equals) (jid1, jid2);
}

JXTA_DECLARE(unsigned int) jxta_id_hashcode(Jxta_id * jid)
{

    if (!JXTA_OBJECT_CHECK_VALID(jid))
        return 0;

    return (jid->formatter->fmt_hashcode) (jid);
}

/* vim: set ts=4 sw=4 et tw=130: */
