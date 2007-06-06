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
 * $Id: jxta_vector.c,v 1.39 2006/10/01 01:07:10 bondolo Exp $
 */

static const char *__log_cat = "VECTOR";

#include <stddef.h>
#include <stdlib.h>

#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_vector.h"
/**********************************************************************
 ** This file implements Jxta_vector. Please refer to jxta_vector.h
 ** for detail on the API.
 **********************************************************************/


static const unsigned int INDEX_DEFAULT_SIZE = 32;
static const unsigned int INDEX_DEFAULT_INCREASE = 16;

struct _jxta_vector {
    JXTA_OBJECT_HANDLE;

    apr_pool_t *jpr_pool;
    apr_thread_mutex_t *mutex;

    unsigned int size;
    unsigned int capacity;

    Jxta_object **elements;
};

static void jxta_vector_free(Jxta_object * vector);
static Jxta_status increase_capacity(Jxta_vector * vector, unsigned int additional);
static Jxta_status ensure_capacity(Jxta_vector * vector, unsigned int desired_capacity);
static Jxta_status move_index_forward(Jxta_vector * vector, unsigned int at_index, unsigned int num_slots);
static void move_index_backward(Jxta_vector * vector, unsigned int at_index);
static Jxta_status add_object_at(Jxta_vector * vector, Jxta_object * object, unsigned int at_index);
static Jxta_status addall_objects_at(Jxta_vector * vector, Jxta_vector * objects, unsigned int at_index, unsigned int from_index,
                                     unsigned int count);
static Jxta_boolean address_equator(const void *a, const void *b);
static int address_comparator(const void *a, const void *b);

/**
 * Free the vector
 *    - free the items in the vector
 *    - free the mutexes
 *    - free the vector object itself
 **/
static void jxta_vector_free(Jxta_object * me)
{
    Jxta_vector *myself = (Jxta_vector *) me;
    unsigned int i;

    /*
     * We need to take the lock in order to make sure that nobody
     * else is using the vector. However, since we are destroying
     * the object, there is no need to unlock. In other words, since
     * jxta_vector_free is not a public API, if the vector has been
     * properly shared, there should not be any external code that still
     * has a reference on this vector. So things should be safe.
     */
    apr_thread_mutex_lock(myself->mutex);

    /* Release all the object contained in the vector */
    for (i = 0; i < myself->size; ++i) {
        JXTA_OBJECT_RELEASE(myself->elements[i]);
        myself->elements[i] = NULL;
    }

    /* Free the elements */
    free(myself->elements);

    apr_thread_mutex_unlock(myself->mutex);
    apr_thread_mutex_destroy(myself->mutex);
    /* Free the pool containing the mutex */
    apr_pool_destroy(myself->jpr_pool);

    memset(myself, 0xDD, sizeof(Jxta_vector));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FREE [%pp]\n", myself);

    free(myself);
}

JXTA_DECLARE(Jxta_vector *) jxta_vector_new(unsigned int initialSize)
{
    Jxta_vector *self;
    apr_status_t res;

    /* Create the vector object */
    self = (Jxta_vector *) calloc(1, sizeof(Jxta_vector));

    if (self == NULL) {
        /* No more memory */
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "NEW [%pp]\n", self);

    /* Initialize it. */
    JXTA_OBJECT_INIT(self, jxta_vector_free, NULL);

    /* Create the elements */
    if (0 == initialSize) {
        /* Use the default value instead */
        self->capacity = INDEX_DEFAULT_SIZE;
    } else {
        self->capacity = initialSize;
    }
    self->elements = (Jxta_object **) calloc(self->capacity, sizeof(Jxta_object *));

    self->size = 0;

    /*
     * Allocate the mutex 
     * Note that a pool is created for each vector. This allows to have a finer
     * granularity when allocated the memory. This constraint comes from the APR.
     */
    res = apr_pool_create(&self->jpr_pool, NULL);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        free(self->elements);
        free(self);
        return NULL;
    }

    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->jpr_pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->jpr_pool);
        free(self->elements);
        free(self);
        return NULL;
    }

    return self;
}


/**
 * A new index needs to be created, increased by size.
 * The old elements are copied into a larger block.
 * The caller of this function must make sure to be holding the mutex.
 **/
static Jxta_status ensure_capacity(Jxta_vector * vector, unsigned int desired_capacity)
{
    if (vector->capacity < desired_capacity) {
        unsigned int additional = desired_capacity - vector->capacity;

        if (additional < INDEX_DEFAULT_INCREASE) {
            additional = INDEX_DEFAULT_INCREASE;
        }

        return increase_capacity(vector, additional);
    }

    return JXTA_SUCCESS;
}


/**
 * A new index needs to be created, increased by size.
 * The old elements are copied into a larger block.
 * The caller of this function must make sure to be holding the mutex.
 **/
static Jxta_status increase_capacity(Jxta_vector * vector, unsigned int additional)
{
    Jxta_object **new_elements;

    if ((vector->capacity + additional) < vector->capacity) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Capacity would overflow : JXTA_INVALID_ARGUMENT\n");
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Increasing capacity : %d -> %d\n", vector->capacity,
                    vector->capacity + additional);

    new_elements = (Jxta_object **) calloc(vector->capacity + additional, sizeof(Jxta_object *));

    if (NULL == new_elements) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Increase capacity failed : JXTA_NOMEM\n");
        return JXTA_NOMEM;
    }

    vector->capacity += additional;

    /* Copy the old elements into the new one */
    memmove(new_elements, vector->elements, vector->size * sizeof(Jxta_object *));

    /* Free the old elements. */
    free(vector->elements);

    /* Assign the new elements. */
    vector->elements = new_elements;

    return JXTA_SUCCESS;
}

/**
 * Move the index up one slot starting at index.
 * The caller is responsible for holding the lock on the vector.
 **/
static Jxta_status move_index_forward(Jxta_vector * vector, unsigned int at_index, unsigned int num_slots)
{
    Jxta_status res;

    /* Check if there is enough room in the index in order to add the object */
    res = ensure_capacity(vector, vector->size + num_slots);

    if (JXTA_SUCCESS != res) {
        return res;
    }

    if (at_index < vector->size) {
        memmove(&vector->elements[at_index + num_slots], &vector->elements[at_index],
                sizeof(Jxta_object *) * (vector->size - at_index));
    }

    vector->size += num_slots;

    memset(&vector->elements[at_index], 0x00, sizeof(Jxta_object *) * num_slots);

    return JXTA_SUCCESS;
}

/**
 * Move the index down one slot starting at index.
 * The caller is responsible for holding the lock on the vector.
 **/
static void move_index_backward(Jxta_vector * vector, unsigned int at_index)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Move index backward\n");
    vector->size--;

    if (at_index < vector->size) {
        memmove(&vector->elements[at_index], &vector->elements[at_index + 1], sizeof(Jxta_object *) * (vector->size - at_index));
    }

    vector->elements[vector->size] = NULL;
}

/**
 * add_object inserts an object into a vector.
 *
 * IMPORTANT: the caller is responsible for holding the lock on the 
 * vector.
 **/
static Jxta_status add_object_at(Jxta_vector * vector, Jxta_object * object, unsigned int at_index)
{
    /* Move the index to make room for the new object */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Move index forward\n");
    move_index_forward(vector, at_index, 1);

    /* Share the object and add it into the index */
    vector->elements[at_index] = JXTA_OBJECT_SHARE(object);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_vector_add_object_at(Jxta_vector * vector, Jxta_object * object, unsigned int at_index)
{
    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);
    if (at_index > vector->size) {
        apr_thread_mutex_unlock(vector->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument [%pp]\n", vector);
        return JXTA_INVALID_ARGUMENT;
    }

    err = add_object_at(vector, object, at_index);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}

JXTA_DECLARE(Jxta_status) jxta_vector_add_object_first(Jxta_vector * vector, Jxta_object * object)
{
    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);
    err = add_object_at(vector, object, 0);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}

JXTA_DECLARE(Jxta_status) jxta_vector_add_object_last(Jxta_vector * vector, Jxta_object * object)
{
    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);
    err = add_object_at(vector, object, vector->size);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}

/**
 * add_object inserts an object into a vector.
 *
 * IMPORTANT: the caller is responsible for holding the lock on the 
 * vector.
 **/
static Jxta_status addall_objects_at(Jxta_vector * vector, Jxta_vector * objects, unsigned int at_index, unsigned int from_index,
                                     unsigned int count)
{
    unsigned int each_object;

    if (vector == objects) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                        FILEANDLINE "Invalid argument (sorry, source == destination not allowed)\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* Move the index to make room for the new object */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Move index forward\n");
    move_index_forward(vector, at_index, count);

    /* Share the object and add it into the index */
    for (each_object = 0; each_object < count; each_object++) {
        vector->elements[at_index + each_object] = JXTA_OBJECT_SHARE(objects->elements[from_index + each_object]);
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_vector_addall_objects_at(Jxta_vector * vector, Jxta_vector * objects, unsigned int at_index)
{
    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(objects);

    apr_thread_mutex_lock(vector->mutex);
    if (at_index > vector->size) {
        apr_thread_mutex_unlock(vector->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument [%pp]\n", vector);
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(objects->mutex);
    err = addall_objects_at(vector, objects, at_index, 0, objects->size);
    apr_thread_mutex_unlock(objects->mutex);
    apr_thread_mutex_unlock(vector->mutex);

    return err;
}

JXTA_DECLARE(Jxta_status) jxta_vector_addall_objects_first(Jxta_vector * vector, Jxta_vector * objects)
{
    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(objects);

    apr_thread_mutex_lock(vector->mutex);
    apr_thread_mutex_lock(objects->mutex);

    err = addall_objects_at(vector, objects, 0, 0, objects->size);

    apr_thread_mutex_unlock(objects->mutex);
    apr_thread_mutex_unlock(vector->mutex);

    return err;
}

JXTA_DECLARE(Jxta_status) jxta_vector_addall_objects_last(Jxta_vector * vector, Jxta_vector * objects)
{
    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(objects);

    apr_thread_mutex_lock(vector->mutex);
    apr_thread_mutex_lock(objects->mutex);

    err = addall_objects_at(vector, objects, vector->size, 0, objects->size);

    apr_thread_mutex_unlock(objects->mutex);
    apr_thread_mutex_unlock(vector->mutex);

    return err;
}

JXTA_DECLARE(Jxta_status) jxta_vector_get_object_at(Jxta_vector * vector, Jxta_object ** objectPt, unsigned int at_index)
{
    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    if ((at_index >= vector->size) || (vector->size == 0)) {
        apr_thread_mutex_unlock(vector->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument [%pp]\n", vector);
        return JXTA_INVALID_ARGUMENT;
    }

    *objectPt = vector->elements[at_index];

    /* We need to share the object */
    JXTA_OBJECT_SHARE(*objectPt);

    apr_thread_mutex_unlock(vector->mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(int) jxta_vector_remove_object(Jxta_vector * me, Jxta_object * object)
{
    int num_removed = 0;
    unsigned int i = 0;

    JXTA_OBJECT_CHECK_VALID(me);

    apr_thread_mutex_lock(me->mutex);
    while (i < me->size) {
        if (object == me->elements[i]) {
            ++num_removed;
            move_index_backward(me, i);
            JXTA_OBJECT_RELEASE(object);
        } else {
            ++i;
        }
    }
    apr_thread_mutex_unlock(me->mutex);

    return num_removed;
}

JXTA_DECLARE(Jxta_status) jxta_vector_remove_object_at(Jxta_vector * vector, Jxta_object ** objectPt, unsigned int at_index)
{
    Jxta_object *object;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    if (at_index >= vector->size) {
        apr_thread_mutex_unlock(vector->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument [%pp]\n", vector);
        return JXTA_INVALID_ARGUMENT;
    }

    object = vector->elements[at_index];
    move_index_backward(vector, at_index);
    apr_thread_mutex_unlock(vector->mutex);

    if (objectPt != NULL) {
    /** The caller wants to get a reference to the object
     ** that is removed. In that case, the caller is responsible
     ** for releasing the object. */

        *objectPt = object;
    } else {
        /* We need to release the object */
        JXTA_OBJECT_RELEASE(object);
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(unsigned int) jxta_vector_size(Jxta_vector * vector)
{
    unsigned int size;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    size = vector->size;
    apr_thread_mutex_unlock(vector->mutex);
    return size;
}

JXTA_DECLARE(Jxta_status) jxta_vector_clone(Jxta_vector * source, Jxta_vector ** dest, unsigned int at_index, unsigned int length)
{
    Jxta_status err = JXTA_SUCCESS;

    JXTA_OBJECT_CHECK_VALID(source);

    apr_thread_mutex_lock(source->mutex);

    length = ((at_index + length) > source->size) ? source->size - at_index : length;

    /* index cant be greater than size, except when length is zero, then we 
       let be equal to size */
    if ((at_index > source->size) || ((at_index == source->size) && (length != 0))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument [%pp]\n", source);
        err = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    /* Create a new vector */
    *dest = jxta_vector_new(length);

    if (*dest == NULL) {
        err = JXTA_NOMEM;
        goto Common_Exit;
    }

    err = addall_objects_at(*dest, source, 0, at_index, length);

  Common_Exit:

    apr_thread_mutex_unlock(source->mutex);
    return err;
}

JXTA_DECLARE(Jxta_status) jxta_vector_clear(Jxta_vector * vector)
{
    unsigned int i;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    /* Release all the object contained in the vector */
    for (i = 0; i < vector->size; ++i) {
        JXTA_OBJECT_RELEASE(vector->elements[i]);
    }
    vector->size = 0;

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}

/* default comparator function */
static int address_comparator(const void *a, const void *b)
{
    return ((char *) a) - ((char *) b);
}

static Jxta_boolean address_equator(const void *a, const void *b)
{
    return a == b;
}

JXTA_DECLARE(Jxta_status) jxta_vector_swap_elements(Jxta_vector * vector, unsigned int first, unsigned int second)
{

    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    if ((first >= vector->size) || (second >= vector->size)) {
        res = JXTA_INVALID_ARGUMENT;
    } else {
        Jxta_object *temp = vector->elements[first];
        vector->elements[first] = vector->elements[second];
        vector->elements[second] = temp;

        res = JXTA_SUCCESS;
    }

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_vector_move_element_first(Jxta_vector * vector, unsigned int to_first)
{

    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    if (to_first >= vector->size) {
        res = JXTA_INVALID_ARGUMENT;
    } else if (to_first > 0) {
        Jxta_object *temp = vector->elements[to_first];

        memmove(&vector->elements[1], &vector->elements[0], sizeof(Jxta_object *) * to_first);

        vector->elements[0] = temp;

        res = JXTA_SUCCESS;
    } else {
        /* already in the right spot */
        res = JXTA_SUCCESS;
    }

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_vector_move_element_last(Jxta_vector * vector, unsigned int to_last)
{

    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    if (to_last >= vector->size) {
        res = JXTA_INVALID_ARGUMENT;
    } else if (to_last != (vector->size - 1)) {
        Jxta_object *temp = vector->elements[to_last];

        memmove(&vector->elements[to_last], &vector->elements[to_last + 1], sizeof(Jxta_object *) * (vector->size - to_last - 1));

        vector->elements[vector->size - 1] = temp;

        res = JXTA_SUCCESS;
    } else {
        /* already in the right spot */
        res = JXTA_SUCCESS;
    }

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_boolean) jxta_vector_contains(Jxta_vector * vector, Jxta_object * object, Jxta_object_equals_func func)
{
    unsigned int each_object;
    Jxta_boolean found = FALSE;

    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(object);

    if (NULL == func) {
        func = (Jxta_object_equals_func) address_equator;
    }

    apr_thread_mutex_lock(vector->mutex);

    for (each_object = 0; each_object < vector->size; each_object++) {
        if ((func) (vector->elements[each_object], object)) {
            found = TRUE;
            break;
        }
    }

    apr_thread_mutex_unlock(vector->mutex);

    return found;
}

JXTA_DECLARE(Jxta_status) jxta_vector_qsort(Jxta_vector * vector, Jxta_object_compare_func func)
{
    JXTA_OBJECT_CHECK_VALID(vector);

    if (NULL == func) {
        func = (Jxta_object_compare_func) address_comparator;
    }

    apr_thread_mutex_lock(vector->mutex);

    qsort(vector->elements, vector->size, sizeof(Jxta_object *), (int (*)(const void *a, const void *b)) func);

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}

/**
* Shuffle adapted from  : <http://benpfaff.org/writings/clc/shuffle.html>
* Copyright  2004 Ben Pfaff.
*
* <p/>This function assumes that srand() has been called with good seed
* material. See <http://benpfaff.org/writings/clc/random-seed.html> for good 
* suggestions.
**/
JXTA_DECLARE(void) jxta_vector_shuffle(Jxta_vector * vector)
{
    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(vector->mutex);

    if (vector->size > 1) {
        size_t each;
        for (each = 0; each < vector->size - 1; each++) {
            size_t swap = each + rand() / (RAND_MAX / (vector->size - each) + 1);
            Jxta_object *temp = vector->elements[swap];
            vector->elements[swap] = vector->elements[each];
            vector->elements[each] = temp;
        }
    }

    apr_thread_mutex_unlock(vector->mutex);
}

/* vim: set ts=4 sw=4 et tw=130: */
