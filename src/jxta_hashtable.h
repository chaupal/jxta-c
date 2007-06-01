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
 * $Id: jxta_hashtable.h,v 1.10 2006/02/01 19:01:15 bondolo Exp $
 */


/**
 *  Jxta_hashtable
 *
 * A Jxta_hashtable is a Jxta_object that manages a hash table of Jxta_objects.
 *
 * Sharing and releasing of Jxta_object is properly done (refer to the API of Jxta_object).
 *
 **/

#ifndef JXTA_HASHTABLE_H
#define JXTA_HASHTABLE_H

#include "jxta_object.h"
#include "jxta_vector.h"


#ifdef __cplusplus
extern "C" {
/* avoid confusing indenters */
#if 0
};
#endif
#endif

/**
 * A Jxta_hashtable is a Jxta_object that manages a hash table of Jxta_objects.
 **/
typedef struct _jxta_hashtable Jxta_hashtable;


/**
 * Allocates a new Hash Table. The table is allocated and is initialized (ready to use). No need to apply JXTA_OBJECT_INIT, 
 * because new does it. The table will handle mutual exclusion automatically all operations are atomic.
 *
 * If that value is 0, then a default size is used.
 *
 * The creator of a hash table is responsible to release it when not used anymore.
 *
 * @param initial_size The initial number of items that can go in the table. 0 means default. Until this number of items is 
 * reached, the table will not be enlarged. The size of the underlying table may be bigger, depending on the implementation.
 * @return A new hash table, or NULL if allocation failed.
 **/
JXTA_DECLARE(Jxta_hashtable *) jxta_hashtable_new(size_t initial_size);


/**
 * Allocates a new Hash Table. The table is allocated and is initialized (ready to use). No need to apply JXTA_OBJECT_INIT, 
 * because new does it. jxta_hashtable_new_0 creates a Jxta_hashtable that is synchronized or not, dependent on the value of the 
 * synchronized flag. A non-synchronized table does not take any procaution to prevent concurrrent access. This is suitable when
 * concurrent access is better controlled externaly, such as if the table is part of larger structure which already needs mutual 
 * exclusion or if the table can never be accessed concurrently.
 *
 * The creator of a hash table is responsible to release it when not used anymore.
 *
 * @param initial_size The initial number of items that can go in the  table. 0 means default.
 * Until this number of items is reached, the table will not be enlarged. The size of the underlying table may be bigger,  
 * depending on the implementation.
 * @param synchronized If TRUE the table handles mutual exclusion by itself if FALSE, the table assumes it is never used
 * concurrently.
 * @return A new hash table, or NULL if allocation failed.
 **/
JXTA_DECLARE(Jxta_hashtable *) jxta_hashtable_new_0(size_t initial_size, Jxta_boolean synchronized);


/**
 * Clears the hashtable and in the process release all entries.
 *
 * @param self The hashtable onto which to apply the operation.
 **/
JXTA_DECLARE(void) jxta_hashtable_clear(Jxta_hashtable * self);


/**
 * Inserts a new item under the specified key. If an item already existed under that key, it is removed from the table.
 * key is duplicated into a internal buffer. value is shared automatically.
 *
 * @param self The hashtable onto which to apply the operation.
 * @param key Address of the data to be used as the key. May not be NULL.
 * @param key_size the size of the data to be used as the key.
 * @param value A pointer to a Jxta_object to be associated with the key
 * @return nothing. This method never fails.
 **/
JXTA_DECLARE(void) jxta_hashtable_put(Jxta_hashtable * self, const void *key, size_t keySize, Jxta_object * value);


/**
 * Inserts a new item under the specified key. If an item already existed under that key, insertiong is not performed.
 * key is duplicated into a internal buffer. value is shared automatically.
 *
 * @param self The hashtable onto which to apply the operation.
 * @param key Address of the data to be used as the key. May not be NULL.
 * @param key_size the size of the data to be used as the key.
 * @param value A pointer to a Jxta_object to be associated with the key
 * @return Jxta_boolean TRUE if the item could be inserted, FALSE if there
 * was already an item registered under the given key.
 **/
JXTA_DECLARE(Jxta_boolean) jxta_hashtable_putnoreplace(Jxta_hashtable * self, const void *key, size_t keySize,
                                                       Jxta_object * value);


/**
 * Inserts a new item under the specified key. If an item already existed under that key, it is removed from the table and
 * returned.
 *
 * key is duplicated into a internal buffer. value is shared automatically.
 *
 * @param self The hashtable onto which to apply the operation.
 * @param key Address of the data to be used as the key. May not be NULL.
 * @param key_size the size of the data to be used as the key.
 * @param old_value if this pointer is non NULL, and if there was a value
 * previously associated with the given key, that value is returned to
 * *old_value. Otherwise this pointer is not used and the object is released.
 * @param value A pointer to a Jxta_object to be associated with the key
 * @return nothing. This method never fails.
 **/
JXTA_DECLARE(void)
jxta_hashtable_replace(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object * value, Jxta_object ** old_value);


/**
 * Finds and return the value associated with the specified key in the hash table. If no such item can be found, an error code is
 * returned. The found value is shared automatically.
 *
 * @param self The hashtable onto which to apply the operation.
 * @param key Address of the data to be used as the key. May not be NULL.
 * @param key_size the size of the data to be used as the key.
 * @param found_value address where the found value is returned. If no
 * item is found the data at this address is not affected.
 * @return JXTA_SUCCESS if an item was found. An error otherwise.
 **/
JXTA_DECLARE(Jxta_status) jxta_hashtable_get(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object ** found_value);


/**
 * Removes and returns the value associated with the specified key. If no such item can be found, an error code is returned.
 * The found value is neither shared nor released. After the call, this hash table is no-longer referencing the object, but the 
 * invoker is, so the reference count is not changed.
 *
 * @param self The hashtable onto which to apply the operation.
 * @param key Address of the data to be used as the key. May not be NULL.
 * @param key_size the size of the data to be used as the key.
 * @param found_value address where the found value is returned. If no
 * item is found the data at this address is not affected.
 * @return JXTA_SUCCESS if an item was found and removed. An error
 * otherwise.
 **/
JXTA_DECLARE(Jxta_status) jxta_hashtable_del(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object ** found_value);


/**
 * Removes the value associated with the specified key only if that value is equal (by address comparison) to the given value.
 * If no such item can be found, an error code is returned.
 *
 * The removed value is optionally released or returned and the associated key is freed (the key passed as a parameter is not freed).
 *
 * @param self The hashtable onto which to apply the operation.
 * @param key Address of the data to be used as the key. May not be NULL.
 * @param key_size the size of the data to be used as the key.
 * @param value The value which was removed. If NULL then the object is released.
 * @return JXTA_SUCCESS if an item was found and removed. JXTA_ITEM_NOTFOUND
 * if the key was not found. JXTA_VIOLATION if the key was found associated to
 * a different value.
 **/
JXTA_DECLARE(Jxta_status) jxta_hashtable_delcheck(Jxta_hashtable * self, const void *key, size_t key_size, Jxta_object * value);


/**
 * Returns a vector of all the values referenced by the given hashtable. The values and returned vector are properly shared. So
 * the only responsibility of the invoker upon return is to release the vector  when no-longer needed. If the needed memory 
 * cannot be allocated, NULL is returned. If the hash table is empty, an empty vector is returned, not NULL.
 *
 * @param self The hashtable onto which to apply the operation.
 * @return Jxta_vector* a pointer to a vector of all the values, or NULL
 * if there is not enough memory to perform the operation.
 **/
JXTA_DECLARE(Jxta_vector *) jxta_hashtable_values_get(Jxta_hashtable * self);


/**
 * Returns all the keys used by this hashtable as a null-terminated array of char*. Each char* in the table except the final NULL 
 * points at a null-terminated string. 
 *
 * The memory holding that table and the strings is allocated by this method and given away to the caller. It is the caller's 
 * responsibility to free it.
 *
 * If the needed memory cannot be allocated, NULL is returned. If memory for one or the other of the keys could not be allocated
 * this key is not included in the table. If the final table is empty, a table containing only the final NULL pointer
 * is returned, not just NULL.
 *
 * Note: Jxta_hashtable accepts keys as a pointer and a size; therefore keys are not necessarily null-terminated. This routine 
 * will append a final zero to returned keys so that string keys are properly rendered, but it does not return a size; therefore,
 * if invoking code cannot figure-out the size of the keys (such as is the case for strings), then it cannot make use of this 
 * interface.
 *
 * @param self The hashtable onto which to apply the operation.
 * @return char** a pointer to a null terminated array of char**.
 **/
JXTA_DECLARE(char **) jxta_hashtable_keys_get(Jxta_hashtable * self);


/**
 * Provides some statistics regarding the hashtable.
 *
 * @param self The hashtable for this operation. 
 * @param capacity Will contain the total number of slots in the hashtable. If NULL then the value will not be stored. 
 * @param usage Will contain the current number of items stored in the hashtable. If NULL then the value will not be stored. 
 * @param occupancy address Will contain the total number of occupied entries. This includes active entries and entries which 
 * where once used and then deleted. Once-used entries can be reused, but their presence does affect the need to re-hash more 
 * than never-used entries.  If NULL then the value will not be stored. 
 * @param max_occupancy address Will contain the maximum value that occupancy may reach before a re-hash is needed. If NULL 
 * then the value will not be stored. 
 * @param agv_hops  Will contain the a measure of the hashing effectiveness. This is a sliding average (very biased towards 
 * recent use) of the number of extra probes beyond the first probe required to find a key. A value of 0 means that keys are 
 * always found at their direct hash slot. A value of 1 means that keys are always found at the first alternate location, etc.
 * A value of about 1 for an occupancy/size ratio of about 0.5 is the norm. If NULL then the value will not be stored.
 **/
JXTA_DECLARE(void)
jxta_hashtable_stats(Jxta_hashtable * self, size_t * capacity, size_t * usage,
                     size_t * occupancy, size_t * max_occupancy, double *avg_hops);
#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_HASH_H */

/* vi: set ts=4 sw=4 tw=130 et: */
