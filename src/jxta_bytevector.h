/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_bytevector.h,v 1.4 2005/01/28 02:22:45 slowhog Exp $
 */


/**************************************************************************************************
 **  Jxta_bytevector
 **
 ** A Jxta_bytevector is a Jxta_object that manages a growable list of Jxta_object.
 ** 
 ** Sharing and releasing of Jxta_object is properly done (refer to the API of Jxta_object
 ** and Jxta_bytevector).
 **
 **************************************************************************************************/

#ifndef JXTA_BYTEVECTOR_H
#define JXTA_BYTEVECTOR_H

#include "jxta_object.h"

#ifdef __cplusplus
extern "C" {

    /* avoid confusing indenters */
#if 0
}
#endif
#endif
/**
 * Jxta_bytevector is the public type of the vector
 **/ typedef struct _jxta_bytevector const Jxta_bytevector;



/************************************************************************
 ** Allocates a new Vector. The vector is allocated and is initialized
 ** (ready to use). No need to apply JXTA_OBJECT_INIT, because new does it.
 ** The initial capacity of the vector is set to a default size.
 **
 ** The creator of a vector is responsible to release it when not used
 ** anymore.
 **
 ** @return a new vector, or NULL if allocation failed.
 *************************************************************************/
Jxta_bytevector *jxta_bytevector_new_0(void);

/************************************************************************
 ** Allocates a new Vector. The vector is allocated and is initialized
 ** (ready to use). No need to apply JXTA_OBJECT_INIT, because new does it.
 ** The initial capacity of the vector is set by initialSize. If that value is
 ** 0, then a default size is used.
 **
 ** The creator of a vector is responsible to release it when not used
 ** anymore.
 **
 ** @param initialSize is the initial size of the vector. 0 means default.
 ** @return a new vector, or NULL if allocation failed.
 *************************************************************************/
Jxta_bytevector *jxta_bytevector_new_1(size_t initialSize);

/************************************************************************
 ** Allocates a new Vector initialized with the provided pointer. No need 
 ** to apply JXTA_OBJECT_INIT, because new does it.
 ** The pointer provided must be a malloc/calloc allocated pointer and
 ** becomes the "property" of the bytevector.
 **
 ** The creator of a vector is responsible to release it when not used
 ** anymore.
 **
 ** @param initialPtr The malloc pointer to use for source data.
 ** @param initialSize is the initial size of the byte vector.
 ** @param initalCapacity is the initial capacity of the byte vector.
 ** @return a new vector, or NULL if allocation failed.
 *************************************************************************/
Jxta_bytevector *jxta_bytevector_new_2(unsigned char *initialPtr, size_t initialSize, size_t initalCapacity);

/************************************************************************
 ** Causes the vector to be synchronized.
 **
 ** @param vector ensure this vector is synchronized.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

Jxta_status jxta_bytevector_set_synchronized(Jxta_bytevector * vector);

/*************************************************************************
 ** Removes all of the elements from a vector. The initial capacity of the
 ** vector is set by initialSize. If that value is 0, then a default size 
 ** is used.
 **
 ** @param vector the vector to clear.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 ************************************************************************/
Jxta_status jxta_bytevector_clear(Jxta_bytevector * vector, size_t initialSize);


/************************************************************************
 ** Add a byte at a particular index. Bytes with higher index are
 ** moved up. The size of the vector is increased by one.
 **
 ** @param vector a pointer to the vector to use.
 ** @param byte the byte to add.
 ** @index where to add the byte.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_add_byte_at(Jxta_bytevector * vector, unsigned char byte, unsigned int index);


/************************************************************************
 ** Add a byte at the beginning of the vector. byes already in the
 ** vector are moved up. The size of the vector is increased by one.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to the Jxta_object to add.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_add_byte_first(Jxta_bytevector * vector, unsigned char byte);


/************************************************************************
 ** Add a byte at the end of the vector.  The size of the vector is 
 ** increased by one.
 **
 ** The object is automatically shared when added to the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param byte a pointer to the Jxta_object to add.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_add_byte_last(Jxta_bytevector * vector, unsigned char byte);


/************************************************************************
 ** Add bytes at a particular index. Bytes with higher index are
 ** moved up. The size of the vector is increased by length.
 **
 ** @param vector a pointer to the vector to use.
 ** @param bytes the bytes to add. If NULL then the vector is filled with
 ** '0' bytes.
 ** @param index where to add the bytes.
 ** @param length number of bytes to add
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_add_bytes_at(Jxta_bytevector * vector, unsigned char const *bytes, unsigned int index, size_t length);


/************************************************************************
 ** Add a bytevector at a particular index. Bytes with higher index are
 ** moved up. The size of the vector is increased by the size of the added
 ** vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param bytes the bytevector to add.
 ** @param index where to add the byte.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_add_bytevector_at(Jxta_bytevector * vector, Jxta_bytevector * bytes, unsigned int index);


/************************************************************************
 ** Add a bytevector at a particular index. Bytes with higher index are
 ** moved up. The size of the vector is increased by the size of the added
 ** vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param bytes the bytevector to add.
 ** @param index where to add the byte.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status
jxta_bytevector_add_partial_bytevector_at(Jxta_bytevector * vector,
                                          Jxta_bytevector * bytes, unsigned int srcIndex, size_t length, unsigned int dstindex);

/************************************************************************
 ** Add bytes at a particular index from a stream. Bytes with higher index are
 ** moved up. The size of the vector is increased by the size of the added
 ** vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param func function to call to read bytes.
 ** @param stream parameter to pass to the read function.
 ** @param length number of bytes to read.
 ** @index where to add the byte.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status
jxta_bytevector_add_from_stream_at(Jxta_bytevector * vector, ReadFunc func, void *stream, size_t length, unsigned int index);

/************************************************************************
 ** Get the byte which is at the given index. 
 ** The byte is not removed from the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to a Jxta_object pointer which will contain the
 ** the object.
 ** @param index index of the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_get_byte_at(Jxta_bytevector * vector, unsigned char *byte, unsigned int index);


/************************************************************************
 ** Makes a clone of the specified portion of the byte vector and returns
 ** it. The bytes are not removed from the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to a Jxta_object pointer which will contain the
 ** the object.
 ** @index index of the first byte to clone.
 ** @index length maximum length to copy. The returned byte vector may be
 ** shorter than this length. INT_MAX will always copy the whole vector.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_get_bytes_at(Jxta_bytevector * vector, unsigned char *dest, unsigned int index, size_t length);

Jxta_status jxta_bytevector_get_bytes_at_1(Jxta_bytevector * vector, unsigned char *dest, unsigned int index, size_t * length);

/************************************************************************
 ** Makes a clone of the specified portion of the byte vector and returns
 ** it. The bytes are not removed from the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @param object a pointer to a Jxta_object pointer which will contain the
 ** the object.
 ** @param index index of the first byte to clone.
 ** @param length maximum length to copy. The returned byte vector may be
 ** shorter than this length. INT_MAX will always copy the whole vector.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status
jxta_bytevector_get_bytevector_at(Jxta_bytevector * vector, Jxta_bytevector ** dest, unsigned int index, size_t length);

/************************************************************************
 ** Writes the byte vector to a stream.
 **
 ** @param vector a pointer to the vector to write.
 ** @param write function to call.
 ** @param stream stream parameter to pass to write
 ** @param index index of the first byte to clone.
 ** @param length maximum length to write. The number of bytes written may be
 ** shorter than this length. INT_MAX will always copy the whole vector.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise. Also returns errors as received from func.
 *************************************************************************/
Jxta_status jxta_bytevector_write(Jxta_bytevector * vector, WriteFunc func, void *stream, unsigned int index, size_t length);

/************************************************************************
 ** Remove the byte which is at the given index.
 **
 ** The size of the vector is decreased.
 **
 ** @param vector a pointer to the vector to use.
 ** @param index index of the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_remove_byte_at(Jxta_bytevector * vector, unsigned int index);


/************************************************************************
 ** Remove bytes begining at the given index.
 **
 ** The size of the vector is decreased by length
 **
 ** @param vector a pointer to the vector to use.
 ** @param index index of the object.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
Jxta_status jxta_bytevector_remove_bytes_at(Jxta_bytevector * vector, unsigned int index, size_t length);


/************************************************************************
 ** Returns the number of bytes in the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @return the number of objects in the vector.
 *************************************************************************/
size_t jxta_bytevector_size(Jxta_bytevector * vector);

/************************************************************************
 ** Returns true if the two vectors are equivalent.
 **
 ** @param vector a pointer to the vector to use.
 ** @return true if the vectors are equivalent.
 *************************************************************************/
Jxta_boolean jxta_bytevector_equals(Jxta_bytevector * vector1, Jxta_bytevector * vector2);

/************************************************************************
 ** Returns a hashcode for the vector.
 **
 ** @param vector a pointer to the vector to use.
 ** @return the hashcode for this vector or '0' if the object is invalid.
 *************************************************************************/
unsigned int jxta_bytevector_hashcode(Jxta_bytevector * vector);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_BYTEVECTOR_H */
