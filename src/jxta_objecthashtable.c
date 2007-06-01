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
 * $Id: jxta_objecthashtable.c,v 1.9.2.1 2005/06/08 23:09:49 slowhog Exp $
 */


/*****************************************************************************
 **  Jxta_objecthashtable
 **
 ** A Jxta_objecthashtable is a Jxta_object that manages a hash table of Jxta_object.
 **
 ** Sharing and releasing of Jxta_object is properly done (refer to the API
 ** of Jxta_object).
 **
 *****************************************************************************/
#include <stddef.h>
#include <limits.h>
#include <stdlib.h>

#include "jxtaapr.h"

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_objecthashtable.h"


#ifdef WIN32
#pragma warning ( once : 4115 )
#endif

/*************************************************************************
 **
 *************************************************************************/
typedef struct {
    unsigned int hashk; /* if zero then this entry is usable or reusable */
    Jxta_object *key;   /* pointer at the object or a pointer to the hashtable itself.
                           iff hashk == 0 then if key == &hashtable this entry is
                           being reused and must be retained until rehash */
    Jxta_object *value; /* pointer at the value object */
} Entry;

struct _Jxta_objecthashtable {
    JXTA_OBJECT_HANDLE;

    Jxta_object_hash_func hash;
    Jxta_object_equals_func equals;

    Entry *tbl;

    unsigned int modmask;
    size_t max_occupancy;
    size_t occupancy;
    size_t usage;

#ifndef NDEBUG

    size_t nb_hops;
    size_t nb_lookups;
#endif

    apr_pool_t *pool;   /* for the mutex */
    apr_thread_mutex_t *mutex;
};

typedef struct _Jxta_objecthashtable Jxta_objecthashtable_mutable;

/*************************************************************************
 **
 *************************************************************************/
static void Jxta_objecthashtable_free(Jxta_object * the_table)
{
    Jxta_objecthashtable_mutable *self = (Jxta_objecthashtable_mutable *) the_table;
    size_t i;
    Entry *e;

    /*
     * We need to take the lock in order to make sure that nobody
     * else is using the object. However, since we are destroying
     * the object, there is no need to unlock. In other words, since
     * Jxta_objecthashtable_free is not a public API, if the vector has been
     * properly shared, there should not be any external code that still
     * has a reference on this object. So things should be safe.
     */
    apr_thread_mutex_lock(self->mutex);

    /* Release all the objects contained in the table */
    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        if (e->hashk == 0)
            continue;   /* entry not in use */
        JXTA_OBJECT_RELEASE(e->key);
        e->key = NULL;
        JXTA_OBJECT_RELEASE(e->value);
        e->value = NULL;
        e->hashk = 0;
    }

    /* Free the entries */
    free(self->tbl);

    /* Free the pool and the mutex */
    apr_thread_mutex_unlock(self->mutex);
    apr_thread_mutex_destroy(self->mutex);
    apr_pool_destroy(self->pool);

    /* Free the object itself */
    free(self);
}

/*************************************************************************
 **
 *************************************************************************/
/*
 * The main task is here. It does all the dirty work.
 * We use open addressing with rehashing. The rehash function is a
 * bit special because we do provide a good hash function, so we not
 * want to pay the price of find a prime when expanding the table.
 * Instead the table size is a power of two. In case of collision, we probe
 * the following entries (modulo tbl sz) in an increment which depends
 * on the initial slot, which avoids clustering. The "special" thing comes
 * now: Since the tbl size is a power of 2, to ensure complete probing, we
 * use only odd increments. That leaves us with only half as many probing
 * pathes as there are entries...This is an undetectable impact on clustering
 * and is actualy faster than alternative complex schemes.
 *
 * Removal has an impact on the proper behaviour of all this. We assume
 * that all "hits" for a given hash key are encountered prior to the first
 * empty spot; otherwise we'd be forced to search the entire table every
 * time. As a result, this invariant must be maintained when removing.
 * That means that a removed entry becomes available for reuse but cannot
 * mark the end of a probing path (who knows how many probing pathes it
 * in the middle of. For the same reason, no way to move another entry in
 * its place). So a removed entry stays in-the-way whether reused or not, until
 * we re-hash.
 */
#define SMART_PROBING

static Entry *findspot(Jxta_objecthashtable * hashtbl, unsigned int hashk, Jxta_object * key, Jxta_boolean adding)
{
    Jxta_objecthashtable_mutable *self = (Jxta_objecthashtable_mutable *) hashtbl;

#ifdef SMART_PROBING

    size_t increment;
#endif

    size_t curslot;

    size_t modmask = self->modmask;

    Entry *tbl = self->tbl;
    size_t slot = hashk & modmask;
    Entry *curr = &(tbl[slot]);
    Entry *reuse = NULL;

#ifndef NDEBUG

    size_t hop_cnt = 0;
#endif /* NDEBUG */

    /*
     * The same loop is used for both cases; the difference is not
     * worth redundant code.
     * The test below generates either two identical probing pathes or
     * two different ones, based on whether slot is even or odd.
     */
#ifdef SMART_PROBING

    if (slot & 1) {
        /* odd slot */
        increment = slot;
    } else {
        /* even slot */
        increment = slot + 1;
    }
#endif
    curslot = slot;

    while (1) {
        if (curr->hashk == hashk) {
            /* maybe we found it */
            if (self->equals(curr->key, key)) {

                /*
                 * So we found the spot. If we're adding, we prefer
                 * to reuse an earlier one, if any.
                 */

#ifndef NDEBUG
                /* collect the number of hops and update the stats */
                ++(self->nb_lookups);
                self->nb_hops += hop_cnt;
                if (self->nb_lookups == 200) {
                    /* scale down the stats to compute a sliding avg. */
                    self->nb_hops /= 2;
                    self->nb_lookups = 100;
                }
#endif
                if (adding && (reuse != NULL)) {
                    /* move the found entry to the reuse spot. It will be */
                    /* assigned a new value, but not yet. */
                    reuse->key = curr->key;
                    reuse->value = curr->value;
                    reuse->hashk = hashk;

                    /* mark this spot as reusable */
                    /* we use the hashtable's address as a non-null valid ptr
                     * to differentiate reusable entries from blank ones.
                     * The address will not be dereferenced or freed.
                     */
                    curr->hashk = 0;
                    curr->value = NULL;
                    curr->key = (Jxta_object *) hashtbl;

                    return reuse;
                }

                return curr;
            }
            /* else this is an uninterresting entry after all; keep looking */
        } else if (curr->hashk == 0) {
            /* so this is either a resuable entry, or a blank one */
            if (curr->key == NULL) {
                /*
                 * This is blank entry.
                 * This concludes a lookup.
                 * Item not found.
                 */
#ifndef NDEBUG
                /* collect the number of hops and update the stats */
                ++(self->nb_lookups);
                self->nb_hops += hop_cnt;
                if (self->nb_lookups == 200) {
                    /* scale down the stats to compute a sliding avg. */
                    self->nb_hops /= 2;
                    self->nb_lookups = 100;
                }
#endif
                if (!adding)
                    return NULL;

                /*
                 * Let's see if we can reuse an earlier spot instead
                 */
                if (reuse != NULL)
                    return reuse;

                return curr;
            }

            /* Else, this is a reusable entry.
             * we never stop until we find an empty spot, but a
             * non-empty spot may be reusable. Remember the earliest one we
             * see; it's the best place for a new entry.
             */
            if (adding && reuse == NULL) {
                reuse = curr;
            }
        }


        /*
         * Apart from all that, follow the probing path.
         */
#ifdef SMART_PROBING
        curslot = (curslot + increment) & modmask;
        curr = &tbl[curslot];
#else

        ++curslot;
        curslot &= modmask;
        curr = &tbl[curslot];
#endif

#ifndef NDEBUG
        /*
         * Since I trust my own sanity only that much, let's question that
         * this stuff realy works, for a while.
         */
        if (curslot == slot) {

            /*
             * Either we missed something or that table is full.
             * Neither is supposed to happen.
             */
            JXTA_LOG("Ooops: hash table does not work? Could not find a slot" "for an item.\nRevise your assumptions.\n");
            JXTA_LOG("In the meantime, we'll just grow that table.\n");

            return NULL;
        }

        /* update nb hops while we're in the ! NDEBUG section. */
        ++hop_cnt;

#endif /* NDEBUG */

    }
}

/*************************************************************************
 **
 *************************************************************************/
/*
 * Grows the table by reallocating a new entries table twice as big as
 * the current one, and re-hashing everything into it. Then the old table
 * is freed.
 */
static Jxta_status grow(Jxta_objecthashtable_mutable * self)
{
    size_t tmp;
    size_t i;
    Entry *e;

    Entry *old_tbl;
    size_t old_capacity;

    Entry *new_tbl = NULL;
    size_t new_modmask;

    /*
     * The occupancy may be mostly due to reusable entries, which
     * a re-hash will clean-up. So, we might not realy need to make the
     * table bigger, but just re-hash it. If the usage is close enough
     * to the limit, we'll still grow, it'd be too bad to pay the price
     * of a rehash and then have to do it again soon after.
     */

    /*
     * be carefull with integer arithmetics...although very unlikely,
     * real_size * N / M could be overflowing. real_size / M * N
     * is bad for small numbers, however. So...
     */
    tmp = self->max_occupancy;
    if (tmp > (UINT_MAX / 3)) {
        tmp = (tmp / 4) * 3;
    } else {
        tmp = (tmp * 3) / 4;
    }

    if (self->usage > tmp)
        new_modmask = ((self->modmask + 1) << 1) - 1;
    else
        new_modmask = self->modmask;

    new_tbl = (Entry *) calloc((new_modmask + 1), sizeof(Entry));

    if (new_tbl == NULL) {
        JXTA_LOG("Out of memory while growing a hash table. Bailing out.\n");
        return JXTA_NOMEM;
    }

    old_tbl = self->tbl;
    old_capacity = self->modmask + 1;

    self->tbl = new_tbl;
    self->modmask = new_modmask;
    tmp = new_modmask + 1;
    if (tmp > (UINT_MAX / 7)) {
        self->max_occupancy = tmp / 10 * 7;
    } else {
        self->max_occupancy = tmp * 7 / 10;
    }

    self->usage = 0;
    self->occupancy = 0;

    /*
     * Re-hash in the new tbl :-(
     */
    for (i = old_capacity, e = old_tbl; i > 0; --i, ++e) {

        Entry *ne;

        if (e->hashk == 0)
            continue;   /* entry not in use */

        ne = findspot(self, e->hashk, e->key, TRUE);

        if (ne == NULL) {
            /*
             * That's *not* supposed to happen, specially since we are
             * already growing the table. This routine is theoretically
             * re-entrant, but it does not make sense to do so here.
             */
            JXTA_LOG("Hash table badly broken. Bailing out\n");
            return JXTA_NOMEM;
        }

        ne->key = e->key;
        ne->value = e->value;
        ne->hashk = e->hashk;

        ++(self->usage);
        ++(self->occupancy);
    }

    free(old_tbl);

    return JXTA_SUCCESS;
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_objecthashtable *jxta_objecthashtable_new(size_t initial_usage, Jxta_object_hash_func hash, Jxta_object_equals_func equals)
{
    size_t real_size = 1;
    apr_status_t res;
    Jxta_objecthashtable_mutable *self;

    if (initial_usage == 0)
        initial_usage = 32;
    initial_usage <<= 1;

    while (real_size < initial_usage)
        real_size <<= 1;

    self = (Jxta_objecthashtable_mutable *) calloc(1, sizeof(Jxta_objecthashtable_mutable));
    if (self == NULL)
        return NULL;

    JXTA_OBJECT_INIT(self, Jxta_objecthashtable_free, 0);

    self->tbl = (Entry *) calloc(real_size, sizeof(Entry));

    if (self->tbl == NULL) {
        free(self);
        return NULL;
    }

    self->hash = hash;
    self->equals = equals;

    self->modmask = real_size - 1;
    /*
     * be carefull with integer arithmetics...although very unlikely,
     * real_size * N / M could be overflowing. real_size / M * N
     * is bad for small numbers, however. So...
     */
    if (real_size > (UINT_MAX / 7)) {
        self->max_occupancy = (real_size / 10) * 7;
    } else {
        self->max_occupancy = (real_size * 7) / 10;
    }
    self->occupancy = 0;
    self->usage = 0;

    /* Create the mutex's pool. */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        free(self->tbl);
        free(self);
        return NULL;
    }

    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self->tbl);
        free(self);
        return NULL;
    }

    return self;
}

/*************************************************************************
 **
 *************************************************************************/
static Jxta_status
jxta_objecthashtable_comboput(Jxta_objecthashtable * hashtbl,
                              Jxta_object * key, Jxta_object * value, Jxta_object ** old_value, Jxta_boolean replace_ok)
{
    Jxta_objecthashtable_mutable *self = (Jxta_objecthashtable_mutable *) hashtbl;
    Entry *e;
    unsigned int hashk;
    Jxta_status res;

    if (!JXTA_OBJECT_CHECK_VALID(hashtbl))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(key))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(value))
        return JXTA_INVALID_ARGUMENT;

    apr_thread_mutex_lock(self->mutex);

    hashk = self->hash(key);
    if (0 == hashk)
        return JXTA_INVALID_ARGUMENT;

    /*
     * At this stage it is hard to tell if we're going to actually add
     * an item or just reuse one.
     */
    if (self->occupancy > self->max_occupancy) {
        res = grow(self);
        if (res != JXTA_SUCCESS)
            goto Common_Exit;
    }

    e = findspot(self, hashk, key, TRUE);

    if (e == NULL) {
        /*
         * That's *not* supposed to happen and a big warning gets displayed.
         * We should still be able to survive the event by growing the table.
         */
        res = grow(self);
        if (JXTA_SUCCESS != res)
            goto Common_Exit;

        e = findspot(self, hashk, key, TRUE);
        if (e == NULL) {
            JXTA_LOG("Hash table badly broken. Bailing out\n");
            res = JXTA_NOMEM;
            goto Common_Exit;
        }
    }

    /*
     * If one day we start to put polymorphism in keys, then we'll be
     * glad that we replace the key, even if the old and new ones match.
     * We make copies of the keys because we do not want to force them to be
     * Jxta_objets.
     */

    if (e->hashk != 0) {
        /* we're replacing */
        if (!replace_ok) {
            res = JXTA_ITEM_EXISTS;
            goto Common_Exit;
        }

        JXTA_OBJECT_RELEASE(e->key);
        e->key = NULL;

        if (old_value == NULL) {
            JXTA_OBJECT_RELEASE(e->value);
        } else {
            *old_value = e->value;
        }
        e->value = NULL;

        /* usage/occupancy does not change */

    } else {

        ++(self->usage);

        if (e->key == NULL) {
            /* this is a completely new entry (not recycled) */
            ++(self->occupancy);
        }
        e->key = NULL;
    }

    e->key = key;
    JXTA_OBJECT_SHARE(e->key);
    e->value = value;
    JXTA_OBJECT_SHARE(e->value);
    e->hashk = hashk;

    res = JXTA_SUCCESS;

  Common_Exit:
    apr_thread_mutex_unlock(self->mutex);

    return res;
}


/*************************************************************************
 **
 *************************************************************************/
Jxta_status
jxta_objecthashtable_replace(Jxta_objecthashtable * hashtbl, Jxta_object * key, Jxta_object * value, Jxta_object ** old_value)
{
    return jxta_objecthashtable_comboput(hashtbl, key, value, old_value, TRUE);
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_status jxta_objecthashtable_put(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object * value)
{
    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    return jxta_objecthashtable_comboput(self, key, value, NULL, TRUE);
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_status jxta_objecthashtable_putnoreplace(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object * value)
{
    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    return jxta_objecthashtable_comboput(self, key, value, NULL, FALSE);
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_status jxta_objecthashtable_get(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object ** found_value)
{
    Entry *e;
    unsigned int hashk;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    apr_thread_mutex_lock(self->mutex);

    hashk = self->hash(key);
    if (0 == hashk)
        return JXTA_INVALID_ARGUMENT;

    e = findspot(self, hashk, key, FALSE);

    if (e == NULL) {
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_ITEM_NOTFOUND;
    }

    if (NULL != found_value) {
        *found_value = e->value;
        JXTA_OBJECT_SHARE(*found_value);
    }

    apr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_status jxta_objecthashtable_del(Jxta_objecthashtable * hashtbl, Jxta_object * key, Jxta_object ** found_value)
{
    Jxta_objecthashtable_mutable *self = (Jxta_objecthashtable_mutable *) hashtbl;
    Entry *e;
    unsigned int hashk;

    if (!JXTA_OBJECT_CHECK_VALID(hashtbl))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(key))
        return JXTA_INVALID_ARGUMENT;

    apr_thread_mutex_lock(self->mutex);

    hashk = self->hash(key);
    if (0 == hashk)
        return JXTA_INVALID_ARGUMENT;

    e = findspot(self, hashk, key, FALSE);

    if (e == NULL) {
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_ITEM_NOTFOUND;
    }

    if (NULL != found_value)
        *found_value = e->value;
    else
        JXTA_OBJECT_RELEASE(e->value);
    e->value = NULL;

    /*
     * No need to either share or release: the object is moving from one
     * place to the other.
     */

    /*
     * Now mark the entry as reusable.
     */
    e->hashk = 0;
    JXTA_OBJECT_RELEASE(e->key);
    e->key = (Jxta_object *) self;      /* we use the hashtable's address as a
                                         * non-null valid ptr to differentiate
                                         * reusable entries from blank ones.
                                         * The address will not be dereferenced
                                         * or freed.
                                         */
    --(self->usage);

    /*
     * occupancy decremented only just before entry gets reused
     * (which re-increments it)
     */

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_status jxta_objecthashtable_delcheck(Jxta_objecthashtable * hashtbl, Jxta_object * key, Jxta_object * value)
{
    Jxta_objecthashtable_mutable *self = (Jxta_objecthashtable_mutable *) hashtbl;
    Entry *e;
    unsigned int hashk;

    if (!JXTA_OBJECT_CHECK_VALID(hashtbl))
        return JXTA_INVALID_ARGUMENT;

    if (!JXTA_OBJECT_CHECK_VALID(key))
        return JXTA_INVALID_ARGUMENT;

    apr_thread_mutex_lock(self->mutex);

    hashk = self->hash(key);
    if (0 == hashk)
        return JXTA_INVALID_ARGUMENT;

    e = findspot(self, hashk, key, FALSE);

    if (e == NULL) {
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_ITEM_NOTFOUND;
    }

    if (value != e->value) {
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_VIOLATION;
    }

    JXTA_OBJECT_RELEASE(e->value);
    e->value = NULL;

    /*
     * Now mark the entry as reusable.
     */
    e->hashk = 0;
    JXTA_OBJECT_RELEASE(e->key);
    e->key = (Jxta_object *) self;      /* we use the hashtable's address as a
                                         * non-null valid ptr to differentiate
                                         * reusable entries from blank ones.
                                         * The address will not be dereferenced
                                         * or freed.
                                         */
    --(self->usage);

    /*
     * occupancy decremented only just before entry gets reused
     * (which re-increments it)
     */

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_vector *jxta_objecthashtable_keys_get(Jxta_objecthashtable * self)
{
    Entry *e;
    Jxta_vector *keys;
    size_t i;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return NULL;

    keys = jxta_vector_new(self->usage);
    if (keys == NULL)
        return NULL;

    apr_thread_mutex_lock(self->mutex);

    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        Jxta_object *obj = e->key;
        if (0 == e->hashk)
            continue;

        (void) jxta_vector_add_object_last(keys, obj);  /* shares automatically */
    }

    apr_thread_mutex_unlock(self->mutex);
    return keys;
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_vector *jxta_objecthashtable_values_get(Jxta_objecthashtable * self)
{
    Entry *e;
    Jxta_vector *vals;
    size_t i;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return NULL;

    vals = jxta_vector_new(self->usage);
    if (vals == NULL)
        return NULL;

    apr_thread_mutex_lock(self->mutex);

    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        Jxta_object *obj;
        if (e->hashk == 0)
            continue;
        obj = e->value;
        (void) jxta_vector_add_object_last(vals, obj);  /* shares automatically */
    }

    apr_thread_mutex_unlock(self->mutex);
    return vals;
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_status jxta_objecthashtable_stupid_lookup(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object ** found_value)
{
    /*
     * This is a diagnostic tool; it does not share the obj and
     * has the worst possible performance.
     */
    Entry *e;
    size_t i;
    unsigned int hashk;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    hashk = self->hash(key);
    if (0 == hashk)
        return JXTA_INVALID_ARGUMENT;

    apr_thread_mutex_lock(self->mutex);

    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        if (e->key != NULL && !self->equals(e->key, key)) {
            /* If this routine is called it means some item wasn't found */
            /* with the fast lookup. See if we have an idea why... */
            if (e->hashk == 0) {
                JXTA_LOG("The entry has been deleted.\n");
                break;
            }

            if (hashk != e->hashk) {
                JXTA_LOG("Hey ! the hashk nb is wrong in this entry.\n");
            }

            *found_value = e->value;

            apr_thread_mutex_unlock(self->mutex);
            return JXTA_SUCCESS;
        }
    }

    apr_thread_mutex_unlock(self->mutex);
    return JXTA_ITEM_NOTFOUND;
}

/*************************************************************************
 **
 *************************************************************************/
Jxta_status
jxta_objecthashtable_stats(Jxta_objecthashtable * self, size_t * size, size_t * usage,
                           size_t * occupancy, size_t * max_occupancy, double *avg_hops)
{
    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    apr_thread_mutex_lock(self->mutex);

    *size = self->modmask + 1;
    *usage = self->usage;
    *occupancy = self->occupancy;
    *max_occupancy = self->max_occupancy;
#ifndef NDEBUG

    *avg_hops = (self->nb_lookups != 0)
        ? ((1.0 * self->nb_hops) / self->nb_lookups) : 0.0;
#else

    *avg_hops = 1.0;    /* make something up! */
#endif

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

/*************************************************************************
 **
 *************************************************************************/
unsigned int jxta_objecthashtable_simplehash(const void *key, size_t ksz)
{
    const unsigned char *s = (const unsigned char *) key;
    unsigned int hash = UINT_MAX;       /* produces better hashes for keys beginging with zero bytes */
    while (ksz--) {
        hash += *s++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    /* hash_key zero is invalid. it remaps to 1 */
    return hash ? hash : 1;
}
