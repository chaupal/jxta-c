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
 * $Id: jxta_objecthashtable.h,v 1.7 2005/09/21 21:16:48 slowhog Exp $
 */


/******************************************************************************
 **  Jxta_objecthashtable
 **
 ** A Jxta_objecthashtable is a Jxta_object that manages a hash table of Jxta_object.
 **
 ** Sharing and releasing of Jxta_object is properly done (refer to the API of
 ** Jxta_object).
 **
 *****************************************************************************/


#ifndef JXTA_OBJECTHASHTABLE_H
#define JXTA_OBJECTHASHTABLE_H

#include "jxta_object.h"

/* #include "jxta_types.h" */
#include "jxta_vector.h"


#ifdef __cplusplus
extern "C" {
    /* avoid confusing indenters */
#if 0
};
#endif
#endif

/*
 * The opaque type definition.
 */
typedef struct _Jxta_objecthashtable const Jxta_objecthashtable;


/************************************************************************
 ** Allocates a new Hash Table. The table is allocated and is initialized
 ** (ready to use). No need to apply JXTA_OBJECT_INIT, because new does it.
 **
 ** If that value is null, then a default size is used.
 **
 ** The creator of a hash table is responsible to release it when not used
 ** anymore.
 **
 ** @param initial_size is the initial number of items that can go in the table.
 ** 0 means default. Until this number of items is reached, the table will not
 ** be enlarged. The size of the underlying table may be bigger, depending on
 ** the implementation.
 ** @return a new hash table, or NULL if allocation failed.
 *************************************************************************/

JXTA_DECLARE(Jxta_objecthashtable *)
    jxta_objecthashtable_new(size_t initial_size, Jxta_object_hash_func hash, Jxta_object_equals_func equals);


/************************************************************************
 ** Inserts a new item under the specified key. If an item already
 ** existed under that key, it is removed from the table.
 ** key is duplicated into a internal buffer. value is shared automatically.
 **
 ** @param key A pointer to a Jxta_object to be used as the key. May not be NULL.
 ** @param value A pointer to a Jxta_object to be associated with the key
 ** @return JXTA_SUCCESS or (rare) errors otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_objecthashtable_put(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object * value);

/************************************************************************
 ** Inserts a new item under the specified key. If an item already
 ** existed under that key, insertion is not performed.
 ** key and values are shared automatically.
 **
 ** @param key A pointer to a Jxta_object to be used as the key. May not be NULL.
 ** @param value A pointer to a Jxta_object to be associated with the key
 ** @return JXTA_SUCCESS or errors otherwise. JXTA_ITEM_EXISTS is
 ** returned if an item already exists under the given key.
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_objecthashtable_putnoreplace(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object * value);


/************************************************************************
 ** Inserts a new item under the specified key. If an item already
 ** existed under that key, it is removed from the table and returned.
 **
 ** key is duplicated into a internal buffer. value is shared automatically.
 **
 ** @param key A pointer to a Jxta_object to be used as the key. May not be NULL.
 ** @param old_value if this pointer is non-NULL, and if there was a value
 ** previously associated with the given key, that value is returned to
 ** *old_value. Otherwise this pointer is not used.
 ** @return JXTA_SUCCESS or (rare) errors otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_objecthashtable_replace(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object * value, Jxta_object ** old_value);


/************************************************************************
 ** Finds and return the value associated with the specified key in the hash
 ** table. If no such item can be found, an error code is returned.
 ** The found value is shared automatically.
 **
 ** @param key Address of the data to be used as the key. May not be NULL.
 ** @param key_size the size of the data to be used as the key.
 ** @param found_value address where the found value is returned. If no
 ** item is found the data at this address is not affected. If NULL is passed
 ** then the value is not returned, but the result code is still valid.
 ** @return JXTA_SUCCESS if an item was found, JXTA_ITEM_NOTFOUND if it was
 ** not found, or (rare) errors otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_objecthashtable_get(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object ** found_value);


/************************************************************************
 ** Removes and returns the value associated with the specified key 
 ** If no such item can be found, an error code is returned.
 ** The found value is neithe shared nor released. After the call, this
 ** hash table is no-longer referencing the object, but the invoker is, so
 ** the reference cound is not changed.
 **
 ** @param key Address of the data to be used as the key. May not be NULL.
 ** @param key_size the size of the data to be used as the key.
 ** @param found_value address where the found value is returned. If no
 ** item is found the data at this address is not affected.
 ** @return JXTA_SUCCESS if an item was found and removed. An error
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_objecthashtable_del(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object ** found_value);

/************************************************************************
 ** Removes the value associated with the specified key only if that
 ** value is equal (by address comparison) to the given value.
 ** If no such item can be found, an error code is returned.
 **
 ** The removed value and associated key are released. (The key passed
 ** as a parameter is not).
 **
 ** @param key Address of the data to be used as the key. May not be NULL.
 ** @param key_size the size of the data to be used as the key.
 ** @param value value to be removed.
 ** @return JXTA_SUCCESS if an item was found and removed. JXTA_ITEM_NOTFOUND
 ** if the key was not found. JXTA_VIOLATION if the key was found associated to
 ** a different value.
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_objecthashtable_delcheck(Jxta_objecthashtable * self, Jxta_object * key, Jxta_object * value);


/************************************************************************
 ** Returns a vector of all the keys referenced by the given hashtable.
 ** The keys and returned vector are properly shared. So the only
 ** responsibility of the invoker upon return is to release the vector
 ** when no-longer needed. 
 ** If the needed memory cannot be allocated, NULL is returned.
 ** If the hash table is empty, an empty vector is returned, not NULL.
 **
 ** @param self the hashtable onto which to apply the operation.
 ** @return Jxta_vector* a pointer to a vector of all the values, or NULL
 ** if there is not enough memory to perform the operation.
 *************************************************************************/
JXTA_DECLARE(Jxta_vector *)
    jxta_objecthashtable_keys_get(Jxta_objecthashtable * self);

/************************************************************************
 ** Returns a vector of all the values referenced by the given hashtable.
 ** The values and returned vector are properly shared. So the only
 ** responsibility of the invoker upon return is to release the vector
 ** when no-longer needed. 
 ** If the needed memory cannot be allocated, NULL is returned.
 ** If the hash table is empty, an empty vector is returned, not NULL.
 **
 ** @param self the hashtable onto which to apply the operation.
 ** @return Jxta_vector* a pointer to a vector of all the values, or NULL
 ** if there is not enough memory to perform the operation.
 *************************************************************************/
JXTA_DECLARE(Jxta_vector *)
    jxta_objecthashtable_values_get(Jxta_objecthashtable * self);

/**
 ** Provides some statistics regarding the given hashtable.
 **
 ** @param self the hashtable of which statistics are wanted.
 ** @param size address where to return the total number of entries.
 ** @param usage address where to return the total number of active entries.
 ** @param occupancy address where to return the total number of occupied
 ** entries this include active entries and entries which where once used and
 ** then deleted. Once-used entries can be reused, but their presence does
 ** affect the need to re-hash more than never-used entries.
 ** @param max_occupancy address where to return the maximum value that
 ** occupancy may reach before a re-hash is needed.
 ** @param agv_hops address where to return a measure of the hashing
 ** effectiveness. This is a sliding average (very biased towards recent
 ** use) of the number of probes required to find a key, in addition to
 ** the initial one. A value of 0 means that keys are always found at their
 ** direct hash slot. A value of 1 means that keys are always found at the
 ** first alternate location, etc. A value of about 1 for an occupancy/size
 ** ratio of about 0.5 is the norm.
 */
JXTA_DECLARE(Jxta_status)
    jxta_objecthashtable_stats(Jxta_objecthashtable * self, size_t * size, size_t * usage,
                           size_t * occupancy, size_t * max_occupancy, double *avg_hops);

/************************************************************************
 ** A generic hash function in case you dont want to write your own. This
 ** function does a reasonable job of hashing random bindary data, but it
 ** may not generate as good a non-colliding value than specially written
 ** hash functions.
 **
 ** @param key the value to be hashed
 ** @param ksz size of the value to be hashed
 ** @return a hash value
 *************************************************************************/
JXTA_DECLARE(unsigned int) jxta_objecthashtable_simplehash(const void *key, size_t ksz);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_HASH_H */

/* vim: set sw=4 ts=4 et tw=130: */
