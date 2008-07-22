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

static const char *__log_cat = "UUID";

#ifdef WIN32
#pragma warning ( once : 4115 4100)
#endif

#include <stddef.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_objecthashtable.h"
#include "jxta_id_priv.h"
#include "jxta_id_uuid_priv.h"
#include "jxta_apr.h"


    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
enum {
    IDBYTEARRAYSIZE = 64,

    UUIDSIZE = 16,
    FLAGSIZE = 1,

    FLAGSIDTYPEOFFSET = FLAGSIZE - 1,
    FLAGCODATID = 0x01,
    FLAGPEERGROUPID = 0x02,
    FLAGPEERID = 0x03,
    FLAGPIPEID = 0x04,
    FLAGMODULECLASSID = 0x05,
    FLAGMODULESPECID = 0x06,

    FLAGSOFFSET = IDBYTEARRAYSIZE - FLAGSIZE,

        /**
         * size of a SHA1 hash. I would use MessageDigest.getDigestLength, but
         * possible exceptions make it difficult to do.
         */
    HASHSIZE = 20,
    UUID_NODE_LENGTH = 6,

    CODATGROUPIDOFFSET = 0,
    CODATIDOFFSET = CODATGROUPIDOFFSET + UUIDSIZE,
    CODATHASHOFFSET = CODATGROUPIDOFFSET + UUIDSIZE,
    CODATPADOFFSET = CODATHASHOFFSET + HASHSIZE,
    CODATPADSIZE = FLAGSOFFSET - CODATPADOFFSET,

    PGGROUPIDOFFSET = 0,
    PGPADOFFSET = PGGROUPIDOFFSET + UUIDSIZE,
    PGPADSIZE = FLAGSOFFSET - PGPADOFFSET,

    PEERGROUPIDOFFSET = 0,
    PEERIDOFFSET = PEERGROUPIDOFFSET + UUIDSIZE,
    PEERPADOFFSET = PEERIDOFFSET + UUIDSIZE,
    PEERPADSIZE = FLAGSOFFSET - PEERPADOFFSET,

    PIPEGROUPIDOFFSET = 0,
    PIPEIDOFFSET = PIPEGROUPIDOFFSET + UUIDSIZE,
    PIPEPADOFFSET = PIPEIDOFFSET + UUIDSIZE,
    PIPEPADSIZE = FLAGSOFFSET - PIPEPADOFFSET,

    MCCLASSIDOFFSET = 0,
    MCROLEIDOFFSET = MCCLASSIDOFFSET + UUIDSIZE,
    MCPADOFFSET = MCROLEIDOFFSET + UUIDSIZE,
    MCPADSIZE = FLAGSOFFSET - MCPADOFFSET,

    MSCLASSIDOFFSET = 0,
    MSSPECIDOFFSET = MSCLASSIDOFFSET + UUIDSIZE,
    MSPADOFFSET = MSSPECIDOFFSET + UUIDSIZE,
    MSPADSIZE = FLAGSOFFSET - MSPADOFFSET
};

typedef struct {

    struct _jxta_id common;
    unsigned char data[IDBYTEARRAYSIZE];
} _jxta_id_uuid;

extern JXTAIDFormat uuid_format;

static const _jxta_id_uuid worldPeerGroupID = {
    {
     JXTA_OBJECT_STATIC_INIT,
     &uuid_format},
    {
     0x59, 0x61, 0x62, 0x61, 0x64, 0x61, 0x62, 0x61,
     0x4A, 0x78, 0x74, 0x61, 0x50, 0x32, 0x50, 0x33,

     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
};

static const _jxta_id_uuid defaultNetPeerGroupID = {
    {
     JXTA_OBJECT_STATIC_INIT,
     &uuid_format},
    {
     0x59, 0x61, 0x62, 0x61, 0x64, 0x61, 0x62, 0x61,
     0x4E, 0x50, 0x47, 0x20, 0x50, 0x32, 0x50, 0x33,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
};

static Jxta_id *translateTbl[][2] = {
    {(Jxta_id *) & defaultNetPeerGroupID, (Jxta_id *) & jxta_defaultNetPeerGroupID},
    {(Jxta_id *) & worldPeerGroupID, (Jxta_id *) & jxta_worldNetPeerGroupID},
    {NULL, NULL}
};

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static int jxta_id_uuid_compare_priv(apr_uuid_t * a, apr_uuid_t * b, Jpr_absolute_time ttime);
static Jxta_id *translateToWellKnown(Jxta_id * jid);
static Jxta_id *translateFromWellKnown(Jxta_id * jid);
static Jxta_status newPeergroupid1(Jxta_id ** pg);
static Jxta_status newPeergroupid2(Jxta_id ** pg, unsigned char const *seed, size_t len);
static Jxta_status newPeerid1(Jxta_id ** peer, Jxta_id * pg);
static Jxta_status newPeerid2(Jxta_id ** peer, Jxta_id * pg, unsigned char const *seed, size_t len);
static Jxta_status newCodatid1(Jxta_id ** codat, Jxta_id * pg);
static Jxta_status newCodatid2(Jxta_id ** codat, Jxta_id * pg, ReadFunc read_func, void *stream);
static Jxta_status newPipeid1(Jxta_id ** pipe, Jxta_id * pg);
static Jxta_status newPipeid2(Jxta_id ** pipe, Jxta_id * pg, unsigned char const *seed, size_t len);
static Jxta_status newModuleclassid1(Jxta_id ** mcid);
static Jxta_status newModuleclassid2(Jxta_id ** mcid, Jxta_id * base);
static Jxta_status newModulespecid(Jxta_id ** msid, Jxta_id * mcid);
static Jxta_status new_from_str(Jxta_id ** id, const char * str, size_t len);
static Jxta_status getUniqueportion(Jxta_id * jid, JString ** uniq);
static void doDelete(Jxta_object * jid);
static Jxta_boolean equals(Jxta_id * jid1, Jxta_id * jid2);
static unsigned int hashcode(Jxta_id * id);
static apr_uint32_t uuid_state_seqnum = 0;

typedef struct  {
	apr_int32_t	time_low;
	apr_int16_t	time_mid;
	apr_int16_t	time_hi_and_version;
	apr_int16_t	clock_seq;
	apr_byte_t	node[6];
} _uuid_fmt;

/**
 Compare the time of a type 1 UUID if the time sequence is equal

    @param a pointer UUID
    @param b pointer UUID

    @return if (a.seq != b.seq) UUID_NOT_COMPARABLE
            if (a.seq == b.seq && a.time == b.time UUID_EQUALS
            if (a.seq == b.seq && a.time > b.time UUID_GREATER_THAN
            if (a.seq == b.seq && a.time < b.time UUID_LESS_THAN

**/
int jxta_id_uuid_compare(apr_uuid_t * a, apr_uuid_t * b)
{
    if (UUID_EQUALS != jxta_id_uuid_seq_no_compare(a, b)) {
        return UUID_NOT_COMPARABLE;
    }
    return jxta_id_uuid_time_stamp_compare(a, b);
}

/**
 Compare the sequence number of UUID a with UUID b

    @param a pointer UUID
    @param b pointer UUID

    @return if (a == b)    UUID_EQUALS
         if (a > b)     UUID_GREATER_THAN
         if (a < b)     UUID_LESS_THAN
**/
int jxta_id_uuid_seq_no_compare(apr_uuid_t * a, apr_uuid_t * b) {
    
    int res = UUID_EQUALS;
    _uuid_fmt *uida = (_uuid_fmt *)a;
    _uuid_fmt *uidb = (_uuid_fmt *)b;
    apr_uint16_t ia = ntohs(uida->clock_seq);
    apr_uint16_t ib = ntohs(uidb->clock_seq);
    if (ia != ib) {
        res = (ia > ib ? UUID_GREATER_THAN:UUID_LESS_THAN);
    }
    return res;
}

/**
 Compare the time stamp of UUID a with UUID b

    @param a pointer UUID
    @param b pointer UUID

    @return if (a == b)    UUID_EQUALS
         if (a > b)     UUID_GREATER_THAN
         if (a < b)     UUID_LESS_THAN
**/
int jxta_id_uuid_time_stamp_compare(apr_uuid_t * a, apr_uuid_t * b) {
    return jxta_id_uuid_compare_priv(a, b, 0);
}

/**
 Compare the timestamp of UUID with absolute time

    @param a pointer UUID
    @param ttime absolute time

    @return if (a.time == absolute time) UUID_EQUALS
            if (a.time > absolute time) UUID_GREATER_THAN
            if (a.time < absolute time) UUID_LESS_THAN
**/
int jxta_id_uuid_time_compare(apr_uuid_t * a, Jpr_absolute_time ttime) {
    return jxta_id_uuid_compare_priv(a, NULL, ttime);
}

static int jxta_id_uuid_compare_priv(apr_uuid_t * a, apr_uuid_t * b, Jpr_absolute_time ttime) {

    apr_uint64_t atime = 0;
    apr_uint32_t timela;
    apr_uint32_t timelb;
    apr_uint16_t timesa;
    apr_uint16_t timesb;
    apr_uint64_t tmp_int;

    timesa = (ntohs(((_uuid_fmt *)a)->time_hi_and_version) & 0x0fff);

    if (NULL != b) {
        timesb = (ntohs(((_uuid_fmt *)b)->time_hi_and_version) & 0x0fff);
        if (timesa != timesb) {
            return timesa > timesb ? UUID_GREATER_THAN:UUID_LESS_THAN;
        }
    } else {
        tmp_int = timesa * pow(2, 48);
        atime += tmp_int;
    }

    timesa = ntohs(((_uuid_fmt *)a)->time_mid);

    if (NULL != b) {
        timesb = ntohs(((_uuid_fmt *)b)->time_mid);
        if (timesa != timesb) {
            return timesa > timesb ? UUID_GREATER_THAN:UUID_LESS_THAN;
        }
    } else {
        tmp_int = timesa * pow(2, 32);
        atime += tmp_int;
    }

    timela = ntohl(((_uuid_fmt *)a)->time_low);

    if (NULL != b) {
        timelb = ntohl(((_uuid_fmt *)b)->time_low);
        if (timela != timelb) {
            return timela > timelb ? UUID_GREATER_THAN:UUID_LESS_THAN;
        }
    } else {

        atime += timela;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "time compare a: " JPR_ABS_TIME_FMT " time: " JPR_ABS_TIME_FMT "\n", atime, ttime);

        if (atime != ttime) {
            return atime > ttime ? UUID_GREATER_THAN:UUID_LESS_THAN;
        }
    }

    /* fall through means time is equal */
    if (b != NULL) {
        timesa = ntohs(((_uuid_fmt *)a)->clock_seq & 0x3fff);
        timesb = ntohs(((_uuid_fmt *)b)->clock_seq & 0x3fff);
        if (timesa != timesb) {
            return timesa > timesb ? UUID_GREATER_THAN:UUID_LESS_THAN;
        }
    }
    return UUID_EQUALS;
}

apr_uuid_t * jxta_id_uuid_increment_seq_number(apr_uuid_t * a) {
    apr_uuid_t *uuid;
    uuid = calloc(1, sizeof(apr_uuid_t));
    if (NULL != uuid) {
        apr_uint16_t tmp_seq;

        memmove(uuid, a, sizeof(apr_uuid_t));
        tmp_seq = ntohs(((_uuid_fmt *)uuid)->clock_seq);
        tmp_seq++;
        ((_uuid_fmt *)uuid)->clock_seq = htons(tmp_seq);
    }

    return uuid;
}

apr_uuid_t * jxta_id_uuid_new()
{
    apr_uint64_t timestamp;
    _uuid_fmt *uuid_fmt;
    apr_uuid_t * uuid;
    uuid = calloc(1, sizeof(apr_uuid_t));
    char tmp[64];
    timestamp = apr_time_now();
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Timestamp " JPR_ABS_TIME_FMT "\n", timestamp);

    uuid_fmt = (_uuid_fmt *) uuid->data;

    uuid_fmt->time_low = (apr_uint32_t)(timestamp & 0xFFFFFFFF);
    uuid_fmt->time_mid = (apr_uint16_t)((timestamp >> 32) & 0xFFFF);
    uuid_fmt->time_hi_and_version = (apr_uint16_t)((timestamp >> 48) & 0x0FFF);
    uuid_fmt->time_hi_and_version |= (1 << 12);

    uuid_fmt->clock_seq = ++uuid_state_seqnum | 0x8000;

    memset(uuid_fmt->node, 0, UUID_NODE_LENGTH);

    uuid_fmt->time_low = htonl(uuid_fmt->time_low);
    uuid_fmt->time_mid = htons(uuid_fmt->time_mid);
    uuid_fmt->time_hi_and_version = htons(uuid_fmt->time_hi_and_version);
    uuid_fmt->clock_seq = htons(uuid_fmt->clock_seq);

    apr_uuid_format(tmp, uuid);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "UUID after network %s\n", tmp);

    return uuid;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_id *translateToWellKnown(Jxta_id * jid)
{
    Jxta_id *result = jid;
    int eachWellKnown;

    for (eachWellKnown = 0; translateTbl[eachWellKnown][0]; eachWellKnown++) {
        if (equals(translateTbl[eachWellKnown][0], jid))
            return translateTbl[eachWellKnown][1];
    }

    return result;
}

static Jxta_id *translateFromWellKnown(Jxta_id * jid)
{
    Jxta_id *result = jid;
    int eachWellKnown;

    for (eachWellKnown = 0; translateTbl[eachWellKnown][0]; eachWellKnown++) {
        if (equals(translateTbl[eachWellKnown][1], jid))
            return translateTbl[eachWellKnown][0];
    }

    return result;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newPeergroupid1(Jxta_id ** pg)
{
    apr_uuid_t uuid;

    /*  get a new uuid and copy it into the peer id area. */
    apr_uuid_get(&uuid);
    if (UUIDSIZE != sizeof(uuid.data)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "APR UUID Size does not match the size of our UUIDs!");
        return JXTA_FAILED;
    }

    return newPeergroupid2(pg, (unsigned char const *) &uuid, UUIDSIZE);
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newPeergroupid2(Jxta_id ** pg, unsigned char const *seed, size_t len)
{

    /* alloc, smear and init */
    _jxta_id_uuid *me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);

    me->common.formatter = &uuid_format;

    memset(me->data, 0, IDBYTEARRAYSIZE);
    me->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET] = FLAGPEERGROUPID;

    len = len > UUIDSIZE ? UUIDSIZE : len;

    /*  get a new uuid and copy it into the codat id area. */
    memcpy(&me->data[PGGROUPIDOFFSET], seed, len);

    me->data[PGGROUPIDOFFSET + 6] &= 0x0f;
    me->data[PGGROUPIDOFFSET + 6] |= 0x40;  /* version 4 */
    me->data[PGGROUPIDOFFSET + 8] &= 0x3f;
    me->data[PGGROUPIDOFFSET + 8] |= 0x80;  /* IETF variant */
    me->data[PGGROUPIDOFFSET + 10] &= 0x3f;
    me->data[PGGROUPIDOFFSET + 10] |= 0x80; /* multicast bit */

    *pg = (Jxta_id *) me;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newPeerid1(Jxta_id ** peer, Jxta_id * pg)
{
    apr_uuid_t uuid;

    /*  get a new uuid and copy it into the peer id area. */
    apr_uuid_get(&uuid);

    if (UUIDSIZE != sizeof(uuid.data)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "APR UUID Size does not match the size of our UUIDs!");
        return JXTA_FAILED;
    }

    return newPeerid2(peer, pg, (unsigned char const *) &uuid, UUIDSIZE);
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newPeerid2(Jxta_id ** peer, Jxta_id * pg, unsigned char const *seed, size_t len)
{

    /*  alloc, smear and init   */
    _jxta_id_uuid *me;

    pg = translateFromWellKnown(pg);

    if (pg->formatter != &uuid_format) {
        return JXTA_INVALID_ARGUMENT;
    }

    me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);

    me->common.formatter = &uuid_format;

    memset(me->data, 0, IDBYTEARRAYSIZE);
    me->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET] = FLAGPEERID;

    /*  copy the peer group id in */
    memcpy(&me->data[PEERGROUPIDOFFSET], &((_jxta_id_uuid *) pg)->data[PGGROUPIDOFFSET], UUIDSIZE);

    len = len > UUIDSIZE ? UUIDSIZE : len;

    /*  get a new uuid and copy it into the codat id area. */
    memcpy(&me->data[PEERIDOFFSET], seed, len);

    me->data[PEERIDOFFSET + 6] &= 0x0f;
    me->data[PEERIDOFFSET + 6] |= 0x40; /* version 4 */
    me->data[PEERIDOFFSET + 8] &= 0x3f;
    me->data[PEERIDOFFSET + 8] |= 0x80; /* IETF variant */
    me->data[PEERIDOFFSET + 10] &= 0x3f;
    me->data[PEERIDOFFSET + 10] |= 0x80;    /* multicast bit */

    *peer = (Jxta_id *) me;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newCodatid1(Jxta_id ** codat, Jxta_id * pg)
{
    apr_uuid_t uuid;

    /*  alloc, smear and init   */
    _jxta_id_uuid *me;

    pg = translateFromWellKnown(pg);

    if (pg->formatter != &uuid_format) {
        return JXTA_INVALID_ARGUMENT;
    }

    me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);

    me->common.formatter = &uuid_format;

    memset(me->data, 0, IDBYTEARRAYSIZE);
    me->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET] = FLAGCODATID;

    /*  copy the peer group id in */
    memcpy(&me->data[CODATGROUPIDOFFSET], &((_jxta_id_uuid *) pg)->data[PGGROUPIDOFFSET], UUIDSIZE);

    /*  get a new uuid and copy it into the codat id area. */
    apr_uuid_get(&uuid);
    if (UUIDSIZE != sizeof(uuid.data)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "APR UUID Size does not match the size of our UUIDs!");
        return JXTA_FAILED;
    }

    memcpy(&me->data[CODATIDOFFSET], uuid.data, UUIDSIZE);

    *codat = (Jxta_id *) me;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newCodatid2(Jxta_id ** codat, Jxta_id * pg, ReadFunc read_func, void *stream)
{

    return JXTA_NOTIMP;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newPipeid1(Jxta_id ** pipe, Jxta_id * pg)
{
    apr_uuid_t uuid;

    /*  get a new uuid and copy it into the codat id area. */
    apr_uuid_get(&uuid);
    if (UUIDSIZE != sizeof(uuid.data)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "APR UUID Size does not match the size of our UUIDs!");
        return JXTA_FAILED;
    }

    return newPipeid2(pipe, pg, (unsigned char const *) &uuid, sizeof(uuid.data));
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newPipeid2(Jxta_id ** pipe, Jxta_id * pg, unsigned char const *seed, size_t len)
{

    /*  alloc, smear and init   */
    _jxta_id_uuid *me;

    pg = translateFromWellKnown(pg);

    if (pg->formatter != &uuid_format) {
        return JXTA_INVALID_ARGUMENT;
    }

    me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);

    me->common.formatter = &uuid_format;

    memset(me->data, 0, IDBYTEARRAYSIZE);
    me->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET] = FLAGPIPEID;

    /*  copy the peer group id in */
    memcpy(&me->data[PIPEGROUPIDOFFSET], &((_jxta_id_uuid *) pg)->data[PGGROUPIDOFFSET], UUIDSIZE);

    len = len > UUIDSIZE ? UUIDSIZE : len;

    /*  get a new uuid and copy it into the codat id area. */
    memcpy(&me->data[PIPEIDOFFSET], seed, len);

    me->data[PIPEIDOFFSET + 6] &= 0x0f;
    me->data[PIPEIDOFFSET + 6] |= 0x40; /* version 4 */
    me->data[PIPEIDOFFSET + 8] &= 0x3f;
    me->data[PIPEIDOFFSET + 8] |= 0x80; /* IETF variant */
    me->data[PIPEIDOFFSET + 10] &= 0x3f;
    me->data[PIPEIDOFFSET + 10] |= 0x80;    /* multicast bit */

    *pipe = (Jxta_id *) me;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newModuleclassid1(Jxta_id ** mcid)
{
    apr_uuid_t uuid;

    /*  alloc, smear and init   */
    _jxta_id_uuid *me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);

    me->common.formatter = &uuid_format;

    memset(me->data, 0, IDBYTEARRAYSIZE);
    me->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET] = FLAGMODULECLASSID;

    /*  get a new uuid and copy it into the class id area. */
    apr_uuid_get(&uuid);
    if (UUIDSIZE != sizeof(uuid.data)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "APR UUID Size does not match the size of our UUIDs!");
        return JXTA_FAILED;
    }

    memcpy(&me->data[MCCLASSIDOFFSET], uuid.data, UUIDSIZE);

    *mcid = (Jxta_id *) me;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newModuleclassid2(Jxta_id ** mcid, Jxta_id * base)
{
    apr_uuid_t uuid;

    /*  alloc, smear and init   */
    _jxta_id_uuid *me;

    if (base->formatter != &uuid_format) {
        return JXTA_INVALID_ARGUMENT;
    }

    me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);

    me->common.formatter = &uuid_format;

    memset(me->data, 0, IDBYTEARRAYSIZE);
    me->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET] = FLAGMODULECLASSID;

    /*  copy the base class id over */
    memcpy(&me->data[MCCLASSIDOFFSET], &((_jxta_id_uuid *) base)->data[MCCLASSIDOFFSET], UUIDSIZE);

    /*  get a new uuid and copy it into the role id area. */
    apr_uuid_get(&uuid);
    if (UUIDSIZE != sizeof(uuid.data)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "APR UUID Size does not match the size of our UUIDs!");
        return JXTA_FAILED;
    }

    memcpy(&me->data[MCROLEIDOFFSET], uuid.data, UUIDSIZE);

    *mcid = (Jxta_id *) me;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status newModulespecid(Jxta_id ** msid, Jxta_id * mcid)
{
    apr_uuid_t uuid;

    /*  alloc, smear and init   */
    _jxta_id_uuid *me;

    if (mcid->formatter != &uuid_format) {
        return JXTA_INVALID_ARGUMENT;
    }

    me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);

    me->common.formatter = &uuid_format;

    memset(me->data, 0, IDBYTEARRAYSIZE);
    me->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET] = FLAGMODULESPECID;

    /*  copy the base class id over */
    memcpy(&me->data[MSCLASSIDOFFSET], &((_jxta_id_uuid *) mcid)->data[MCCLASSIDOFFSET], UUIDSIZE);

    /*  get a new uuid and copy it into the role id area. */
    apr_uuid_get(&uuid);
    if (UUIDSIZE != sizeof(uuid.data)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "APR UUID Size does not match the size of our UUIDs!");
        return JXTA_FAILED;
    }

    memcpy(&me->data[MSSPECIDOFFSET], uuid.data, UUIDSIZE);

    *msid = (Jxta_id *) me;

    return JXTA_SUCCESS;
}

/*
 * param str: the JXTAIDUNIQ portion(after '-')
 */
static Jxta_status new_from_str(Jxta_id ** id, const char * str, size_t len)
{
    char tmp[3] = { '\0', '\0', '\0' };
    unsigned int eachByte;
    Jxta_id *result;
    _jxta_id_uuid *me;

    if (NULL == str) {
        return JXTA_INVALID_ARGUMENT;
    }

    if (len < (2 * FLAGSIZE) || 0 != (len % 2)) {
        return JXTA_INVALID_ARGUMENT;
    }

    /*  alloc, smear and init   */
    me = (_jxta_id_uuid *) malloc(sizeof(_jxta_id_uuid));

    if (NULL == me)
        return JXTA_NOMEM;

    memset(me, 0xdb, sizeof(_jxta_id_uuid));

    JXTA_OBJECT_INIT(me, doDelete, 0);
    me->common.formatter = &uuid_format;
    memset(me->data, 0, IDBYTEARRAYSIZE);

    /*  do all but the last flags */
    for (eachByte = 0; eachByte < (len / 2) - FLAGSIZE; eachByte++) {
        tmp[0] = str[eachByte * 2U];
        tmp[1] = str[eachByte * 2U + 1U];

        me->data[eachByte] = (unsigned char) strtoul(tmp, NULL, 16);
    }

    /*  do the flags  */
    for (eachByte = 0; eachByte < FLAGSIZE; eachByte++) {
        tmp[0] = str[len - (FLAGSIZE + eachByte) * 2U];
        tmp[1] = str[len - (FLAGSIZE + eachByte) * 2U + 1U];

        me->data[FLAGSOFFSET + eachByte] = (unsigned char) strtoul(tmp, NULL, 16);
    }

    result = translateToWellKnown((Jxta_id *) me);

    /*
     * Maybe result is the same object than me, maybe not. In either case, we're
     * now returning a reference to result and not to me.
     * If they're one and the same, the following operation has no effect.
     */
    JXTA_OBJECT_SHARE(result);
    JXTA_OBJECT_RELEASE(me);

    *id = (Jxta_id *) result;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_status getUniqueportion(Jxta_id * jid, JString ** uniq)
{
    unsigned int eachByte;
    unsigned int lastByte;
    JString *result;
    char aChar[4];

    jid = translateToWellKnown(jid);

    /*  is it no longer one of us? */
    if (jid->formatter != &uuid_format) {
        return jxta_id_get_uniqueportion(jid, uniq);
    }

    for (lastByte = FLAGSOFFSET - 1; lastByte > 0; lastByte--)
        if (0 != ((_jxta_id_uuid *) jid)->data[lastByte])
            break;

    result = jstring_new_1((lastByte + 1U) * 2U + strlen(uuid_format.fmt) + 1U);

    /*  save the format */
    jstring_append_2(result, jid->formatter->fmt);
    jstring_append_2(result, "-");

    /*  now the data bytes */
    for (eachByte = 0; eachByte <= lastByte; eachByte++) {
        apr_snprintf(aChar, sizeof(aChar),"%02X", ((_jxta_id_uuid *) jid)->data[eachByte]);
        jstring_append_2(result, aChar);
    }

    /*  and finally the type */
    apr_snprintf(aChar, sizeof(aChar), "%02X", ((_jxta_id_uuid *) jid)->data[FLAGSOFFSET + FLAGSIDTYPEOFFSET]);
    jstring_append_2(result, aChar);

    *uniq = result;

    return JXTA_SUCCESS;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static Jxta_boolean equals(Jxta_id * jid1, Jxta_id * jid2)
{
    _jxta_id_uuid *ujid1 = (_jxta_id_uuid *) jid1;
    _jxta_id_uuid *ujid2 = (_jxta_id_uuid *) jid2;


    if (jid1->formatter != jid2->formatter)
        return FALSE;

    return (0 == memcmp(ujid1->data, ujid2->data, IDBYTEARRAYSIZE) ? TRUE : FALSE);
}

    /************************************************************************
     **
     *************************************************************************/
static unsigned int hashcode(Jxta_id * id)
{
    _jxta_id_uuid *ujid = (_jxta_id_uuid *) id;
    unsigned int hash;

    if (!JXTA_OBJECT_CHECK_VALID(ujid))
        return 0;

    hash = jxta_objecthashtable_simplehash(ujid->data, IDBYTEARRAYSIZE);

    return hash;
}

    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/
static void doDelete(Jxta_object * jid)
{
    if (NULL == jid)
        return;

    memset((void *) jid, 0xdd, sizeof(_jxta_id_uuid));

    free((void *) jid);
}


    /******************************************************************************/
    /*                                                                            */
    /******************************************************************************/

JXTAIDFormat uuid_format = {
    "uuid",

    newPeergroupid1,
    newPeergroupid2,
    newPeerid1,
    newPeerid2,
    newCodatid1,
    newCodatid2,
    newPipeid1,
    newPipeid2,
    newModuleclassid1,
    newModuleclassid2,
    newModulespecid,
    new_from_str,
    getUniqueportion,
    equals,
    hashcode
};


/* vim: set ts=4 sw=4 et tw=130: */
