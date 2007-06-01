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

#ifndef JXTA_QUERY_H
#define JXTA_QUERY_H

#include "jxta_object.h"
#include "jxta_vector.h"

#include "jstring.h"
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/globals.h>

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
#define ENHANCED_QUERY_LOG "QueryLog"
struct _jxta_query_context {
    JXTA_OBJECT_HANDLE;
    Jxta_vector *queries;       /* predicates */
    JString *documentName;      /* document name in case wild card name space */
    JString *sort;              /* sort from the compiled expression */
    JString *prefix;            /* prefix of the name space */
    JString *name;              /* Name part of the name space */
    JString *nameSpace;         /* Name space for this advertisement */
    JString *query;             /* Query we are building from */
    JString *queryNameSpace;    /* Name space in the query */
    JString *newQuery;          /* This is the new query after it is transformed */
    JString *origQuery;         /* initial query */
    JString *element;           /* The current element */
    JString *attribName;        /* current attribute name */
    JString *value;             /* The current query value */
    JString *sqlcmd;            /* build the SQL command here */
    JString *sqloper;           /* don't think we need this yet */
    JString *level2;            /* The level 2 element */
    Jxta_boolean bAttribute;    /* flags the attribute */
    xmlXPathContextPtr xpathctx;    /* XPath context for query */
    xmlXPathCompExprPtr xpathcomp;  /* compiled XPath expression pointer */
    int found;                  /* items found */
    Jxta_boolean first;         /* no elements have been processed */
    int levelNumber;            /* number of predicates before the level changes */
    int currLevel;              /* current level during the scan */
    int endOfCollection;        /* end of current collection */
};



typedef struct _jxta_query_context Jxta_query_context;

struct _jxta_query_element {
    JXTA_OBJECT_HANDLE;
    JString *jSQL;
    JString *jBoolean;
    JString *jOper;
    JString *jName;
    JString *jValue;
};

typedef struct _jxta_query_element Jxta_query_element;


JXTA_DECLARE(Jxta_query_context *) jxta_query_new(const char *q);


/**
* Execute an XPath like query against the advertisement string.
**/
JXTA_DECLARE(Jxta_status)
    jxta_query_XPath(Jxta_query_context * ctx, const char *advXML, Jxta_boolean bResults);



JXTA_DECLARE(void) jxta_query_build_SQL_operator(JString * elem, JString * sqloper, JString * value, JString * result);


/************************************************************************
 * 									*
 * 			Parser Types					*
 * 									*
 ************************************************************************/

/*
 * Types are private:
 */

typedef enum {
    XPATH_OP_END = 0,
    XPATH_OP_AND,
    XPATH_OP_OR,
    XPATH_OP_EQUAL,
    XPATH_OP_CMP,
    XPATH_OP_PLUS,
    XPATH_OP_MULT,
    XPATH_OP_UNION,
    XPATH_OP_ROOT,
    XPATH_OP_NODE,
    XPATH_OP_RESET,
    XPATH_OP_COLLECT,
    XPATH_OP_VALUE,
    XPATH_OP_VARIABLE,
    XPATH_OP_FUNCTION,
    XPATH_OP_ARG,
    XPATH_OP_PREDICATE,
    XPATH_OP_FILTER,
    XPATH_OP_SORT
#ifdef LIBXML_XPTR_ENABLED
        , XPATH_OP_RANGETO
#endif
} xmlXPathOp;

typedef enum {
    AXIS_ANCESTOR = 1,
    AXIS_ANCESTOR_OR_SELF,
    AXIS_ATTRIBUTE,
    AXIS_CHILD,
    AXIS_DESCENDANT,
    AXIS_DESCENDANT_OR_SELF,
    AXIS_FOLLOWING,
    AXIS_FOLLOWING_SIBLING,
    AXIS_NAMESPACE,
    AXIS_PARENT,
    AXIS_PRECEDING,
    AXIS_PRECEDING_SIBLING,
    AXIS_SELF
} xmlXPathAxisVal;

typedef enum {
    NODE_TEST_NONE = 0,
    NODE_TEST_TYPE = 1,
    NODE_TEST_PI = 2,
    NODE_TEST_ALL = 3,
    NODE_TEST_NS = 4,
    NODE_TEST_NAME = 5
} xmlXPathTestVal;

typedef enum {
    NODE_TYPE_NODE = 0,
    NODE_TYPE_COMMENT = XML_COMMENT_NODE,
    NODE_TYPE_TEXT = XML_TEXT_NODE,
    NODE_TYPE_PI = XML_PI_NODE
} xmlXPathTypeVal;

typedef struct _xmlXPathStepOp xmlXPathStepOp;
typedef xmlXPathStepOp *xmlXPathStepOpPtr;
struct _xmlXPathStepOp {

    xmlXPathOp op;              /* The identifier of the operation */
    int ch1;                    /* First child */
    int ch2;                    /* Second child */
    int value;
    int value2;
    int value3;
    void *value4;
    void *value5;
    void *cache;
    void *cacheURI;
};

struct _xmlXPathCompExpr {
    int nbStep;                 /* Number of steps in this expression */
    int maxStep;                /* Maximum number of steps allocated */
    xmlXPathStepOp *steps;      /* ops for computation of this expression */
    int last;                   /* index of last step in expression */
    xmlChar *expr;              /* the expression being computed */
    xmlDictPtr dict;            /* the dictionnary to use if any */
#ifdef DEBUG_EVAL_COUNTS
    int nb;
    xmlChar *string;
#endif
};

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif

/* vi: set ts=4 sw=4 tw=130 et: */
