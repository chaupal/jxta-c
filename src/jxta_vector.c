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
 * $Id: jxta_vector.c,v 1.22 2005/04/07 02:16:58 bondolo Exp $
 */

static const char *__log_cat = "VECTOR";

#include <stddef.h>
#include <stdlib.h>

#include "jxtaapr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
/**********************************************************************
 ** This file implements Jxta_vector. Please refer to jxta_vector.h
 ** for detail on the API.
 **********************************************************************/
#define INDEX_DEFAULT_SIZE 32
#define INDEX_DEFAULT_INCREASE 16
struct _jxta_vector {
    JXTA_OBJECT_HANDLE;

    apr_pool_t *jpr_pool;
    apr_thread_mutex_t *mutex;

    unsigned int size;
    unsigned int capacity;

    Jxta_object **index;
};

/**
 * Free the vector
 *    - free the items in the vector
 *    - free the mutexes
 *    - free the vector object itself
 **/
static void jxta_vector_free(Jxta_object * vector)
{
    Jxta_vector *self = (Jxta_vector *) vector;
    unsigned int i = 0;

    /*
     * We need to take the lock in order to make sure that nobody
     * else is using the vector. However, since we are destroying
     * the object, there is no need to unlock. In other words, since
     * jxta_vector_free is not a public API, if the vector has been
     * properly shared, there should not be any external code that still
     * has a reference on this vector. So things should be safe.
     */
    apr_thread_mutex_lock(self->mutex);

    /* Release all the object contained in the vector */
    for (i = 0; i < self->size; ++i) {
        JXTA_OBJECT_RELEASE(self->index[i]);
    }
    /* Free the index */
    free(self->index);
    /* Free the pool containing the mutex */
    apr_pool_destroy(self->jpr_pool);
    /* Free the object itself */
    free(self);
}

Jxta_vector *jxta_vector_new(unsigned int initialSize)
{
    Jxta_vector *self;
    apr_status_t res;

    /* Create the vector object */
    self = (Jxta_vector *) calloc(1, sizeof(Jxta_vector));

    if (self == NULL) {
        /* No more memory */
        return NULL;
    }

    /* Initialize it. */
    JXTA_OBJECT_INIT(self, jxta_vector_free, NULL);

    /* Create the index */
    if (0 == initialSize) {
        /* Use the default value instead */
        self->capacity = INDEX_DEFAULT_SIZE;
    } else {
        self->capacity = initialSize;
    }
    self->index = (Jxta_object **) calloc(self->capacity, sizeof(Jxta_object *));

    self->size = 0;

    /*
     * Allocate the mutex 
     * Note that a pool is created for each vector. This allows to have a finer
     * granularity when allocated the memory. This constraint comes from the APR.
     */
    res = apr_pool_create(&self->jpr_pool, NULL);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        free(self->index);
        free(self);
        return NULL;
    }
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->jpr_pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->jpr_pool);
        free(self->index);
        free(self);
        return NULL;
    }

    return self;
}

/**
 * A new index needs to be created, increased by size.
 * The old index is copied into the new one.
 * The old index is freed.
 * The caller of this function must make sure to be holding the mutex.
 **/
static void increase_capacity(Jxta_vector * vector, unsigned int sizeIncrease)
{
    Jxta_object **oldIndex = vector->index;

    /* Allocate a new index */
    vector->capacity += sizeIncrease;
    vector->index = (Jxta_object **) calloc(vector->capacity, sizeof(Jxta_object *));

    /* Copy the old index into the new one */
    memmove(vector->index, oldIndex, vector->size * sizeof(Jxta_object *));

    /* Free the old index */
    free(oldIndex);
}

/**
 * Move the index up one slot starting at index.
 * The caller is responsible for holding the lock on the vector.
 **/
static void move_index_forward(Jxta_vector * vector, unsigned int at_index)
{
    /* Check if there is enough room in the index in order to add the object */
    if (vector->size == vector->capacity) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Increasing capacity\n");
        increase_capacity(vector, INDEX_DEFAULT_INCREASE);
    }

    if (at_index < vector->size) {
        memmove(&vector->index[at_index + 1], &vector->index[at_index], sizeof(Jxta_object *) * (vector->size - at_index));
    }

    vector->size++;

    vector->index[at_index] = NULL;
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
        memmove(&vector->index[at_index], &vector->index[at_index + 1], sizeof(Jxta_object *) * (vector->size - at_index));
    }

    vector->index[vector->size] = NULL;
}

/**
 * add_object inserts an object into a vector.
 * IMPORTANT: the caller is responsible for holding the lock on the 
 * vector.
 **/
static Jxta_status add_object_at(Jxta_vector * vector, Jxta_object * object, unsigned int at_index)
{
    /* Move the index to make room for the new object */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Move index forward\n");
    move_index_forward(vector, at_index);

    /* Share the object and add it into the index */
    vector->index[at_index] = JXTA_OBJECT_SHARE(object);

    return JXTA_SUCCESS;
}

Jxta_status jxta_vector_add_object_at(Jxta_vector * vector, Jxta_object * object, unsigned int at_index)
{
    Jxta_status err;

    if (!JXTA_OBJECT_CHECK_VALID(vector) || !JXTA_OBJECT_CHECK_VALID(object)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);
    if (at_index > vector->size) {
        apr_thread_mutex_unlock(vector->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    err = add_object_at(vector, object, at_index);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}

Jxta_status jxta_vector_add_object_first(Jxta_vector * vector, Jxta_object * object)
{
    Jxta_status err;

    if (!JXTA_OBJECT_CHECK_VALID(vector) || !JXTA_OBJECT_CHECK_VALID(object)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);
    err = add_object_at(vector, object, 0);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}

Jxta_status jxta_vector_add_object_last(Jxta_vector * vector, Jxta_object * object)
{
    Jxta_status err;

    if (!JXTA_OBJECT_CHECK_VALID(vector) || !JXTA_OBJECT_CHECK_VALID(object)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    apr_thread_mutex_lock(vector->mutex);
    err = add_object_at(vector, object, vector->size);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}

Jxta_status jxta_vector_get_object_at(Jxta_vector * vector, Jxta_object ** objectPt, unsigned int at_index)
{

    if (!JXTA_OBJECT_CHECK_VALID(vector) || (NULL == objectPt)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);

    if ((at_index >= vector->size) || (vector->size == 0)) {
        apr_thread_mutex_unlock(vector->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    *objectPt = vector->index[at_index];

    /* We need to share the object */
    JXTA_OBJECT_SHARE(*objectPt);

    apr_thread_mutex_unlock(vector->mutex);
    return JXTA_SUCCESS;
}

Jxta_status jxta_vector_remove_object_at(Jxta_vector * vector, Jxta_object ** objectPt, unsigned int at_index)
{
    Jxta_object *object;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);

    if (at_index >= vector->size) {
        apr_thread_mutex_unlock(vector->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    object = vector->index[at_index];
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

unsigned int jxta_vector_size(Jxta_vector * vector)
{
    unsigned int size;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return 0;
    }
    apr_thread_mutex_lock(vector->mutex);

    size = vector->size;
    apr_thread_mutex_unlock(vector->mutex);
    return size;
}

Jxta_status jxta_vector_clone(Jxta_vector * source, Jxta_vector ** dest, unsigned int at_index, unsigned int length)
{
    unsigned int i;
    Jxta_status err = JXTA_SUCCESS;

    if (!JXTA_OBJECT_CHECK_VALID(source) || (dest == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (length == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(source->mutex);

    length = ((at_index + length) > source->size) ? source->size - at_index : length;

    /* index cant be greater than size, except when length is zero, then we 
       let be equal to size */
    if ((at_index > source->size) || ((at_index == source->size) && (length != 0))) {
        err = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    /* Create a new vector */
    *dest = jxta_vector_new(length);

    if (*dest == NULL) {
        err = JXTA_NOMEM;
        goto Common_Exit;
    }

    /* Copy the elements */
    for (i = 0; i < length; ++i) {
        Jxta_object *pt;
        err = jxta_vector_get_object_at(source, &pt, at_index + i);
        if (err != JXTA_SUCCESS) {
            goto Common_Exit;
        }

        err = jxta_vector_add_object_last(*dest, pt);
        if (err != JXTA_SUCCESS) {
            JXTA_OBJECT_RELEASE(pt);
            pt = NULL;
            goto Common_Exit;
        }
        /* We don't care about the object anymore. We need to release it. */
        JXTA_OBJECT_RELEASE(pt);
    }

  Common_Exit:

    apr_thread_mutex_unlock(source->mutex);
    return err;
}

Jxta_status jxta_vector_clear(Jxta_vector * vector)
{
    unsigned int i;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);

    /* Release all the object contained in the vector */
    for (i = 0; i < vector->size; ++i) {
        JXTA_OBJECT_RELEASE(vector->index[i]);
    }
    vector->size = 0;

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}

/* default comparator function */
static int address_comparator(const void *a, const void *b)
{
    return ((char *) a) - ((char *) b);
} Jxta_status jxta_vector_qsort(Jxta_vector * vector, Jxta_object_compare_func * func)
{
    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL == func)
        func = (Jxta_object_compare_func *) address_comparator;

    apr_thread_mutex_lock(vector->mutex);

    qsort(vector->index, vector->size, sizeof(Jxta_object *), (int (*)(const void *a, const void *b)) func);

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif
