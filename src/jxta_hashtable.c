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


/**
 **  Jxta_hashtable
 **
 ** A Jxta_hashtable is a Jxta_object that manages a hash table of Jxta_object.
 **
 ** Sharing and releasing of Jxta_object is properly done (refer to the API
 ** of Jxta_object).
 **
 **/
#include <limits.h>
#include <stdlib.h>

#include "jxta_apr.h"

#include "jxta_log.h"
#include "jxta_errno.h"

#include "jxta_object.h"
#include "jxta_hashtable.h"

typedef struct {
    size_t hashk;
    void *key;
    size_t ksz;
    Jxta_object *value;
} Entry;

struct _jxta_hashtable {
    JXTA_OBJECT_HANDLE;

    Entry *tbl;
    unsigned int modmask;
    size_t max_occupancy;
    size_t occupancy;
    size_t usage;

    size_t nb_hops;
    size_t nb_lookups;

    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
};

static const char *__log_cat = "Hashtable";

static void jxta_hashtable_free(Jxta_object * the_table)
{
    Jxta_hashtable *self = (Jxta_hashtable *) the_table;
    int i;
    Entry *e;

    /*
     * NOTE : we do not take the mutex during deletion, because
     * _free can only be called by JXTA_OBJECT_RELEASE when the
     * last reference is gone. If some thread other than the one
     * releasing the object for the last time is using that table
     * it denotes a very serious problem, which consequences go way
     * beyond concurent access. So the mutex is a moot precaution.
     */

    /* Release all the object contained in the table */
    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        if (e->hashk == 0)
            continue;   /* entry not in use */
        free(e->key);
        JXTA_OBJECT_RELEASE(e->value);
    }

    /* Free the entries */
    free(self->tbl);

    if (self->mutex != NULL) {
        /* Free the mutex */
        apr_thread_mutex_destroy(self->mutex);

        /* Free the pool containing the mutex */
        apr_pool_destroy(self->pool);
    }

    /* Free the object itself */
    free(self);
}

JXTA_DECLARE(Jxta_hashtable *) jxta_hashtable_new_0(size_t initial_usage, Jxta_boolean synchronized)
{
    size_t real_size = 1;
    apr_status_t res;
    Jxta_hashtable *self;

    if (initial_usage == 0)
        initial_usage = 32;
    initial_usage <<= 1;

    while (real_size < initial_usage)
        real_size <<= 1;

    self = (Jxta_hashtable *) malloc(sizeof(Jxta_hashtable));
    if (self == 0)
        return NULL;

    memset(self, 0, sizeof(*self));

    JXTA_OBJECT_INIT((void *) self, jxta_hashtable_free, 0);

    self->tbl = (Entry *) calloc(real_size, sizeof(Entry) );

    if (self->tbl == NULL) {
        free(self);
        return NULL;
    }

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

    if (!synchronized) {
        return self;
    }

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

JXTA_DECLARE(Jxta_hashtable *) jxta_hashtable_new(size_t initial_usage)
{
    return jxta_hashtable_new_0(initial_usage, FALSE);
}

static unsigned long hash(const void *key, size_t ksz)
{
    const unsigned char *s = (const unsigned char *) key;
    unsigned long hash = 0;
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

static Entry *findspot(Jxta_hashtable * self, size_t hashk, const void *key, size_t ksz, Jxta_boolean adding)
{

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
            if (curr->ksz == ksz && memcmp(curr->key, key, ksz) == 0) {

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
                    reuse->ksz = curr->ksz;
                    reuse->value = curr->value;
                    reuse->hashk = hashk;

                    /* mark this spot as reusable */
                    /* we use the hashtable's address as a non-null valid ptr
                     * to differentiate reusable entries from blank ones.
                     * The address will not be dereferenced or freed.
                     */
                    curr->hashk = 0;
                    curr->value = NULL;
                    curr->key = self;

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

        /*
         * Since I trust my own sanity only that much, let's question that
         * this stuff realy works, for a while.
         */
        if (curslot == slot) {

            /*
             * Either we missed something or that table is full.
             * Neither is supposed to happen.
             */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            "Ooops: hash table does not work? Could not find a slot for an item." "\n\tRevise your assumptions."
                            "\n\tIn the meantime, we'll just grow that table.\n");

            return NULL;
        }
#ifndef NDEBUG
        /* update nb hops while we're in the ! NDEBUG section. */
        ++hop_cnt;
#endif /* NDEBUG */
    }
}

/*
 * Grows the table by reallocating a new entries table twice as big as
 * the current one, and re-hashing everything into it. Then the old table
 * is freed.
 */
static void grow(Jxta_hashtable * self)
{
    size_t tmp;
    int i;
    Entry *e;
    size_t old_capacity = self->modmask + 1;
    Entry *old_tbl = self->tbl;

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

    if (self->usage > tmp) {
        tmp = old_capacity << 1;
        self->modmask = tmp - 1;
        if (tmp > (UINT_MAX / 7)) {
            self->max_occupancy = tmp / 10 * 7;
        } else {
            self->max_occupancy = tmp * 7 / 10;
        }
    }

    self->tbl = (Entry *) malloc(sizeof(Entry) * (self->modmask + 1));

    if (self->tbl == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL, "Out of memory while growing a hash table. Bailing out.\n");
        abort();
    }
    memset(self->tbl, 0, sizeof(Entry) * (self->modmask + 1));
    self->usage = 0;
    self->occupancy = 0;

    /*
     * Re-hash in the new tbl :-(
     */
    for (i = old_capacity, e = old_tbl; i > 0; --i, ++e) {

        Entry *ne;

        if (e->hashk == 0)
            continue;   /* entry not in use */

        ne = findspot(self, e->hashk, e->key, e->ksz, TRUE);

        if (ne == NULL) {
            /*
             * That's *not* supposed to happen, specially since we are
             * already growing the table. This routine is theoretically
             * re-entrant, but it does not make sense to do so here.
             */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL, "Hash table badly broken. Bailing out\n");
            abort();
        }

        ne->key = e->key;
        ne->ksz = e->ksz;
        ne->value = e->value;
        ne->hashk = e->hashk;

        ++(self->usage);
        ++(self->occupancy);
    }

    free(old_tbl);
}

/*
 * Handles many ways to put an item in the table:
 * - add but refuse to replace. (replace allowed == FALSE)
 * - add or replace and throw away previous value if any. (old_value == NULL)
 * - add or replace and return the previous value if any. (noneof the above)
 */
static Jxta_boolean
jxta_hashtable_comboput(Jxta_hashtable * self, const void *key,
                        size_t key_size, Jxta_object * value, Jxta_object ** old_value, Jxta_boolean replace_allowed)
{
    Entry *e;
    size_t hashk = (size_t) hash(key, key_size);

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(value);

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    /*
     * At this stage it is hard to tell if we're going to actually add
     * an item or just reuse one.
     */
    if (self->occupancy > self->max_occupancy) {
        grow(self);
    }

    e = findspot(self, hashk, key, key_size, TRUE);

    if (e == NULL) {
        /*
         * That's *not* supposed to happen and a big warning gets displayed.
         * We should still be able to survive the event by growing the table.
         */
        grow(self);
        e = findspot(self, hashk, key, key_size, TRUE);
        if (e == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL, "Hash table badly broken. Bailing out\n");
            if (self->mutex != NULL)
                apr_thread_mutex_unlock(self->mutex);
            abort();
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
        if (!replace_allowed) {
            if (self->mutex != NULL)
                apr_thread_mutex_unlock(self->mutex);
            return FALSE;
        }
        free(e->key);
        if (old_value == NULL) {
            JXTA_OBJECT_RELEASE(e->value);
        } else {
            *old_value = e->value;
        }

        /* usage/occupancy does not change */

    } else {

        ++(self->usage);

        if (e->key == NULL) {
            /* this is a completely new entry (not recycled) */
            ++(self->occupancy);
        } else {
            if (self != e->key) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                                "invalid recycled entry (%x != %x) at index %d! hashtable may be corrupt\n", self, e->key,
                                e - self->tbl);
            }
            e->key = NULL;
        }
    }

    e->key = malloc(key_size);
    if (e->key == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL, "Out of memory while inserting in a hash table. " "Bailing out.\n");
        if (self->mutex != NULL)
            apr_thread_mutex_unlock(self->mutex);
        abort();
    }
    memcpy(e->key, key, key_size);

    e->value = JXTA_OBJECT_SHARE(value);
    e->ksz = key_size;
    e->hashk = hashk;

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);

    return TRUE;
}

JXTA_DECLARE(void)
    jxta_hashtable_replace(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object * value, Jxta_object ** old_value)
{
    jxta_hashtable_comboput(self, key, key_size, value, old_value, TRUE);
}

JXTA_DECLARE(void) jxta_hashtable_put(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object * value)
{
    jxta_hashtable_comboput(self, key, key_size, value, NULL, TRUE);
}

JXTA_DECLARE(Jxta_boolean) jxta_hashtable_putnoreplace(Jxta_hashtable * self, const void *key, size_t key_size,
                                                       Jxta_object * value)
{
    return jxta_hashtable_comboput(self, key, key_size, value, NULL, FALSE);
}

JXTA_DECLARE(Jxta_status) jxta_hashtable_contains(Jxta_hashtable * self, const void *key, size_t key_size)
{
    Jxta_status result;
    Entry *e;
    size_t hashk = (size_t) hash(key, key_size);

    JXTA_OBJECT_CHECK_VALID(self);

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    e = findspot(self, hashk, key, key_size, FALSE);

    result = (e == NULL) ? JXTA_ITEM_NOTFOUND : JXTA_SUCCESS;

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);
    return result;
}


JXTA_DECLARE(Jxta_status) jxta_hashtable_get(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object ** found_value)
{
    Jxta_status result;
    Entry *e;
    size_t hashk = (size_t) hash(key, key_size);

    JXTA_OBJECT_CHECK_VALID(self);

    if (found_value == NULL) {
        return JXTA_ITEM_NOTFOUND;
    }

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    e = findspot(self, hashk, key, key_size, FALSE);

    if (e == NULL) {
        result = JXTA_ITEM_NOTFOUND;
    } else {
        *found_value = JXTA_OBJECT_SHARE(e->value);
        result = JXTA_SUCCESS;
    }

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);
        
    return result;
}

JXTA_DECLARE(Jxta_status) jxta_hashtable_del(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object ** found_value)
{
    Entry *e;
    size_t hashk = (size_t) hash(key, key_size);

    JXTA_OBJECT_CHECK_VALID(self);

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    e = findspot(self, hashk, key, key_size, FALSE);

    if (e == NULL) {
        if (self->mutex != NULL)
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
    free(e->key);
    e->hashk = 0;
    e->key = (Jxta_object *) self;      /* we use the hashtable's addr as a valid
                                         * non-null ptr to differentiate reusable
                                         * entries from blank ones.
                                         */

    --(self->usage);

    /*
     * occupancy decremented only just before entry gets reused
     * (which re-increments it)
     */

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_hashtable_delcheck(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object * value)
{
    Entry *e;
    size_t hashk = (size_t) hash(key, key_size);

    JXTA_OBJECT_CHECK_VALID(self);

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    e = findspot(self, hashk, key, key_size, FALSE);

    if (e == NULL) {
        if (self->mutex != NULL)
            apr_thread_mutex_unlock(self->mutex);
        return JXTA_ITEM_NOTFOUND;
    }

    if (value != e->value) {
        if (self->mutex != NULL)
            apr_thread_mutex_unlock(self->mutex);
        return JXTA_VIOLATION;
    }

    JXTA_OBJECT_RELEASE(e->value);
    e->value = NULL;

    /*
     * Now mark the entry as reusable.
     */
    free(e->key);
    e->hashk = 0;
    e->key = (Jxta_object *) self;      /* we use the hashtable's addr as a valid
                                         * non-null ptr to differentiate reusable
                                         * entries from blank ones.
                                         */

    --(self->usage);

    /*
     * occupancy decremented only just before entry gets reused
     * (which re-increments it)
     */

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_vector *) jxta_hashtable_values_get(Jxta_hashtable * self)
{
    Entry *e;
    Jxta_vector *vals;
    int i;

    JXTA_OBJECT_CHECK_VALID(self);

    vals = jxta_vector_new(self->usage);
    if (vals == NULL)
        return NULL;

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        Jxta_object *obj;
        if (e->hashk == 0)
            continue;
        obj = e->value;
        jxta_vector_add_object_last(vals, obj); /* shares automatically */
    }

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);
    return vals;
}

/*
 * FIXME: this inteface works only for string keys. If you put
 * other than string keys in your hashtbl, this is not usuable.
 */
JXTA_DECLARE(char **) jxta_hashtable_keys_get(Jxta_hashtable * self)
{
    Entry *e;
    int i;
    char **keys;
    size_t key_i = 0;

    JXTA_OBJECT_CHECK_VALID(self);

    keys = (char **) malloc(sizeof(char *) * (self->usage + 1));
    if (keys == NULL)
        return NULL;

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        char *key;
        if (e->hashk == 0)
            continue;
        key = malloc(e->ksz + 1);
        if (key == NULL)
            continue;
        memcpy(key, e->key, e->ksz);
        key[e->ksz] = '\0';
        keys[key_i++] = key;
    }
    keys[key_i] = NULL;

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);
    return keys;
}

/*
 * This is a diagnostic tool; it does not share the obj and
 * has the worst possible performance.
 */
JXTA_DECLARE(Jxta_status) jxta_hashtable_stupid_lookup(Jxta_hashtable * self, const void *key, size_t key_size,
                                                       Jxta_object ** found_value)
{
    Entry *e;
    int i;
    size_t hashk = hash(key, key_size);

    JXTA_OBJECT_CHECK_VALID(self);

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        if (e->key != NULL && !memcmp(e->key, key, key_size)) {
            /* If this routine is called it means some item wasn't found */
            /* with the fast lookup. See if we have an idea why... */
            if (e->hashk == 0) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "The entry has been deleted.\n");
                break;
            }
            if (hashk != e->hashk) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Hey ! the hashk nb is wrong in this entry.\n");
            }
            if (e->ksz != key_size) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Watchout ! The key sizes do not match (yours was used).");
            }
            *found_value = e->value;
            if (self->mutex != NULL)
                apr_thread_mutex_unlock(self->mutex);
            return JXTA_SUCCESS;
        }
    }

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);
    return JXTA_ITEM_NOTFOUND;
}

JXTA_DECLARE(void) jxta_hashtable_clear(Jxta_hashtable * self)
{
    int i;
    Entry *e;

    JXTA_OBJECT_CHECK_VALID(self);

    if (self->mutex != NULL)
        apr_thread_mutex_lock(self->mutex);

    /* Release all the object contained in the table */
    for (i = self->modmask + 1, e = self->tbl; i > 0; --i, ++e) {
        if (e->hashk == 0)
            continue;   /* entry not in use */
        free(e->key);
        e->key = (Jxta_object *) self;  /* we use the hashtable's addr as a valid
                                         * non-null ptr to differentiate reusable
                                         * entries from blank ones.
                                         */

        e->hashk = 0;
        JXTA_OBJECT_RELEASE(e->value);
        e->value = NULL;
        --(self->usage);
    }

    if (self->mutex != NULL)
        apr_thread_mutex_unlock(self->mutex);
}

JXTA_DECLARE(void)
jxta_hashtable_stats(Jxta_hashtable * self, size_t * capacity, size_t * usage,
                     size_t * occupancy, size_t * max_occupancy, double *avg_hops)
{
    JXTA_OBJECT_CHECK_VALID(self);

    if (self->mutex != NULL) {
        apr_thread_mutex_lock(self->mutex);
    }

    if (NULL != capacity) {
        *capacity = self->modmask + 1;
    }

    if (NULL != usage) {
        *usage = self->usage;
    }

    if (NULL != occupancy) {
        *occupancy = self->occupancy;
    }

    if (NULL != max_occupancy) {
        *max_occupancy = self->max_occupancy;
    }

    if (NULL != avg_hops) {
        *avg_hops = (self->nb_lookups != 0)
            ? ((1.0 * self->nb_hops) / self->nb_lookups)
            : 0.0;
    }

    if (self->mutex != NULL) {
        apr_thread_mutex_unlock(self->mutex);
    }
}
