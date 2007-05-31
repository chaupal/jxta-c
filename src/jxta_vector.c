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
 * $Id: jxta_vector.c,v 1.20 2005/02/16 23:23:49 bondolo Exp $
 */

#include <stdlib.h>

#include "jxtaapr.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_vector.h"

/**********************************************************************
 ** This file implements Jxta_vector. Please refer to jxta_vector.h
 ** for detail on the API.
 **********************************************************************/

#define INDEX_DEFAULT_SIZE 50
#define INDEX_DEFAULT_INCREASE 20

struct _jxta_vector {
    JXTA_OBJECT_HANDLE;
    Jxta_object **index;
    int size;
    int capacity;
    apr_thread_mutex_t *mutex;
    apr_pool_t *jpr_pool;
};



/**
 ** Free the shared dlist (invoked when the shared dlist is no longer used by anyone.
 **    - release the objects the shared list contains
 **    - free the dlist
 **    - free the mutexes
 **    - free the shared dlist object itself
 **/
static void jxta_vector_free(Jxta_object * vector)
{
    Jxta_vector *self = (Jxta_vector *) vector;
    int i = 0;

    if (self == NULL) {
        return;
    }

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

/************************************************************************
 ** Allocates a new Vector. The vector is allocated and is initialized
 ** (ready to use). No need to apply JXTA_OBJECT_INIT, because new does it.
 ** The initial size of the vector is set by initialSize. If that value is
 ** null, then a default size is used.
 **
 ** The creator of a vector is responsible to release it when not used
 ** anymore.
 **
 ** @param initialSize is the initial size of the vector. 0 means default.
 ** @return a new vector, or NULL if allocation failed.
 *************************************************************************/

Jxta_vector *jxta_vector_new(int initialSize)
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
    if (initialSize <= 0) {
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
 ** A new index needs to be created, increased by size.
 ** The old index is copied into the new one.
 ** The old index is freed.
 ** The caller of this function must make sure to be holding the mutex.
 **/
static void increase_capacity(Jxta_vector * vector, int sizeIncrease)
{
    Jxta_object **oldIndex = vector->index;

    JXTA_OBJECT_CHECK_VALID(vector);

    /* Allocate a new index */
    vector->capacity += sizeIncrease;
    vector->index = (Jxta_object **) malloc(sizeof(Jxta_object *) * vector->capacity);
    memset(vector->index, 0, sizeof(Jxta_object *) * vector->capacity);
    /* Copy the old index into the new one */
    memcpy(vector->index, oldIndex, vector->size * sizeof(Jxta_object *));
    /* Free the old index */
    free(oldIndex);
}

/**
 ** Move the index up on slot starting at index.
 ** The caller is responsible for holding the lock on the vector.
 **/
static void move_index_forward(Jxta_vector * vector, int index)
{
    int i;

    JXTA_OBJECT_CHECK_VALID(vector);

    for (i = (vector->size - 1); i >= index; --i) {
        vector->index[i + 1] = vector->index[i];
    }
}

/**
 ** Move the index down on slot starting at index.
 ** The caller is responsible for holding the lock on the vector.
 **/
static void move_index_backward(Jxta_vector * vector, int index)
{
    int i;

    JXTA_OBJECT_CHECK_VALID(vector);

    for (i = index + 1; i < vector->size; ++i) {
        vector->index[i - 1] = vector->index[i];
    }
    vector->index[i] = NULL;
}


/**
 ** add_object inserts an object into a vector.
 ** IMPORTANT: the caller is responsible for holding the lock on the 
 ** vector.
 **/
static Jxta_status add_object_at(Jxta_vector * vector, Jxta_object * object, int index)
{
    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(object);

    if (index > vector->size) {
        JXTA_LOG("index %d is out of bound (size of vector= %d)\n", index, vector->size);
        return JXTA_INVALID_ARGUMENT;
    }

    /* Check if there is enough room in the index in order to add the object */
    if (vector->size == vector->capacity) {
        JXTA_LOG("Increase capacity\n");
        increase_capacity(vector, INDEX_DEFAULT_INCREASE);
    }

    if (index < vector->size) {
        /* Move the index to make room for the new object */
        JXTA_LOG("Move index forward\n");
        move_index_forward(vector, index);
    }

    /* Share the object */
    JXTA_OBJECT_SHARE(object);

    /* Add it into the index */
    vector->index[index] = object;

    /* Increase the size */
    vector->size++;
    return JXTA_SUCCESS;
}


/************************************************************************
 ** Add an object at a particular index. Object with higher index are
 ** moved up. The size of the vector is increased by one.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to the Jxta_object to add.
 ** @index where to add the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_vector_add_object_at(Jxta_vector * vector, Jxta_object * object, int index)
{

    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(object);

    if ((vector == NULL) || (object == NULL)) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);
    if ((index < 0) || (index > vector->size)) {
        apr_thread_mutex_unlock(vector->mutex);
        return JXTA_INVALID_ARGUMENT;
    }

    err = add_object_at(vector, object, index);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}


/************************************************************************
 ** Add an object at the beginning of the vector. Objects already in the
 ** vector are moved up. The size of the vector is increased by one.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to the Jxta_object to add.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

Jxta_status jxta_vector_add_object_first(Jxta_vector * vector, Jxta_object * object)
{

    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(object);

    if ((vector == NULL) || (object == NULL)) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);
    err = add_object_at(vector, object, 0);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}


/************************************************************************
 ** Add an object at the end of the vector. Objects already in the
 ** vector are moved up. The size of the vector is increased by one.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to the Jxta_object to add.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

Jxta_status jxta_vector_add_object_last(Jxta_vector * vector, Jxta_object * object)
{
    Jxta_status err;

    JXTA_OBJECT_CHECK_VALID(vector);
    JXTA_OBJECT_CHECK_VALID(object);

    if ((vector == NULL) || (object == NULL)) {
        JXTA_LOG("Invalid argumenent\n");
        return JXTA_INVALID_ARGUMENT;
    }
    apr_thread_mutex_lock(vector->mutex);
    err = add_object_at(vector, object, vector->size);
    apr_thread_mutex_unlock(vector->mutex);
    return err;
}

/************************************************************************
 ** Get the object which is at the given index. The object is shared
 ** before being returned: the caller is responsible for releasing the
 ** object when it is no longer used.
 ** The object is not removed from the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to a Jxta_object pointer which will contain the
 ** the object.
 ** @index index of the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

Jxta_status jxta_vector_get_object_at(Jxta_vector * vector, Jxta_object ** objectPt, int index)
{

    JXTA_OBJECT_CHECK_VALID(vector);

    if (vector == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);

    if ((index < 0) || (index >= vector->size) || (vector->size == 0)) {
        apr_thread_mutex_unlock(vector->mutex);
        return JXTA_INVALID_ARGUMENT;
    }

    *objectPt = vector->index[index];

    /* We need to share the object */
    JXTA_OBJECT_SHARE(*objectPt);

    apr_thread_mutex_unlock(vector->mutex);
    return JXTA_SUCCESS;
}

/************************************************************************
 ** Remove the object which is at the given index.
 ** If the callers set objectPt to a non nul pointer to a Jxta_object
 ** pointer, this pointer will contained the removed object. In that case,
 ** the caller is responsible for releasing the object when it is not used
 ** anymore. If objectPt is set to NULL, then the object is automatically
 ** released.
 **
 ** The size of the vector is decreased.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to a Jxta_object pointer which will contain the
 ** the removed object.
 ** @index index of the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

Jxta_status jxta_vector_remove_object_at(Jxta_vector * vector, Jxta_object ** objectPt, int index)
{

    Jxta_object *object;

    JXTA_OBJECT_CHECK_VALID(vector);

    if (vector == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(vector->mutex);

    if ((index < 0) || (index >= vector->size)) {
        apr_thread_mutex_unlock(vector->mutex);
        return JXTA_INVALID_ARGUMENT;
    }

    object = vector->index[index];
    if (objectPt != NULL) {
    /** The caller wants to get a reference to the object
     ** that is removed. In that case, the caller is responsible
     ** for releasing the object. */

        *objectPt = object;
    } else {
        /* We need to release the object */
        JXTA_OBJECT_RELEASE(object);
    }
    move_index_backward(vector, index);
    vector->size--;
    apr_thread_mutex_unlock(vector->mutex);
    return JXTA_SUCCESS;
}

/************************************************************************
 ** Returns the number of objects in the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @return the number of objects in the vector.
 *************************************************************************/

int jxta_vector_size(Jxta_vector * vector)
{

    int size;

    JXTA_OBJECT_CHECK_VALID(vector);

    if (vector == NULL) {
        return 0;
    }
    apr_thread_mutex_lock(vector->mutex);

    size = vector->size;
    apr_thread_mutex_unlock(vector->mutex);
    return size;
}

/************************************************************************
 ** Duplicates a vector, but not its contents: a new vector object is
 ** created, with a reference to all the objects that are in the original
 ** object. (shallow clone) Note that the objects are automatically shared.
 ** 
 ** Using index and length arguments, a sub section of a vector can
 ** be cloned instead of the totality of the vector
 **
 ** @param source a pointer to the original vector
 ** @param dest a pointer to a Jxta_vector pointer which will contain the
 ** the cloned vector.
 ** @index index start index
 ** @length number of objects, starting at the given index to clone. if 
 **  length is larger than the available number of elements, then only the
 **  available elements will be returned.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

Jxta_status jxta_vector_clone(Jxta_vector * source, Jxta_vector ** dest, int index, int length)
{

    int i;
    Jxta_status err = JXTA_SUCCESS;

    JXTA_OBJECT_CHECK_VALID(source);

    if (!JXTA_OBJECT_CHECK_VALID(source) || (dest == NULL)) {
        return JXTA_INVALID_ARGUMENT;
    }

    if ((index < 0) || (length < 0))
        return JXTA_INVALID_ARGUMENT;

    apr_thread_mutex_lock(source->mutex);

    length = ((index + length) > source->size) ? source->size - index : length;

    /* index cant be greater than size, except when length is zero, then we 
       let be equal to size */
    if ((index > source->size) || ((index == source->size) && (length != 0))) {
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
        err = jxta_vector_get_object_at(source, &pt, index + i);
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

/*************************************************************************
 ** Removes all of the elements from a vector. All of the objects are
 ** formerly in the vector are unshared.
 **
 ** @param vector the vector to clear.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 ************************************************************************/
Jxta_status jxta_vector_clear(Jxta_vector * vector)
{

    int i;

    if (!JXTA_OBJECT_CHECK_VALID(vector))
        return JXTA_INVALID_ARGUMENT;

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
}

/*************************************************************************
 ** Sorts a vector using the provided comparator function.
 **
 ** @param vector   the vector to sort.
 ** @param func     the sort function. If NULL then the vector is sorted
 ** byte the address of the object pointers. (useful when determining if
 ** vector contains duplicate elements or merging two vectors).
 **
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 ************************************************************************/
Jxta_status jxta_vector_qsort(Jxta_vector * vector, Jxta_object_compare_func * func)
{
    if (!JXTA_OBJECT_CHECK_VALID(vector))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == func)
        func = (Jxta_object_compare_func *) address_comparator;

    apr_thread_mutex_lock(vector->mutex);

    qsort(vector->index, vector->size, sizeof(Jxta_object *), (int (*)(const void *a, const void *b)) func);

    apr_thread_mutex_unlock(vector->mutex);

    return JXTA_SUCCESS;
}
