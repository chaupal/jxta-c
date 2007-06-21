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
 * $Id: jxta_vector.h,v 1.14 2006/10/01 01:07:10 bondolo Exp $
 */


/**************************************************************************************************
 **  Jxta_vector
 **
 ** A Jxta_vector is a Jxta_object that manages a growable list of Jxta_object.
 ** 
 ** Sharing and releasing of Jxta_object is properly done (refer to the API of Jxta_object
 ** and Jxta_vector).
 **
 **************************************************************************************************/

#ifndef JXTA_VECTOR_H
#define JXTA_VECTOR_H

#include "jxta_object.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 * Jxta_vector is the public type of the vector
 **/
typedef struct _jxta_vector Jxta_vector;



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

JXTA_DECLARE(Jxta_vector *) jxta_vector_new(unsigned int initialSize);


/************************************************************************
 ** Add an object at a particular index. Object with higher index are
 ** moved up. The size of the vector is increased by one.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to the Jxta_object to add.
 ** @param at_index where to add the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

JXTA_DECLARE(Jxta_status) jxta_vector_add_object_at(Jxta_vector * vector, Jxta_object * object, unsigned int at_index);



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

JXTA_DECLARE(Jxta_status) jxta_vector_add_object_first(Jxta_vector * vector, Jxta_object * object);


/************************************************************************
 ** Add an object at the end of the vector.  The size of the vector is 
 ** increased by one.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to the Jxta_object to add.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_add_object_last(Jxta_vector * vector, Jxta_object * object);

/************************************************************************
 ** Add all of the objects at the specified location of the vector.  The size of the vector is 
 ** increased by the number of elements in objects.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param objects a pointer to the vector to add.
 ** @param at_index where to add the objects.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_addall_objects_at(Jxta_vector * vector, Jxta_vector * objects, unsigned int at_index);

/************************************************************************
 ** Add all of the objects to the start of the vector.  The size of the vector is 
 ** increased by the number of elements in objects.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param objects a pointer to the vector to add.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_addall_objects_first(Jxta_vector * vector, Jxta_vector * objects);

/************************************************************************
 ** Add all of the objects to the end of the vector.  The size of the vector is 
 ** increased by the number of elements in objects.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param objects a pointer to the vector to add.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_addall_objects_last(Jxta_vector * vector, Jxta_vector * objects);

/************************************************************************
 ** Get the object which is at the given index. The object is shared
 ** before being returned: the caller is responsible for releasing the
 ** object when it is no longer used.
 ** The object is not removed from the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to a Jxta_object pointer which will contain the
 ** the object.
 ** @param at_index index of the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_get_object_at(Jxta_vector * vector, Jxta_object ** objectPt, unsigned int at_index);

/************************************************************************
 ** Remove all the objects in the vector with the same value(pointer).  
 ** The objects removed are automatically released.
 **
 ** The size of the vector is decreased.
 **
 ** @param me a pointer to the vector to use.
 ** @param object a pointer to the Jxta_object to be removed
 ** @return -1 if arguments are invalid, number of objects been removed
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(int) jxta_vector_remove_object(Jxta_vector * me, Jxta_object * object);


/************************************************************************
 ** Remove the object which is at the given index.
 ** If the callers set objectPt to a non null pointer to a Jxta_object
 ** pointer, this pointer will contained the removed object. In that case,
 ** the caller is responsible for releasing the object when it is not used
 ** anymore. If objectPt is set to NULL, then the object is automatically
 ** released.
 **
 ** <p/>The size of the vector is decreased.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to a Jxta_object pointer which will contain the
 ** the removed object.
 ** @param at_index index of the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_remove_object_at(Jxta_vector * vector, Jxta_object ** objectPt, unsigned int at_index);


/************************************************************************
 ** Returns the number of objects in the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @return the number of objects in the vector.
 *************************************************************************/
JXTA_DECLARE(unsigned int) jxta_vector_size(Jxta_vector * vector);


/************************************************************************
 ** Duplicates a vector, but not its contents: a new vector object is
 ** created, with a reference to all the objects that are in the original
 ** object. (shallow clone) Note that the objects are automatically shared.
 ** 
 ** <p/>Using index and length arguments, a sub section of a vector can
 ** be cloned instead of the totality of the vector
 **
 ** @param source a pointer to the original vector
 ** @param dest a pointer to a Jxta_vector pointer which will contain the
 ** the cloned vector.
 ** @param at_index index start index
 ** @param length number of objects, starting at the given index to clone. if 
 **  length is larger than the available number of elements, then only the
 **  available elements will be returned. (INT_MAX will get all elements).
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_clone(Jxta_vector * source, Jxta_vector ** dest, unsigned int at_index,
                                            unsigned int length);


/*************************************************************************
 ** Removes all of the elements from a vector. All of the objects are
 ** formerly in the vector are unshared.
 **
 ** @param vector the vector to clear.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 ************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_clear(Jxta_vector * vector);


/*************************************************************************
 ** Exchange the position of two elements within the vector. The vector is
 ** otherwise unchanged.
 **
 ** @param vector   the vector containing the elements to be swapped.
 ** @param first    index of the first element to be swapped.
 ** @param second   index of the second element to be swapped.
 **
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 ************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_swap_elements(Jxta_vector * vector, unsigned int first, unsigned int second );

/*************************************************************************
 ** Move the specified element to the start of the vector and move all 
 ** intervening elements backwards (towards the end) one position. The 
 ** vector is otherwise unchanged.
 **
 ** @param vector   the vector containing the elements to be swapped.
 ** @param to_first    index of the first element to be placed first.
 **
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 ************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_move_element_first(Jxta_vector * vector, unsigned int to_first );

/*************************************************************************
 ** Move the specified element to the end of the vector and move all 
 ** intervening elements forward (towards the start) one position. The 
 ** vector is otherwise unchanged.
 **
 ** @param vector   the vector containing the elements to be swapped.
 ** @param to_last  index of the first element to be placed first.
 **
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 ************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_vector_move_element_last(Jxta_vector * vector, unsigned int to_last );

/*************************************************************************
 ** Determines if the vector contains the specified object.
 **
 ** @param vector   the vector to search.
 ** @param object   The object which is sought.
 ** @param func     the equals function. If NULL then the vector is
 ** searched for an object who's address matches that of the object.
 **
 ** @return TRUE if the object was found in the vector otherwise FALSE.
 ************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_vector_contains(Jxta_vector * vector, Jxta_object* object, Jxta_object_equals_func func);

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
JXTA_DECLARE(Jxta_status) jxta_vector_qsort(Jxta_vector * vector, Jxta_object_compare_func func);

/*************************************************************************
 ** Shuffles a vector. The elements in the vector will re-arranged in
 ** a pseudo random order.
 **
 ** <p/>This function assumes that srand() has been called with good seed
 ** material. See <http://benpfaff.org/writings/clc/random-seed.html> for good
 ** suggestions.
 **
 ** @param vector   the vector to shuffle.
 ************************************************************************/
JXTA_DECLARE(void) jxta_vector_shuffle(Jxta_vector * vector);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_SERVICE_H */
