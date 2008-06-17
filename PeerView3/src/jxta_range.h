/*
 * Copyright (c) 2005-2006 Sun Microsystems, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESSED OR IMPLIED
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
  */

#ifndef JXTA_RANGE_H
#define JXTA_RANGE_H

#include <stddef.h>
#include <stdio.h>

#include "jxta_object.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_id.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif /* if 0 */
#endif /* __cplusplus */


typedef struct _jxta_range Jxta_range;

/**
 * One-time call to initialize the range component
 * 
**/
JXTA_DECLARE(void) jxta_range_init(void);
    
/**
 * Destroy the Range component
**/
JXTA_DECLARE(void) jxta_range_destroy(void);
    
/**
 * Create a new range object
 * 
 * @return range object
**/
JXTA_DECLARE(Jxta_range *)
    jxta_range_new(void);

/**
 * Create a new range object
 * @param const char* nameSpace - namespace:advertisement format
 * @param const char* elemAttr - element/attribute space seperated
 * 
 * @return range object
 **/
JXTA_DECLARE(Jxta_range *)
    jxta_range_new_1(const char *nameSpace, const char *elemAttr);
    
/**
 * Add a new range keyed by the namespace, element and attribute.
 * 
 * @param Jxta_range* rge - Range object
 * @param const char* nspace - Name space of the advertisement
 * @param const char* element - Element
 * @param const char* aattribute - Attribute
 * @param const char* range - Range is specified as "(high :: low)"
 * @param Jxta_boolean force - If a range exists replace it.
 *
 * @return Jxta_status
 * 
**/
JXTA_DECLARE(Jxta_status)
    jxta_range_add(Jxta_range *rge, const char *nspace, const char *element, const char *aattribute, const char *range, Jxta_boolean force);
    
/**
 * Remove the specified range.
 * 
 * @param Jxta_range* rge - Range object
 * @param const char* nspace - Name space of the advertisement
 * @param const char* element - Element
 * @param const char* aattribute - Attribute
 * @param const char* range - Range is specified as "(high :: low)"
 *
 * @return Jxta_status
 * 
**/    
JXTA_DECLARE(Jxta_status)
    jxta_range_remove(Jxta_range *rge, const char *nspace, const char *element, const char *aattribute, const char *range);
/**
 * Get the position where the value falls with the specified range given the number of segments.
 * 
 * @param Jxta_range* rge - Range object
 * @param const char * value - String representation of the numeric value within the range
 *
 * @return int - >= 0 position within the segments (0 based)
 *               -1 - segements are less than 2
 *               -2 - value is less than the low of the range 
 *               -3 - value is greater than the high of the range
 * 
**/ 
JXTA_DECLARE(int)
    jxta_range_get_position(Jxta_range *rge, const char* value, int segments);
    
/**
 * Report an instance within a range
 *
 * @param Jxta_range* rge - Range object
 * @param double value - Numeric value within the range
 *
 * @return Jxta_status
**/
JXTA_DECLARE(Jxta_status)
    jxta_range_item_added(Jxta_range *rge, double value);
    
/**
 * Report a replication of an item within range
 *
 * @param Jxta_range* rge - Range object
 * @param double value - Numeric value within the range
 *
 * @return Jxta_status
**/
JXTA_DECLARE(Jxta_status)
    jxta_range_item_replicated(Jxta_range *rge, double value);

/**
 * Set the namespace for this range
 *
 * @param Jxta_range* rge - Range object
 * @param const char * -  namespace representing this range 
**/
    
JXTA_DECLARE(void)
    jxta_range_set_name_space(Jxta_range *rge, const char *nspace);    
    
/**
 * Get the name space for this range
 *
 * @param Jxta_range* rge - Range object
 * @return - const char * -  nameSpace representing this range 
**/
    
JXTA_DECLARE(const char *)
    jxta_range_get_name_space(Jxta_range *rge);
    
/**
 * Set the element for this range
 *
 * @param Jxta_range* rge - Range object
 * @param const char * -  element representing this range 
**/
    
JXTA_DECLARE(void)
    jxta_range_set_element(Jxta_range *rge, const char *element);    
    
/**
 * Get the element for this range
 *
 * @param Jxta_range* rge - Range object
 * @return - const char * -  element representing this range 
**/
    
JXTA_DECLARE(const char *)
    jxta_range_get_element(Jxta_range *rge);
    
/**
 * Set the element/attr for this range
 * Element and attribute are space seperated
 *
 * @param Jxta_range* rge - Range object
 * @param const char * -  element/attr representing this range 
**/
    
JXTA_DECLARE(void)
    jxta_range_set_element_attr(Jxta_range *rge, const char *elemAttr);    
    
/**
 * Set the attribute for this range
 *
 * @param Jxta_range* rge - Range object
 * @param const char * -  attribute representing this range 
**/
    
JXTA_DECLARE(void)
    jxta_range_set_attribute(Jxta_range *rge, const char *aattribute);    
/**
 * Get the attribute for this range
 *
 * @param Jxta_range* rge - Range object
 *
 * @return - const char * - attribute representing this range 
**/
    
JXTA_DECLARE(const char *)
    jxta_range_get_attribute(Jxta_range *rge);
/**
 * Get the high value of the range
 *
 * @param Jxta_range* rge - Range object
 * @return - double - high end of this range 
**/
 
JXTA_DECLARE(double)
    jxta_range_get_high(Jxta_range *rge);
/**
 * Get the low value of the range
 *
 * @param Jxta_range* rge - Range object
 * @return - double - low end of this range 
**/
    
JXTA_DECLARE(double)
    jxta_range_get_low(Jxta_range *rge);

    
/**
 * set the range for this range object
 * 
 * @param Jxta_range *rge - Range object
 * @param const char *nSpace - Name space
 * @param const char *elemAttr - Element/Attribute space seperated
 * @param const char *range - "(xxx :: yyy)"  xxx - low yyy - high
**/    
JXTA_DECLARE(void)
    jxta_range_set_range(Jxta_range *rge, const char *nSpace, const char *elemAttr, const char *range);
     
/**
 * Get the string representation of the range
 *
 * @param Jxta_range* rge - Range object
 * @return char * - string representation of the range
 *     format "(low :: high)"
**/
    
JXTA_DECLARE(char *)
    jxta_range_get_range_string(Jxta_range *rge);
/**
 * Get a vector of all the ranges that have been instantiated
 *
 * @param Jxta_range* rge - Range object
 * @return Jxta_vector * - Vector of range objects that have been reported
**/

JXTA_DECLARE(Jxta_vector *)
    jxta_range_get_ranges(void);

/**
 * Return a number from the string passed as a parameter
 *
 * @param const char* sNum - string value of Integer, Decimal or float
 * 
 * @return double - float value
 *
**/
JXTA_DECLARE(double) jxta_range_cast_to_number(const char * sNum);

#ifdef __cplusplus
#if 0
{
#endif /*if 0 */
}
#endif /*ifdef __cplusplus */

#endif /* #ifndef JXTA_RANGE_H */
