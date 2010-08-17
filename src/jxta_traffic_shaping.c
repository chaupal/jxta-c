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


static const char *__log_cat = "SHAPE";

#include <stdlib.h> /* malloc, free */
#include <assert.h>

#include "jxta_apr.h"

#include "jpr/jpr_excep_proto.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "queue.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_service_private.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_endpoint_address.h"
#include "jxta_traffic_shaping_priv.h"
#include "jxta_router_service.h"
#include "jxta_listener.h"
#include "jxta_peergroup.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_stdpg_private.h"
#include "jxta_routea.h"
#include "jxta_log.h"
#include "jxta_apr.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_util_priv.h"

/* #define P_DEBUG 1 */

typedef struct _frame Frame;
typedef struct _bucket Bucket;
typedef struct _look_ahead Look_ahead;

struct _traffic {
    JXTA_OBJECT_HANDLE;
    int id;
    Jxta_time time;
    apr_int64_t rate;
    apr_int64_t size;
    int interval;
    int frame;
    int look_ahead_seconds;
    float reserve;
    Frame **active_frames;
    Look_ahead *la_ptr;
    apr_thread_mutex_t *frame_lock;
    apr_pool_t *frame_pool;
    apr_int64_t max_bytes;
    apr_int64_t bytes_frame;
    apr_int64_t bytes_frame_normal;
    apr_int64_t bytes_frame_reserve;

    apr_int64_t bytes_interval;
    apr_int64_t bytes_interval_normal;
    apr_int64_t bytes_interval_reserve;

    Ts_max_option max_option;
};

struct _frame {
    int id;
    apr_int64_t size;
    int interval;
    apr_int64_t rate;
    apr_int64_t bytes_available;
    apr_int64_t bytes_reserve;
    Jxta_time start;
    Jxta_time end;
    int num_buckets;
    int active_buckets;
    Bucket **buckets;
    int reserve_pct;
    Look_ahead *look_ahead;
};

struct _bucket {
    int id;
    apr_int64_t bytes_available;
    apr_int64_t bytes_reserve;
    Frame *frame;
    Jxta_time start;
    Jxta_time end;
};

struct _look_ahead {
    int id;
    apr_int64_t bytes_available;
    apr_int64_t rate;
    apr_int64_t max;
    apr_int64_t bytes;
    apr_int64_t reserve;
    Jxta_time look_ahead;
    Jxta_time start;
    Jxta_time end;
};


static void print_bucket_info(JString *log_s, Jxta_time now, Bucket *b, int i, Jxta_boolean display_w);
static void print_frame_info(JString * s, Jxta_time now, Frame *frame, Jxta_boolean display_o);
static void align_buckets(Jxta_time now, Frame *rcv_w, Frame *give_w);
static void update_look_ahead_bytes(Look_ahead *l, Jxta_time now, apr_int64_t size);
static void adjust_look_ahead(Jxta_traffic_shaping *t, Look_ahead *l, Jxta_time now);


static int frame_id=0;
static int bucket_id=0;

static void free_frame(Frame *f)
{
    if (NULL != f->buckets) {
        int j=0;

        while (f->buckets[j]) {
            free(f->buckets[j++]);
        }
        free(f->buckets);

    }
    free(f);
}

static void traffic_free(Jxta_object * traffic)
{
    Jxta_traffic_shaping *self = (Jxta_traffic_shaping *) traffic;
    int i=0;

    while(NULL != self->active_frames && self->active_frames[i]) {
        Frame *f;

        f = self->active_frames[i++];
        free_frame(f);

    }
    if (NULL != self->active_frames) {
        free(self->active_frames);

     }
    if (self->la_ptr) {
        free(self->la_ptr);

    }
    if (NULL != self->frame_lock) {
        apr_thread_mutex_destroy(self->frame_lock);
        apr_pool_destroy(self->frame_pool);
    }

    free(self);

}

static void print_bucket_info(JString *log_s, Jxta_time now, Bucket *b, int i, Jxta_boolean display_w)
{
    Jxta_time end;
    Jxta_time start;
    char tmpbuf[256];

    if (now < b->start && 0 == i) return;

    start = (b->start - now);
    end = (b->end - now);


    if (0==i) {

        if (!display_w) {
            apr_snprintf(tmpbuf, sizeof(tmpbuf), "wid:%d " , b->frame->id);
            jstring_append_2(log_s, tmpbuf);

        } else {
            print_frame_info(log_s, now, b->frame, TRUE);
        }
    }
    jstring_append_2(log_s, "<");
    if (now >= b->start && now <= b->end) {
        jstring_append_2(log_s, "A ");
    }
    apr_snprintf(tmpbuf, sizeof(tmpbuf)
                , "%d %" APR_INT64_T_FMT " " JPR_DIFF_TIME_FMT " * " JPR_DIFF_TIME_FMT ")> "
                , b->id, b->bytes_available, start, end);

    jstring_append_2(log_s, tmpbuf);
}

static void print_frame_info(JString *s, Jxta_time now, Frame *f, Jxta_boolean display_o)
{
    int i = 0;
    Jxta_time start;
    Jxta_time end;
    char tmpbuf[256];

    start = f->start - now;
    end = (f->end - now);


    apr_snprintf(tmpbuf, sizeof(tmpbuf), "fid:%d avail:%" APR_INT64_T_FMT " start:" JPR_DIFF_TIME_FMT " ends:" JPR_DIFF_TIME_FMT " active_buckets:%d\n", f->id
                            , f->bytes_available
                            , start, end 
                            , f->active_buckets);
    jstring_append_2(s, tmpbuf);
    while (NULL != f->buckets && f->buckets[i] != NULL) {
        if (now < ((Bucket *) f->buckets[i])->start && 0 == i) break;
        print_bucket_info(s, now, f->buckets[i], i, FALSE);
        i++;
    }
    if (0 != i) jstring_append_2(s, "\n");
}


static Frame *init_frame(Jxta_time now, int p_interval, apr_int64_t p_rate, int reserve, Jxta_time start, int num_buckets, Jxta_boolean add_buckets, Look_ahead *l)
{
    Frame *f;
    Bucket **buckets;
    Jxta_time f_start;
    apr_int64_t bytes_frame_reserve;
    apr_int64_t interval_size;

    f_start = start;

    f = calloc(1, sizeof(Frame));

    interval_size = p_rate * p_interval;
    bytes_frame_reserve = interval_size * (reserve/100);
    f->size = ((interval_size - bytes_frame_reserve) * num_buckets);
    f->interval = p_interval;
    f->num_buckets = num_buckets;
    f->rate = p_rate;
    f->start = f_start;
    f->end = (f_start + ((num_buckets * f->interval * 1000) -1));
    f->bytes_available = f->size;
    f->id = ++frame_id;
    f->reserve_pct = reserve;
    f->look_ahead = l;
#ifdef P_DEBUG
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "fid:%d interval:%d rate:%" APR_INT64_T_FMT " buckets:%d Start " JPR_DIFF_TIME_FMT " End:" JPR_DIFF_TIME_FMT " size:%" APR_INT64_T_FMT "\n"
                        , f->id, f->interval, f->rate, num_buckets, (start - now), (f->end - now) , f->size);
#endif
    if (add_buckets) {
        int i;
        apr_int64_t bucket_bytes;
        apr_int64_t frame_total=0;

        f->buckets = calloc(num_buckets + 1, sizeof(Bucket *));
        f->active_buckets = num_buckets;
        buckets = f->buckets;
        bucket_bytes = (f->bytes_available/num_buckets);
        frame_total = f->bytes_available;
        for (i=0; i<num_buckets; i++) {
            Bucket *b;
            b = calloc(1, sizeof(Bucket));

            buckets[i] = b;
            b->id = ++bucket_id;
            b->frame = f;
            b->bytes_available = bucket_bytes;
            b->start = f_start;
            b->end = b->start + (p_interval * 1000) - 1;
            f_start = b->end + 1;
            frame_total -= bucket_bytes;
#ifdef P_DEBUG
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Bucket start:" JPR_DIFF_TIME_FMT " end:" JPR_DIFF_TIME_FMT " avail:%" APR_INT64_T_FMT "\n"
                    , b->start - now, b->end - now, b->bytes_available);
#endif
        }
        if (frame_total > 0) {
            buckets[i-1]->bytes_available += frame_total;
        }
        f->buckets[i] = NULL;
    }
    return f;
}


static void adjust_frame(Jxta_time now, Frame *f)
{
    int i=0;

    if (NULL == f->buckets) {
        return;
    }
    while (f->buckets[i]) {
        Bucket *b;

        b = f->buckets[i];
        if (b->end < now) {
            int j=i+1;
            Jxta_boolean first=TRUE;

#ifdef P_DEBUG
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "******* Bucket expired frame_id:%d i:%d bid:%d\n", f->id, i, b->id);
#endif
            f->active_buckets--;

            /* use these bytes */
            /* w->bytes_available += b->bytes_available; */

            while (f->buckets[j]) {
                Bucket *b1;

                b1 = f->buckets[j];
                if (first) {
                    /* use the unused bytes */
                    b1->bytes_available += b->bytes_available;
                    first = FALSE;
                }
                f->buckets[j-1] = b1;
                j++;
            }
            f->buckets[j-1] = NULL;
            free(b);
        } else {
            i++;
        }
    }
}

static void adjust_frames(Jxta_time now, Frame **frames)
{
    int i=0;


    while (frames[i]) {
        JString *log_s=NULL;
        Frame *f;

        f = frames[i];
        log_s = jstring_new_0();
        if (0 == i && f->start > now) return;

        if (f->end < now) {
            int j=i+1;
            Jxta_time start_time;
            Jxta_boolean create_b=FALSE;
            Jxta_boolean align_b=FALSE;
            Jxta_time last_start = f->start;
            Frame *last_f=NULL;
#ifdef P_DEBUG
            char tmpbuf[128];
            apr_snprintf(tmpbuf, sizeof(tmpbuf),  "****** frame %d expired \n", f->id);
            jstring_append_2(log_s, tmpbuf);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "****** frame %d expired \n", f->id);
#endif
            while (frames[j]) {
                frames[j-1] = frames[j];
                last_f = frames[j++];
                if (last_f->end < now) {
                    last_f = NULL;
                    continue;
                } else {
                    last_start = last_f->start;
                }
            }
            start_time = last_start + (f->interval * 1000);
            if (NULL != f->buckets && NULL != last_f) {
                align_b = TRUE;
            } else if (NULL != f->buckets) {
                create_b = TRUE;
                start_time = i == 0 ? now:now + (f->interval * 1000);
            }

            frames[j-1] = init_frame(now, f->interval, f->rate, f->reserve_pct, start_time, f->num_buckets, create_b, f->look_ahead);
            if (align_b) {
                align_buckets(now, frames[j-1], last_f);
            }

            free_frame(f);

            f = NULL;
            frames[j] = NULL;
        } else {
            adjust_frame(now, f);
            i++;
        }
        if (NULL != f) {
            print_frame_info(log_s, now, f, i==0 ? TRUE:FALSE);
        }

        JXTA_OBJECT_RELEASE(log_s);
    }

    return;
}

static void align_buckets(Jxta_time now, Frame *rcv_f, Frame *give_f)
{
    int i=0;
    int j=0;
    Bucket *new_b=NULL;
    Bucket *last_b=NULL;
    int num_buckets_max;
    apr_int64_t bucket_bytes;

    num_buckets_max = give_f->num_buckets;
    bucket_bytes = (rcv_f->bytes_available/num_buckets_max);

    if (NULL == rcv_f->buckets) {
        rcv_f->buckets = calloc(num_buckets_max + 1, sizeof(Bucket *));
    }
    rcv_f->active_buckets = 0;
    rcv_f->start = give_f->start + (rcv_f->interval * 1000);
    rcv_f->bytes_available = 0;
#ifdef P_DEBUG
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "align rcv_w:%d give_w:%d num_buckets_max:%d\n", rcv_f->id, give_f->id, num_buckets_max);
#endif
    while (give_f->buckets[i]) {
        Bucket *give_b;
        Jxta_time start;
        Jxta_time end;

        give_b = give_f->buckets[i++];

        start = give_b->start;
        end =  give_b->end;

        new_b = rcv_f->buckets[j];
#ifdef P_DEBUG
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "bid:%d start:" JPR_DIFF_TIME_FMT " end:" JPR_DIFF_TIME_FMT "\n", give_b->id, start-now, end-now);
#endif
        if (NULL == new_b) {
            new_b = calloc(1, sizeof(Bucket));
        }
        rcv_f->active_buckets++;
        new_b->id = ++bucket_id;
        new_b->frame = rcv_f;
        new_b->bytes_available = bucket_bytes;
        rcv_f->bytes_available += new_b->bytes_available;
        new_b->start = start + (rcv_f->interval * 1000);
        new_b->end = end + (rcv_f->interval * 1000);

        if (NULL != rcv_f->buckets[j]) {
            free(rcv_f->buckets[j]);
        }
        rcv_f->buckets[j++] = new_b;

        last_b = new_b;
#ifdef P_DEBUG
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "new_b id:%d start:" JPR_DIFF_TIME_FMT " end:" JPR_DIFF_TIME_FMT " size:%" APR_INT64_T_FMT "\n", new_b->id, new_b->start - now, new_b->end - now, new_b->bytes_available);
#endif
    }
    while (rcv_f->active_buckets < num_buckets_max) {
        apr_int64_t bytes_max;

        bytes_max = (((give_f->rate * give_f->interval) - rcv_f->look_ahead->reserve) * num_buckets_max);
        new_b = rcv_f->buckets[j];
        if (NULL == new_b) {
            new_b = calloc(1, sizeof(Bucket));
        }
        rcv_f->active_buckets++;
        new_b->id = ++bucket_id;
        new_b->frame = rcv_f;
        new_b->bytes_available = bucket_bytes;
        new_b->start = NULL != last_b ? last_b->start + (rcv_f->interval * 1000):give_f->start + (rcv_f->interval * 1000);
        new_b->end = NULL != last_b ? last_b->end + (rcv_f->interval * 1000):new_b->start + (rcv_f->interval * 1000);
        if ((new_b->bytes_available + rcv_f->bytes_available) <= bytes_max) {
            rcv_f->bytes_available += new_b->bytes_available;
        }

        rcv_f->buckets[j++] = new_b;
        rcv_f->buckets[j] = NULL;
        last_b = new_b;
#ifdef P_DEBUG
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "new_b id:%d j:%d start:" JPR_DIFF_TIME_FMT " end:" JPR_DIFF_TIME_FMT " size:%" APR_INT64_T_FMT "\n", new_b->id, j, new_b->start - now, new_b->end - now, new_b->bytes_available);
#endif
    }
}

static Jxta_boolean check_buckets(Jxta_time now, Frame *f, apr_int64_t size)
{
    int i=0;
    Jxta_boolean enough=FALSE;
    apr_int64_t bytes_available=0;
    char tmpbuf[256];

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Check buckets for size:%" APR_INT64_T_FMT " in w_id:%d with %" APR_INT64_T_FMT " bytes\n", size, f->id, f->bytes_available);

    if (NULL == f->buckets) {
        return (f->bytes_available > size);
    }
    while (f->buckets[i]) {
        Bucket *b_ptr;

        b_ptr = f->buckets[i++];
        if (now > b_ptr->end) {
            continue;
        }
        /* Check the active bucket */
        if (b_ptr->start <= now && now < b_ptr->end) {
            if (b_ptr->bytes_available > size) {
                enough = TRUE;
                apr_snprintf(tmpbuf, sizeof(tmpbuf), "Found enough in b_id:%d", b_ptr->id);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s\n", tmpbuf);
                break;
            }
        }
        bytes_available += b_ptr->bytes_available;

        if (bytes_available > size) {
            enough = TRUE;
            apr_snprintf(tmpbuf, sizeof(tmpbuf),  "bytes available: %" APR_INT64_T_FMT "are now enough", bytes_available);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s\n", tmpbuf);
            break;
        }
    }
    return enough;
}

static void update_bucket_bytes(Jxta_time now, Frame *f, apr_int64_t size)
{
    int i=0;
    apr_int64_t div_size=0;
    apr_int64_t working_size;

    if (now < f->start) {
        return;
    }
    working_size = size;
    while (f->buckets[i]) {
        Bucket *b;
        b = f->buckets[i++];
        if (now > b->end) {
            continue;
        }
#ifdef P_DEBUG
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "b_id:%d div_size:%" APR_INT64_T_FMT " working_size:%" APR_INT64_T_FMT "\n", b->id, div_size, working_size);
#endif
        if (b->bytes_available >= working_size && 0 == div_size) {
            b->bytes_available -= working_size;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Update wid:%d bid:%d %" APR_INT64_T_FMT " minus %" APR_INT64_T_FMT "\n", f->id, b->id, b->bytes_available, working_size);
            break;
        } else {
            if (0 == div_size) {
                div_size = size / f->active_buckets;
                working_size -= div_size;
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Update wid:%d bid:%d %" APR_INT64_T_FMT "minus %" APR_INT64_T_FMT "\n", f->id, b->id, b->bytes_available, div_size);
            if (div_size >= b->bytes_available) {
                b->bytes_available -= div_size;
            } else {
                b->bytes_available = 0;
            }
            working_size -= div_size;
        }
    }
}

static void update_frame_bytes(Jxta_traffic_shaping *traffic, Jxta_time now, apr_int64_t size)
{
    int i=0;
    Frame **active_frames;

    active_frames = traffic->active_frames;
    while (active_frames[i]) {
        Frame *f;

        f=active_frames[i++];
        if (now >= f->start && now < f->end) {
            update_bucket_bytes(now, f, size);
            if (size <= f->bytes_available) {
                f->bytes_available -= size;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR
                                , "Size %" APR_INT64_T_FMT " exceeded frame bytes %" APR_INT64_T_FMT " available reset to 0\n", size, f->bytes_available);
                f->bytes_available = 0;
                assert(f->bytes_available != 0);
            }
        }
    }
    update_look_ahead_bytes(traffic->la_ptr, now, size);
}

static void update_look_ahead_bytes(Look_ahead *l, Jxta_time now, apr_int64_t size)
{
    if (size > l->bytes_available) {
        l->bytes_available = 0;
    } else {
        l->bytes_available -= size;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
                , "look_ahead_available: %" APR_INT64_T_FMT JPR_DIFF_TIME_FMT "\n"
                , l->bytes_available, l->end - now);
}

static Jxta_boolean check_active_frames(Jxta_traffic_shaping *traffic, Jxta_time now, apr_int64_t size)
{
    Jxta_boolean enough=FALSE;
    int i=0;
    Frame **active_frames;

    active_frames = traffic->active_frames;

    adjust_look_ahead(traffic, traffic->la_ptr, now);

    adjust_frames(now, traffic->active_frames);

    while (active_frames[i]) {
        Frame *f;

        f = active_frames[i++];
        /* only check active frames */
        if (now < f->start || now > f->end) {
            continue;
        }
        if (!check_buckets(now, f, size)) {
            enough = FALSE;
            break;;
        }
        enough = TRUE;
    }

    return enough;
}

void traffic_shaping_lock(Jxta_traffic_shaping *traffic)
{
    apr_thread_mutex_lock(traffic->frame_lock);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Traffic shaping lock [%pp]\n", traffic);

}

void traffic_shaping_unlock(Jxta_traffic_shaping *traffic)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Traffic shaping un-lock [%pp]\n", traffic);
    apr_thread_mutex_unlock(traffic->frame_lock);
}

Jxta_status traffic_shaping_check_max(Jxta_traffic_shaping *ts, apr_int64_t length, apr_int64_t *max, float compression, float *compressed)
{
    Jxta_status res = JXTA_SUCCESS;
    float expand_factor;
    float compressed_size;

    assert (NULL != max);
    *max = ts->max_option == TS_MAX_OPTION_FRAME ? ts->bytes_frame_normal:ts->la_ptr->max;
    expand_factor = 1 / compression;
    *max *= expand_factor;

    compressed_size = compression > 0 && compression < 1 ? length / expand_factor:length;
    *compressed = compressed_size;
    if (compressed_size > ts->max_bytes) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
            , "Length:%" APR_INT64_T_FMT " compressed:%f exceeded max bytes:%" APR_INT64_T_FMT " ts->max:%" APR_INT64_T_FMT "\n", length, compressed_size, *max, ts->max_bytes);
        res = JXTA_LENGTH_EXCEEDED;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
                            , "length:%" APR_INT64_T_FMT " compressed_size:%f expand factor: %f max:%" APR_INT64_T_FMT "\n"
                            , length, compressed_size, expand_factor, *max);
    }

    return res;
}

Jxta_boolean traffic_shaping_check_size(Jxta_traffic_shaping *traffic, apr_int64_t size, Jxta_boolean update, Jxta_boolean *look_ahead_update)
{
    Jxta_time now;
    Jxta_boolean enough;

    now = jpr_time_now();

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Check message size:%" APR_INT64_T_FMT "*****\n", size);

    enough = check_active_frames(traffic, now, size);
    if (enough) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "*Have enough to send the message\n");
        *look_ahead_update = FALSE;
        if (update) {
            update_frame_bytes(traffic, now, size);
        }

    } else if (traffic->max_option == TS_MAX_OPTION_LOOK_AHEAD) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
                        , "Don't have enough to send the message check look ahead with %" APR_INT64_T_FMT " bytes\n"
                        , traffic->la_ptr->bytes_available);
        if (size < traffic->la_ptr->bytes_available) {
            enough = TRUE;
            *look_ahead_update = TRUE;
            if (update) {
                traffic->la_ptr->bytes_available -= size;
            }
        }

    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG
                , "enough %s look_ahead_available:-------------------> %" APR_INT64_T_FMT " " JPR_DIFF_TIME_FMT "\n"
                , !enough ? "FALSE":"TRUE", traffic->la_ptr->bytes_available, traffic->la_ptr->end - now);
    if (update) {
        adjust_frames(now, traffic->active_frames);
    }
    return enough;
}

JXTA_DECLARE(void traffic_shaping_update(Jxta_traffic_shaping *traffic, apr_int64_t size, Jxta_boolean look_ahead_update))
{
    Jxta_time now;

    now = jpr_time_now();

    if (look_ahead_update) {
        traffic->la_ptr->bytes_available -= size;
    } else {
        update_frame_bytes(traffic, now , size);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Updated %s to %" APR_INT64_T_FMT "\n"
                                , look_ahead_update ? "look_ahead":"frame"
                                , look_ahead_update ? traffic->la_ptr->bytes_available:0);
    adjust_frames(now, traffic->active_frames);
}

static Look_ahead * init_look_ahead(Jxta_time now, apr_int64_t add, apr_int64_t fc_rate, Jxta_time fc_look_ahead, float fc_reserve)
{
    Look_ahead *l;

    l = calloc(1, sizeof(Look_ahead));
    l->start = now;
    l->end = now + (fc_look_ahead * 1000);
    l->rate = fc_rate;
    /* l->max = fc_rate * fc_look_ahead * 1000; */
    l->max = fc_rate * fc_look_ahead;
    l->look_ahead = fc_look_ahead;
    l->bytes_available = l->max + add ;
    if (l->bytes_available > l->max) {
        l->bytes_available = l->max;
    }
    l->reserve = (l->bytes_available) * (fc_reserve/100);
    l->bytes = l->bytes_available - l->reserve;

    return l;
}

static void adjust_look_ahead(Jxta_traffic_shaping *t, Look_ahead *l, Jxta_time now)
{
    if (l->end > now) {
        Jxta_time_diff diff_time;
        apr_int64_t available;

        diff_time = now - l->start;
        available = l->bytes_available + ((diff_time /1000) * (l->rate));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "look_ahead_available:%" APR_INT64_T_FMT " possibly available:%" APR_INT64_T_FMT "\n", l->bytes_available, available);
        if (available < l->max) {
            l->bytes_available = available;
        } else {
            l->bytes_available = l->max;
        }
        /* l->bytes_available = available; */
        l->start = now;
        /* adjust the end time */
        l->end = l->end + diff_time;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "avail:%" APR_INT64_T_FMT " diff_time:" JPR_DIFF_TIME_FMT " end time:" JPR_DIFF_TIME_FMT "\n"
                    , l->bytes_available, diff_time, l->end-now);
    } else {
        Look_ahead *new_l;

        new_l = init_look_ahead(now, l->bytes_available , t->rate, l->look_ahead, t->reserve);
        free(t->la_ptr);

        t->la_ptr = new_l;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "new look_ahead_available:%" APR_INT64_T_FMT " \n", new_l->bytes_available);
    }
}

JXTA_DECLARE(Jxta_traffic_shaping * traffic_shaping_new())
{
    Jxta_traffic_shaping *ts;

    ts = calloc(1, sizeof(Jxta_traffic_shaping));
    ts->time = -1;
    ts->size = -1;
    ts->interval = -1;
    ts->frame = -1;
    ts->look_ahead_seconds = -1;
    ts->reserve = -1;
    ts->max_option = -1;

    apr_pool_create(&ts->frame_pool, NULL);
    apr_thread_mutex_create(&ts->frame_lock, APR_THREAD_MUTEX_NESTED, ts->frame_pool);

    JXTA_OBJECT_INIT(ts, traffic_free, NULL);

    return ts;
}

void traffic_shaping_init(Jxta_traffic_shaping *t)
{
    int num_active_frames;
    int i = 0;
    Jxta_time start=0;
    Jxta_boolean init = TRUE;
    Jxta_time now;
    Look_ahead *l;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Traffic Shaping Init\n");

    t->rate = t->size / ((apr_int64_t) t->time);
    t->bytes_frame = t->rate * t->frame;
    t->bytes_frame_reserve = t->bytes_frame * (t->reserve/100);
    t->bytes_frame_normal = t->bytes_frame - t->bytes_frame_reserve;

    t->bytes_interval = t->rate * t->interval;
    t->bytes_interval_reserve = t->bytes_interval * (t->reserve/100);
    t->bytes_interval_normal = t->bytes_interval - t->bytes_interval_reserve;

    now = jpr_time_now();

    if (t->look_ahead_seconds > 0) {
        l = init_look_ahead(now, 0, t->rate, t->look_ahead_seconds, t->reserve);
        t->max_bytes = l->max;
        t->la_ptr = l;
    } else {
        l = init_look_ahead(now, 0, 0, 0, 0);
        t->max_bytes = t->bytes_frame;
        t->la_ptr = l;
    }

    start = now;
    i = 0;

    num_active_frames = t->frame/t->interval;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Number of buckets %d size:%" APR_INT64_T_FMT " num_active_frames:%d\n", num_active_frames, t->size, num_active_frames);

    if (NULL == t->active_frames) {
        t->active_frames = calloc(num_active_frames + 1, sizeof(Frame *));
        t->active_frames[num_active_frames] = NULL;
    }

    start = now;
    for (i=0; i < num_active_frames; i++) {
        Frame *act_ptr;

        act_ptr = init_frame(now, t->interval,  t->rate, t->reserve, start, num_active_frames, TRUE, l);
        init = FALSE;
        start += (t->interval * 1000);

        t->active_frames[i] = act_ptr;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ts:[%pp] size:%" APR_INT64_T_FMT " interval:%d rate:%" APR_INT64_T_FMT " max_bytes:%" APR_INT64_T_FMT "\n"
                , t, t->size, t->interval, t->rate, t->max_bytes);

    jxta_log_append(__log_cat,  JXTA_LOG_LEVEL_DEBUG, "ts:[%pp] bytes_frame:%" APR_INT64_T_FMT " bytes_frame_normal:%" APR_INT64_T_FMT " bytes_frame_reserve:%" APR_INT64_T_FMT " bytes_interval:%" APR_INT64_T_FMT " bytes_interval_normal:%" APR_INT64_T_FMT " bytes_interval_reserve:%" APR_INT64_T_FMT "\n"
        , t, t->bytes_frame, t->bytes_frame_normal, t->bytes_frame_reserve
        , t->bytes_interval, t->bytes_interval_normal, t->bytes_interval_reserve);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "la:[%pp] la_available:%" APR_INT64_T_FMT "  la_reserve:%" APR_INT64_T_FMT " la_bytes:%" APR_INT64_T_FMT " la_end:"
         JPR_DIFF_TIME_FMT "\n"
            ,l , l->bytes_available, l->reserve, l->bytes, l->end-now);

    return;
}

void traffic_shaping_set_time(Jxta_traffic_shaping *t, Jxta_time time)
{
    t->time = time;
}

Jxta_time traffic_shaping_time(Jxta_traffic_shaping *t)
{
    return t->time;
}

void traffic_shaping_set_size(Jxta_traffic_shaping *t, apr_int64_t size)
{
    t->size = size;
}

apr_int64_t traffic_shaping_size(Jxta_traffic_shaping *t)
{
    return t->size;
}

void traffic_shaping_set_interval(Jxta_traffic_shaping *t, int interval)
{
    t->interval = interval;
}

int traffic_shaping_interval(Jxta_traffic_shaping *t)
{
    return t->interval;
}

void traffic_shaping_set_frame(Jxta_traffic_shaping *t, Jxta_time seconds)
{
    t->frame = seconds;
}

Jxta_time traffic_shaping_frame(Jxta_traffic_shaping *t)
{
    return t->frame;
}

void traffic_shaping_set_look_ahead(Jxta_traffic_shaping *t, Jxta_time seconds)
{
    t->look_ahead_seconds = seconds;
}

Jxta_time traffic_shaping_look_ahead(Jxta_traffic_shaping *t)
{
    return t->look_ahead_seconds;
}

void traffic_shaping_set_reserve(Jxta_traffic_shaping *t, int reserve)
{
    t->reserve = reserve;
}

int traffic_shaping_reserve(Jxta_traffic_shaping *t)
{
    return t->reserve;
}

Ts_max_option traffic_shaping_max_option(Jxta_traffic_shaping *t)
{
    return t->max_option;
}

void traffic_shaping_set_max_option(Jxta_traffic_shaping *t, Ts_max_option max_option)
{
    t->max_option = max_option;
}

