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
 * $Id: jxta_id.h,v 1.10 2006/05/23 17:39:53 slowhog Exp $
 */

#ifndef JXTAID_H
#define JXTAID_H

#include "jxta_apr.h"
#include "jxta_object.h"
#include "jstring.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
* JXTA ID TYPES
**/

/**
* Jxta ids are opaque Jxta_objects. You can use JXTA_OBJECT_RELEASE to
* free them.
**/
typedef const struct _jxta_id Jxta_id;

typedef Jxta_id Jxta_CID;
typedef Jxta_id Jxta_PGID;
typedef Jxta_id Jxta_PID;

typedef Jxta_id Jxta_MCID;
typedef Jxta_id Jxta_MSID;

/**
* JXTA ID CONSTANTS
**/


/**
* A placeholder for uninitialized ids or default value. prefered to using
* NULL because nullID is more symetric, has a printable representation
* and reduces need for special handling code.
**/
JXTA_DECLARE_DATA Jxta_id *jxta_id_nullID;

/**
* Id of the world peer group. A well known Jxta ID.
**/
JXTA_DECLARE_DATA Jxta_id *jxta_id_worldNetPeerGroupID;

/**
* Id of the default network peer group. A well known Jxta ID.
**/
JXTA_DECLARE_DATA Jxta_id *jxta_id_defaultNetPeerGroupID;

/**
* A string containing the URI encoding scheme for Jxta Ids. "urn"
**/
JXTA_DECLARE_DATA const char *jxta_id_URIENCODINGNAME;

/**
* A string containing the URN name space used for Jxta Ids. "jxta"
**/
JXTA_DECLARE_DATA const char *jxta_id_URNNAMESPACE;

/**
* names of the id format which will be used to construct new
* Jxta ids.
**/
JXTA_DECLARE_DATA char const *jxta_id_DEFAULT_ID_FORMAT;

/**
* names of all of the id formats which are supported by this
* implementation as a NULL terminated array of string pointers.
*
**/
JXTA_DECLARE_DATA char const *jxta_id_ID_FORMATS[];


/**
*
* JXTA ID INFORMATIONALS
*
**/


/**
* Returns name of the id format which will be used to construct new
* Jxta ids.
*
* @return string containing the name of the default id format.
**/
JXTA_DECLARE(char const *) jxta_id_get_default_id_format(void);

/**
* Returns names of all of the id formats which are supported by this
* implementation as a NULL terminated array of string pointers.
*
* @return NULL terminated array of string pointers containing the names
* of all of the id formats supported by this Jxta implementation.
**/
JXTA_DECLARE(char const **) jxta_id_get_id_formats(void);


/**
**
** JXTA ID Constructors
**
**/


/**
* Constructs a new random peer group id of the default id format.
* 
* @param pg where to store the new peer group id.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_peergroupid_new_1(Jxta_id ** pg);

/**
* Constructs a new peer group id of the default id format. This varient
* allows you to provide your own source of random/canonical seed
* information.
* 
* @param pg where to store the new peer group id.
* @param seed contains the random seed information to be used for this id.
* @param len length of the seed information it should be at least 4 bytes
*  length and should be a pseudo-random/canonical value.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_peergroupid_new_2(Jxta_id ** pg, unsigned char const *seed, size_t len);

/**
* Constructs a new random peer id in the peer group specified. The id
* will be of the same id format as the peer group id.
* 
* @param peerid where to store the new peer id.
* @param pg the group in which the peer id will be created. the id format
*  used for the peer id will also match the id format of this group id.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_peerid_new_1(Jxta_id ** peerid, Jxta_id * pg);

/**
* Constructs a new peer id in the peer group specified. The id
* will be of the same id format as the peer group id. This varient
* allows you to provide your own source of random/canonical seed
* information.
* 
* @param peerid where to store the new peer id.
* @param pg the group in which the peer id will be created. the id format
* used for the peer id will also match the id format of this group id.
* @param seed contains the random seed information to be used for this id.
* @param len length of the seed information it should be at least 4 bytes
*  length and should be a pseudo-random/canonical value.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_peerid_new_2(Jxta_id ** peerid, Jxta_id * pg, unsigned char const *seed, size_t len);

/**
* Constructs a new random codat id in the peer group specified. The id
* will be of the same id format as the peer group id. This varient of
* codat id is used for dynamic content. For refering to fixed content
* use the jxta_id_codatid_new_2 varient.
* 
* @param codatid where to store the new cpdat id.
* @param pg the group in which the codat id will be created. the id format
*  used for the codat id will also match the id format of this group id.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_codatid_new_1(Jxta_id ** codatid, Jxta_id * pg);

/**
* Constructs a new random codat id in the peer group specified. The id
* will be of the same id format as the peer group id. This varient of
* codat id is used for static content. For refering to dynamic content
* use the jxta_id_codatid_new_1 varient. The content is hashed and the id
* returned may include this hash value.
* 
* @param codatid where to store the new codat id.
* @param pg the group in which the codat id will be created. the id format
*  used for the codat id will also match the id format of this group id.
* @param read_func the function to use to read the content data.
* @param stream argument to be passed to read_func.
* @return returns JXTA_SUCCESS if successful, otherwise errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_codatid_new_2(Jxta_id ** codatid, Jxta_id * pg, ReadFunc read_func, void *stream);

/**
* Constructs a new random pipe id in the peer group specified. The id
* will be of the same id format as the peer group id. 
* 
* @param pipeod where to store the new pipe id.
* @param pg the group in which the pipe id will be created. the id format
*  used for the pipe id will also match the id format of this group id.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_pipeid_new_1(Jxta_id ** pipeid, Jxta_id * pg);

/**
* Constructs a new pipe id in the peer group specified. The id will be 
* of the same id format as the peer group id.  This varient
* allows you to provide your own source of random/canonical seed
* information.
* 
* @param pipeid where to store the new pipe id.
* @param pg the group in which the pipe id will be created. the id format
*  used for the pipe id will also match the id format of this group id.
* @param seed contains the random seed information to be used for this id.
* @param len length of the seed information it should be at least 4 bytes
*  length and should be a pseudo-random/canonical value.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_pipeid_new_2(Jxta_id ** pipeid, Jxta_id * pg, unsigned char const *seed, size_t len);

/**
* Constructs a new module class id. The id will be of the default id 
* format.
* 
* @param mcid where to store the new module class id.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_moduleclassid_new_1(Jxta_id ** mcid);

/**
* Constructs a new module class id.  The id will be of the same id 
* format as the base module class id.  
* 
* @param mcid where to store the new module class id.
* @param base a base module class id to use in the generation of this id.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_moduleclassid_new_2(Jxta_id ** mcid, Jxta_id * base);

/**
* Constructs a new module spec id. The id will be of the same id 
* format as the provided module class id.  
* 
* @param msid where to store the new module spec id.
* @param mcid the module spec id on which to base this spec id.
* @return returns JXTA_SUCCESS if successful, otherwise (unlikely) errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_modulespecid_new(Jxta_id ** msid, Jxta_id * mcid);

/**
* Constructs a new jxta id from a string.
* 
* @param id where to store the new id.
* @param id_str  pointer to the string containing the textual representation of the ID.
* @param len  number of characters in the string
* @return returns JXTA_SUCCESS if successful, otherwise errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_from_str(Jxta_id ** id, const char * id_str, size_t len);

/**
* Constructs a new jxta id from a NULL terminate string.
* 
* @param id where to store the new id.
* @param id_cstr  string containing the textual representation of the ID.
* @return returns JXTA_SUCCESS if successful, otherwise errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_from_cstr(Jxta_id ** id, const char * id_cstr);

/**
* Constructs a new jxta id from a jstring.
* 
* @param id where to store the new id.
* @param jid  string containing the textual representation of the ID.
* @return returns JXTA_SUCCESS if successful, otherwise errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_from_jstring(Jxta_id ** id, JString * jid);


/**
*
* JXTA ID OPERATIONS
*
**/


/**
** Returns the id format of the provided id.
** 
** @param jid  The id who's format is desired.
** @return pointer to the format of the provided id. If the id is invalid
**  then NULL is returned.
**/
JXTA_DECLARE(char const *) jxta_id_get_idformat(Jxta_id * jid);

/**
** Returns the unique canonical portion of the provided id. this value
** may be useful for hashing, URI construction, etc.
** 
** @param string where to store the created string.
** @param jid  The id who's unique value is desired.
** @return returns JXTA_SUCCESS if successful, otherwise errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_get_uniqueportion(Jxta_id * jid, JString ** string);

/**
** Returns the URI presentation of the provided id. 
** 
** @param string URI presentation of the id.
** @param jid  The id who's unique value is desired.
** @return returns JXTA_SUCCESS if successful, otherwise errors.
**/
JXTA_DECLARE(Jxta_status) jxta_id_to_jstring(Jxta_id * jid, JString ** string);

JXTA_DECLARE(Jxta_status) jxta_id_to_cstr(Jxta_id * id, char **p, apr_pool_t *pool);

/**
** Compares two ids for equality. Note that for programmatic reasons,
** passing NULL for both ids *DOES* return TRUE.
** 
** @param jid1  one id to compare
** @param jid2  the other id to compare.
** @return returns TRUE if equivalent, otherwise false.
**/
JXTA_DECLARE(Jxta_boolean) jxta_id_equals(Jxta_id * jid1, Jxta_id * jid2);

/**
 ** Returns a hashcode for the id.
 **
 ** @param jid a pointer to the id to use.
 ** @return the hashcode for this id or '0' if the object is invalid.
 **/
JXTA_DECLARE(unsigned int) jxta_id_hashcode(Jxta_id * jid);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif
#endif /* JXTA_ID_H */

/* vi: set ts=4 sw=4 tw=130 et: */
