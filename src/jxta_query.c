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
 * $Id$
  *
  */
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#ifdef WIN32
#include <float.h>
#define isnan(a)      _isnan(a) 
#endif
#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_types.h"
#include "jxta_query.h"
#include "jxta_debug.h"
#include "jxta_log.h"
#include "jxta_sql.h"
#include "jxta_cm.h"
#include "jxta_cm_private.h"

#define __log_cat "QUERY"
static void query_reset_context(Jxta_query_context * jctx);

static Jxta_status query_SQL(Jxta_query_context * jctx);

static Jxta_status query_find_Ns(Jxta_query_context * jctx, const char *q, Jxta_boolean appendQuery);

static Jxta_status query_create_SQL(Jxta_query_context * jctx, xmlXPathCompExprPtr comp, xmlXPathStepOpPtr op, int depth);

static void query_examine_object(Jxta_query_context * jctx, xmlXPathObjectPtr cur, int depth);

static void query_split_Ns(Jxta_query_context * jctx, const char *q, Jxta_boolean bPrefix, Jxta_boolean bName);

static Jxta_status query_xform_query(Jxta_query_context * jctx, const char *query);

/* Function calls contain 10 steps on the libxml stack. */
#define NB_WILD_STEPS (20 - 1)

static void jxta_wildxpath_handler(xmlXPathParserContextPtr ctxt, int nargs);
static void jxta_wildxpath_append_subexpr(const char* subexpr, xmlXPathCompExprPtr outcomp, xmlXPathCompExprPtr* subcomps, int i, int* s, int* start, int* last);
static int  jxta_wildxpath_haswildcard(const char* pattern);


/** 
 * Delete function to destroy the context object
 *
 */
static void jxta_query_ctx_delete(Jxta_object * jo)
{
    Jxta_query_context *jc = (Jxta_query_context *) jo;
    JXTA_OBJECT_RELEASE(jc->queries);
    JXTA_OBJECT_RELEASE(jc->documentName);
    JXTA_OBJECT_RELEASE(jc->sort);
    JXTA_OBJECT_RELEASE(jc->prefix);
    JXTA_OBJECT_RELEASE(jc->name);
    JXTA_OBJECT_RELEASE(jc->nameSpace);
    JXTA_OBJECT_RELEASE(jc->element);
    JXTA_OBJECT_RELEASE(jc->attribName);
    JXTA_OBJECT_RELEASE(jc->value);
    JXTA_OBJECT_RELEASE(jc->query);
    JXTA_OBJECT_RELEASE(jc->queryNameSpace);
    JXTA_OBJECT_RELEASE(jc->sqlcmd);
    JXTA_OBJECT_RELEASE(jc->sqloper);
    JXTA_OBJECT_RELEASE(jc->level2);
    JXTA_OBJECT_RELEASE(jc->newQuery);
    JXTA_OBJECT_RELEASE(jc->origQuery);
    if (NULL != jc->xpathcomp) {
        xmlXPathFreeCompExpr(jc->xpathcomp);
    }
    if (jc->xpathctx) {
        xmlXPathFreeContext(jc->xpathctx);
    }
    memset(jc, 0xdd, sizeof(Jxta_query_context));
    free(jc);
}

static void jxta_query_entry_delete(Jxta_object * obj)
{
    Jxta_query_element *qel = (Jxta_query_element *) obj;
    JXTA_OBJECT_RELEASE(qel->jSQL);
    JXTA_OBJECT_RELEASE(qel->jBoolean);
    JXTA_OBJECT_RELEASE(qel->jOper);
    JXTA_OBJECT_RELEASE(qel->jName);
    JXTA_OBJECT_RELEASE(qel->jValue);
    free(qel);
}

/**
* Create a new JXTA query context. The query is an XPath type query.
* See document at http://jxta.org
* @fixme specify the document at the web site
**/
JXTA_DECLARE(Jxta_query_context *) jxta_query_new(const char *q)
{
    Jxta_status status;
    Jxta_query_context *jctx = (Jxta_query_context *) calloc(1, sizeof(Jxta_query_context));

    JXTA_OBJECT_INIT(jctx, (JXTA_OBJECT_FREE_FUNC) jxta_query_ctx_delete, 0);
    jctx->queries = jxta_vector_new(1);
    jctx->documentName = jstring_new_0();
    jctx->sort = jstring_new_0();
    jctx->prefix = jstring_new_0();
    jctx->name = jstring_new_0();
    jctx->nameSpace = jstring_new_0();
    jctx->element = jstring_new_0();
    jctx->attribName = jstring_new_0();
    jctx->value = jstring_new_0();
    jctx->negative = FALSE;
    jctx->query = jstring_new_0();
    jctx->queryNameSpace = jstring_new_0();
    jctx->origQuery = jstring_new_2(q);
    jctx->sqlcmd = jstring_new_2(SQL_SELECT);
    jstring_append_2(jctx->sqlcmd, SQL_ASTRIX);
    jstring_append_2(jctx->sqlcmd, SQL_FROM);
    jstring_append_2(jctx->sqlcmd, CM_TBL_ELEM_ATTRIBUTES);
    jstring_append_2(jctx->sqlcmd, SQL_WHERE);
    jctx->newQuery = jstring_new_0();
    jctx->sqloper = jstring_new_2((const char *) "=");
    jctx->level2 = jstring_new_0();
    jctx->first = TRUE;
    jctx->levelNumber = 0;
    jctx->endOfCollection = 0;
    jctx->hasNumeric = FALSE;
    status = query_xform_query(jctx, q);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jctx);
        return NULL;
    }
    return jctx;
}

static Jxta_query_element *query_entry_new(JString * jSQL, JString * jBoolean, JString * jOper, JString * jName, JString * jValue)
{
    const char *val;
    Jxta_query_element *qel = (Jxta_query_element *) calloc(1, sizeof(Jxta_query_element));

    JXTA_OBJECT_INIT(qel, (JXTA_OBJECT_FREE_FUNC) jxta_query_entry_delete, 0);
    qel->jSQL = jstring_clone(jSQL);
    qel->jBoolean = jstring_clone(jBoolean);
    qel->jOper = jstring_clone(jOper);
    qel->jName = jstring_clone(jName);
    qel->jValue = jstring_clone(jValue);
    val = jstring_get_string(qel->jValue);
    qel->isNumeric = FALSE;
    if ('#' == *val) {
        qel->isNumeric = TRUE;
    } else {
        qel->hasWildcard = FALSE;
        while (*val) {
            if ('%' == *val || '_' == *val) {
                qel->hasWildcard = TRUE;
                break;
            }
            val++;
        }
    }
    return qel;
}
static xmlGenericErrorFunc query_err_func(void *userData, const char *msg)
{
    xmlErrorPtr err;
    int i;
    char *temp;
    char *carrot;
    JString *errString = jstring_new_0();

    err = xmlGetLastError();
    if (NULL != err->str1) {
        jstring_append_2(errString, err->str1);
        jstring_append_2(errString, "\n");
        carrot = calloc(1, err->int1 + 3);
        temp = carrot;
        for (i = 1; err->int1 > 1 && i < err->int1; i++) {
            *temp++ = ' ';
        }
        *temp++ = '^';
        *temp++ = '\n';
        *temp = '\0';
        jstring_append_2(errString, carrot);
        free(carrot);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                    "error in jxta_query domain:%d --- %s line:%d position:%d\n%s",
                    err->domain, err->message, err->line, err->int1, jstring_get_string(errString));
    JXTA_OBJECT_RELEASE(errString);
    return NULL;
}

JXTA_DECLARE(Jxta_status)
    jxta_query_XPath(Jxta_query_context * jctx, const char *advXML, Jxta_boolean bResults)
{
    const char *newQuery;
    xmlDocPtr doc;
    xmlXPathObjectPtr res = NULL;
    xmlXPathContextPtr ctxt = NULL;
    xmlXPathCompExprPtr comp = NULL;
    Jxta_status status = JXTA_SUCCESS;
    const char *docName = NULL;

    if (jctx == NULL) {
        status = JXTA_INVALID_ARGUMENT;
        return status;
    }
    jctx->found = 0;
    docName = jstring_get_string(jctx->documentName);
    if (bResults && strlen(docName) > 0) {
        query_xform_query(jctx, jstring_get_string(jctx->origQuery));
    }
    newQuery = jstring_get_string(jctx->newQuery);

    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "Query - %s\n", newQuery);

    doc = xmlReadDoc((unsigned char *) advXML, NULL, NULL, 0);
    if (doc == NULL) {
        status = JXTA_INVALID_ARGUMENT;
        return status;
    }

    /* create an XPath context */
    ctxt = xmlXPathNewContext(doc);

    if (ctxt == NULL) {
        status = JXTA_INVALID_ARGUMENT;
        xmlFreeDoc(doc);
        return status;
    }

    if (jctx->xpathctx != NULL) {
        xmlXPathFreeContext(jctx->xpathctx);
    }
    jctx->xpathctx = ctxt;

    /* start at the root node */
    ctxt->node = xmlDocGetRootElement(doc);

    xmlSetGenericErrorFunc(ctxt, (xmlGenericErrorFunc) query_err_func);

    if (NULL != jctx->xpathcomp) {
        xmlXPathFreeCompExpr(jctx->xpathcomp);
        jctx->xpathcomp = NULL;
    }
    /* compile the XPath statement */
    comp = xmlXPathCompile(BAD_CAST newQuery);

    /* if it compiles fine - evaluate the document */
    if (comp != NULL) {
        jctx->xpathcomp = comp;

        /* start at the root node */
        ctxt->node = xmlDocGetRootElement(doc);

        /* if no results are needed just return SQL */
        if (!bResults) {
            query_reset_context(jctx);
            query_SQL(jctx);
        } else {
            jxta_wildxpath_init(ctxt);
            res = jxta_wildxpath_eval_comp(ctxt, comp);
            jxta_wildxpath_cleanup(ctxt);
        }
        xmlXPathFreeCompExpr(comp);
        jctx->xpathcomp = NULL;

    } else {
        status = JXTA_INVALID_ARGUMENT;
        res = NULL;

    }                                                                                                                                                         
    /* return the result */
    if (res == NULL) {
        if (bResults) {
            status = JXTA_FAILED;
        }
    } else {
        jctx->found = 0;
        if (res->nodesetval != NULL) {
            jctx->found = res->nodesetval->nodeNr;
        }
        xmlXPathFreeObject(res);
    }
    if (doc != NULL) {
        xmlFreeDoc(doc);
    }
    return status;                                                                                                                                            
}


static Jxta_status query_xform_query(Jxta_query_context * jctx, const char *query)
{
    Jxta_status status;
    const char *stringForNameSpace = "/*[name()=\"";
    const char *newQuery = "";
    const char *name;
    const char *prefix;
    int plength;
    int nlength;

    if (*query != '/') {
        status = JXTA_INVALID_ARGUMENT;
        return status;
    }
    query++;
    status = query_find_Ns(jctx, query, TRUE);
    if (status != JXTA_SUCCESS) {
        return status;
    }
    newQuery = jstring_get_string(jctx->query);
    name = jstring_get_string(jctx->name);
    prefix = jstring_get_string(jctx->prefix);

    plength = strlen(prefix);
    nlength = strlen(name);
    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG,
                    "------------------ ? level2: %s \n", jstring_get_string(jctx->level2));
    jstring_reset(jctx->newQuery, NULL);
    if (plength > 0 && nlength > 0) {
        const char *docName;
        docName = jstring_get_string(jctx->documentName);
        if (strlen(docName) > 0) {
            Jxta_boolean replaceName=FALSE;
            Jxta_boolean replacePrefix=FALSE;
            if ('*' == *name) replaceName = TRUE;
            if ('*' == *prefix) replacePrefix = TRUE;
            query_split_Ns(jctx, docName, replacePrefix, replaceName);
            name = jstring_get_string(jctx->name);
            prefix = jstring_get_string(jctx->prefix);
        }
        jstring_append_2(jctx->newQuery, stringForNameSpace);
        jstring_append_2(jctx->newQuery, prefix);
        jstring_append_2(jctx->newQuery, ":");
        jstring_append_2(jctx->newQuery, name);
        jstring_append_2(jctx->newQuery, "\"]");
        jstring_append_2(jctx->newQuery, newQuery);
    } else {
        jstring_append_2(jctx->newQuery, "/*");
        jstring_append_2(jctx->newQuery, query);
        return status;
    }
    return status;
}

static Jxta_status query_find_Ns(Jxta_query_context * jctx, const char *q, Jxta_boolean appendQuery)
{
    Jxta_boolean bns = FALSE;
    int i = 0;

    jstring_reset(jctx->prefix, NULL);
    jstring_reset(jctx->name, NULL);

    for (i = 0; q[i] != '\0' && q[i] != ':' && q[i] != '[' && q[i] != '/'; i++);
    if (':' == q[i]) {
        jstring_append_0(jctx->prefix, q, i);
        q += i + 1;
        for (i = 0; q[i] != '\0' && q[i] != '[' && q[i] != '/'; i++);
    }
    jstring_append_0(jctx->name, q, i);
    q += i;

    if (appendQuery) {
        jstring_reset(jctx->query, NULL);
        jstring_append_2(jctx->query, q);
    }
/*    jxta_query_find_level2(jctx, q); */
    return JXTA_SUCCESS;
}

static void query_split_Ns(Jxta_query_context * jctx, const char *q, Jxta_boolean bPrefix, Jxta_boolean bName)
{
    int i = 0;

    for (i = 0; q[i] != '\0' && q[i] != ':'; i++);
    if (':' == q[i]) {
        if (bPrefix) {
            jstring_reset(jctx->prefix, NULL);
            jstring_append_0(jctx->prefix, q, i);
        }
        q += i + 1;
    }
    if (bName) {
        jstring_reset(jctx->name, NULL);
        jstring_append_2(jctx->name, q);
    }
}

static Jxta_status query_SQL(Jxta_query_context * jctx)
{
    int i;
    xmlXPathCompExprPtr comp;
    JString * copy = NULL;
    JString * nameSpace = NULL;

    comp = jctx->xpathcomp;
    i = comp->last;
    jstring_reset(jctx->sqlcmd, NULL);
    query_create_SQL(jctx, comp, &comp->steps[i], 1);
    if (!jctx->first) {
        copy = jstring_new_0();
        jstring_append_1(copy, jctx->sqlcmd);
        jstring_reset(jctx->sqlcmd, NULL);
    }
    jstring_append_2(jctx->sqlcmd, CM_COL_SRC);
    jstring_append_2(jctx->sqlcmd, SQL_DOT);
    jstring_append_2(jctx->sqlcmd, CM_COL_NameSpace);
    nameSpace = jstring_new_0();
    jstring_append_1(nameSpace, jctx->queryNameSpace);
    if (cm_sql_escape_and_wc_value(nameSpace, TRUE)) {
        jstring_append_2(jctx->sqlcmd, SQL_LIKE);
    } else {
        jstring_append_2(jctx->sqlcmd, SQL_EQUAL);
    }
    SQL_VALUE(jctx->sqlcmd, nameSpace);
    if (NULL != copy) {
        jstring_append_2(jctx->sqlcmd, SQL_AND);
        jstring_append_2(jctx->sqlcmd, SQL_LEFT_PAREN);
        jstring_append_1(jctx->sqlcmd, copy);
        jstring_append_2(jctx->sqlcmd, SQL_RIGHT_PAREN);
        JXTA_OBJECT_RELEASE(copy);
    }
    if (NULL != nameSpace) {
        JXTA_OBJECT_RELEASE(nameSpace);
    }
    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s\n", jstring_get_string(jctx->sqlcmd));
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void)
    jxta_query_build_SQL_operator(JString * elem, JString * sqloper, JString * value, JString * result)
{
    const char *val;
    Jxta_boolean isNumeric = FALSE;

    /* for attributes the name waits */
    jstring_append_2(result, SQL_LEFT_PAREN);
    jstring_append_2(result, CM_COL_SRC);
    jstring_append_2(result, SQL_DOT);
    jstring_append_2(result, CM_COL_Name);
    jstring_append_2(result, SQL_EQUAL);
    SQL_VALUE(result, elem);
    jstring_append_2(result, SQL_AND);
    jstring_append_2(result, CM_COL_SRC);
    jstring_append_2(result, SQL_DOT);
    val = jstring_get_string(value);
    if (val && '#' == *val) {
        isNumeric = TRUE;
        jstring_append_2(result, CM_COL_NumValue);
    } else {
        jstring_append_2(result, CM_COL_Value);
    }
    if (cm_sql_escape_and_wc_value(value, TRUE)) {
        if (!strcmp(jstring_get_string(sqloper), SQL_EQUAL)) {
            jstring_reset(sqloper, NULL);
            jstring_append_2(sqloper, SQL_LIKE);
        } else {
            jstring_reset(sqloper, NULL);
            jstring_append_2(sqloper, SQL_NOT_LIKE);
        }
    }
    jstring_append_1(result, sqloper);
    if (isNumeric) {
        SQL_NUMERIC_VALUE(result, value);
    } else {
        SQL_VALUE(result, value);
    }
    jstring_append_2(result, SQL_RIGHT_PAREN);
}

/**
* The only values that should be reset are those that are used going through the step table
*/
static void query_reset_context(Jxta_query_context * jctx)
{
    jstring_reset(jctx->sort, NULL);
    jstring_reset(jctx->prefix, NULL);
    jstring_reset(jctx->name, NULL);
    jstring_reset(jctx->nameSpace, NULL);
    jstring_reset(jctx->element, NULL);
    jctx->negative = FALSE;
    jstring_reset(jctx->value, NULL);
    jstring_reset(jctx->sqlcmd, NULL);
    jstring_reset(jctx->sqloper, NULL);
    jctx->first = TRUE;
    jctx->endOfCollection = 0;
    jctx->currLevel = 0;
}

static Jxta_status query_create_SQL(Jxta_query_context * jctx, xmlXPathCompExprPtr comp, xmlXPathStepOpPtr op, int depth)
{
    int i, j;
    JString *elem;
    char shift[100];
    
    for (i = 0; ((i < depth) && (i < 25)); i++)
        shift[2 * i] = shift[2 * i + 1] = ' ';
    shift[2 * i] = shift[2 * i + 1] = 0;
    /*  start at the end an work your way forward */
    for (j = comp->last; j > 0; j--) {
        if (j < jctx->endOfCollection)
            jstring_reset(jctx->element, NULL);
        op = &comp->steps[j];
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "step %i  ch1 %i  ch2 %i\n", j, op->ch1, op->ch2);
        if (op == NULL) {
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "Step is NULL\n");
            return JXTA_FAILED;
        }
        switch (op->op) {

        case XPATH_OP_END:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sEND\n", shift);
            break;
        case XPATH_OP_AND:
            jstring_reset(jctx->sort, NULL);
            jstring_append_2(jctx->sort, SQL_AND);
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sAND\n", shift);
            break;
        case XPATH_OP_OR:
            jstring_reset(jctx->sort, NULL);
            jstring_append_2(jctx->sort, SQL_OR);
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sOR\n", shift);
            break;
        case XPATH_OP_EQUAL:
            jstring_reset(jctx->sqloper, NULL);
            if (op->value) {
                jstring_append_2(jctx->sqloper, SQL_EQUAL);
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sEQUAL \n", shift);
            } else {
                jstring_append_2(jctx->sqloper, SQL_NOT_EQUAL);
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sEQUAL !=\n", shift);
            }
            break;
        case XPATH_OP_CMP:
            jstring_reset(jctx->sqloper, NULL);
            if (op->value) {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sCMP <\n", shift);
                jstring_append_2(jctx->sqloper, SQL_LESS_THAN);
            } else {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sCMP >\n", shift);
                jstring_append_2(jctx->sqloper, SQL_GREATER_THAN);
            }
            if (!op->value2) {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s=\n", shift);
                jstring_append_2(jctx->sqloper, SQL_EQUAL);
            }
            break;
        case XPATH_OP_PLUS:
            if (op->value == 0) {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sPLUS -\n", shift);
                jctx->negative = TRUE;
            } else if (op->value == 1) {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sPLUS +\n", shift);
                jctx->negative = FALSE;
            } else if (op->value == 2) {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sPLUS unary -\n", shift);
                jctx->negative = TRUE;
            } else if (op->value == 3) {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sPLUS unary - -\n", shift);
                jctx->negative = TRUE;
            }
            break;
        case XPATH_OP_MULT:
            if (op->value == 0)
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sMULT *\n", shift);
            else if (op->value == 1)
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sMULT div\n", shift);
            else
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sMULT mod\n", shift);
            break;
        case XPATH_OP_UNION:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sUNION\n", shift);
            break;
        case XPATH_OP_ROOT:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sROOT\n", shift);
            break;
        case XPATH_OP_NODE:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sNODE\n", shift);
            break;
        case XPATH_OP_RESET:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sRESET\n", shift);
            break;
        case XPATH_OP_SORT:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sSORT\n", shift);
            break;

        case XPATH_OP_COLLECT:
            {
                xmlXPathAxisVal axis = (xmlXPathAxisVal) op->value;
                xmlXPathTestVal test = (xmlXPathTestVal) op->value2;
                xmlXPathTypeVal type = (xmlXPathTypeVal) op->value3;
                const xmlChar *prefix = (const xmlChar *) op->value4;
                const xmlChar *name = (const xmlChar *) op->value5;

                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sCOLLECT\n", shift);
                switch (axis) {
                case AXIS_ANCESTOR:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'ancestors' \n", shift);
                    break;
                case AXIS_ANCESTOR_OR_SELF:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'ancestors-or-self'\n ", shift);
                    break;
                case AXIS_ATTRIBUTE:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'attributes' \n", shift);
                    break;
                case AXIS_CHILD:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'child' \n", shift);
                    break;
                case AXIS_DESCENDANT:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'descendant' \n", shift);
                    break;
                case AXIS_DESCENDANT_OR_SELF:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'descendant-or-self' \n", shift);
                    break;
                case AXIS_FOLLOWING:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'following' \n", shift);
                    break;
                case AXIS_FOLLOWING_SIBLING:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'following-siblings' \n", shift);
                    break;
                case AXIS_NAMESPACE:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'namespace' \n", shift);
                    break;
                case AXIS_PARENT:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'parent' \n", shift);
                    break;
                case AXIS_PRECEDING:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'preceding' \n", shift);
                    break;
                case AXIS_PRECEDING_SIBLING:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'preceding-sibling' \n", shift);
                    break;
                case AXIS_SELF:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'self' \n", shift);
                    break;
                }
                switch (test) {
                case NODE_TEST_NONE:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'none' \n", shift);
                    break;
                case NODE_TEST_TYPE:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'type' \n", shift);
                    break;
                case NODE_TEST_PI:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'PI' \n", shift);
                    break;
                case NODE_TEST_ALL:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'all' \n", shift);
                    break;
                case NODE_TEST_NS:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'namespace' \n", shift);
                    break;
                case NODE_TEST_NAME:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'name' \n", shift);
                    break;
                }

                switch (type) {
                    JString *jSQL;
                    Jxta_query_element *jEntry;
                case NODE_TYPE_NODE:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s 'node' \n", shift);
                    jctx->bAttribute = FALSE;
                    if (name != NULL && axis != AXIS_ATTRIBUTE) {
                        jstring_reset(jctx->element, NULL);
                        if (prefix != NULL) {
                            jstring_append_2(jctx->element, (const char *) prefix);
                            jstring_append_2(jctx->element, ":");
                        }
                        jstring_append_2(jctx->element, (const char *) name);
                        jctx->endOfCollection = op->ch1;
                    } else if (axis == AXIS_ATTRIBUTE) {
                        jctx->bAttribute = TRUE;
                        jstring_reset(jctx->attribName, NULL);
                        jstring_append_2(jctx->attribName, " ");
                        jstring_append_2(jctx->attribName, (const char *) name);

                    }
                    if (0 == jstring_length(jctx->sqloper))
                        break;
                    if (jctx->first) {
                        jctx->first = FALSE;
                    } else {
                        jstring_append_2(jctx->sqlcmd, SQL_OR);
                    }
                    elem = jstring_new_0();
                    jstring_append_1(elem, jctx->element);
                    jSQL = jstring_new_0();
                    if (TRUE == jctx->bAttribute) {
                        jstring_append_1(elem, jctx->attribName);
                        jxta_query_build_SQL_operator(elem, jctx->sqloper, jctx->value, jSQL);
                        jctx->bAttribute = FALSE;

                    } else {
                        jxta_query_build_SQL_operator(elem, jctx->sqloper, jctx->value, jSQL);
                    }
                    jEntry = query_entry_new(jSQL, jctx->sort, jctx->sqloper, elem, jctx->value);
                    jxta_vector_add_object_first(jctx->queries, (Jxta_object *) jEntry);
                    jstring_append_1(jctx->sqlcmd, jSQL);
                    JXTA_OBJECT_RELEASE(jSQL);
                    JXTA_OBJECT_RELEASE(jEntry);
                    jstring_reset(jctx->value, NULL);
                    jstring_reset(jctx->sqloper, NULL);
                    JXTA_OBJECT_RELEASE(elem);
                    elem = NULL;
                    break;
                case NODE_TYPE_COMMENT:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s'comment' \n", shift);
                    break;
                case NODE_TYPE_TEXT:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s'text' \n", shift);
                    break;
                case NODE_TYPE_PI:
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%s'PI' \n", shift);
                    break;
                }
                if (prefix != NULL)
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "prefix - %s%s:\n", shift, prefix);
                if (name != NULL)
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "name - %s%s\n", shift, (const char *) name);
                break;

            }
        case XPATH_OP_VALUE:
            {
                xmlXPathObjectPtr object = (xmlXPathObjectPtr) op->value4;

                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sELEM \n", shift);
                query_examine_object(jctx, object, depth);
            }
        case XPATH_OP_VARIABLE:
            {
                const xmlChar *prefix = (const xmlChar *) op->value5;
                const xmlChar *name = (const xmlChar *) op->value4;

                if (prefix != NULL)
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sVARIABLE %s:%s\n", shift, prefix, name);
                else if (name != NULL)
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sVARIABLE %s\n", shift, name);
                else
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG,
                                    "%sVARIABLE ---------- has no name or prefix --------\n", shift);
                break;
            }
        case XPATH_OP_FUNCTION:
            {
                int nbargs = op->value;
                const xmlChar *prefix = (const xmlChar *) op->value5;
                const xmlChar *name = (const xmlChar *) op->value4;
                const xmlChar *nameCheck = (xmlChar *) "name";
                if (xmlStrEqual(name, nameCheck)) {
                    jstring_reset(jctx->element, NULL);
                    jstring_append_2(jctx->element, "NameSpace");
                }
                jstring_reset(jctx->nameSpace, NULL);
                if (prefix != NULL) {
                    jstring_append_2(jctx->nameSpace, (const char *) prefix);
                    jstring_append_2(jctx->nameSpace, ":");
                    jstring_append_2(jctx->nameSpace, (const char *) name);
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG,
                                    "%sFUNCTION %s:%s(%d args)\n", shift, prefix, name, nbargs);
                } else {
                    jstring_append_2(jctx->nameSpace, (const char *) name);
                    jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sFUNCTION %s(%d args)\n", shift, name, nbargs);
                }
                if (jstring_length(jctx->value) == 0) {
                    break;
                }
                jstring_reset(jctx->queryNameSpace, NULL);
                jstring_append_1(jctx->queryNameSpace, jctx->value);

                break;
            }
        case XPATH_OP_ARG:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sARG\n", shift);
            break;
        case XPATH_OP_PREDICATE:
            {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sPREDICATE\n", shift);
                jctx->currLevel++;
                break;
            }
        case XPATH_OP_FILTER:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sFILTER\n", shift);
            break;
        default:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sUNKNOWN %d\n", shift, op->op);
            return JXTA_FAILED;
        }
    }
    return JXTA_SUCCESS;
}

static void query_examine_object(Jxta_query_context * jctx, xmlXPathObjectPtr cur, int depth)
{
    int i;
    char shift[100];
    FILE *output = stdout;

    if (output == NULL)
        return;

    for (i = 0; ((i < depth) && (i < 25)); i++)
        shift[2 * i] = shift[2 * i + 1] = ' ';
    shift[2 * i] = shift[2 * i + 1] = 0;

    if (cur == NULL) {
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "Object is empty (NULL)\n");
        return;
    }
    switch (cur->type) {
    case XPATH_STRING:
        jstring_reset(jctx->value, NULL);
        jstring_append_2(jctx->value, (const char *) cur->stringval);
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a string : %s\n", shift, cur->stringval);
        break;
    case XPATH_UNDEFINED:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a XPATH_UNDEFINED : %s\n", shift, cur->stringval);
        break;
    case XPATH_NODESET:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a XPATH_NODESET : %s\n", shift, cur->stringval);
        break;
    case XPATH_BOOLEAN:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a XPATH_BOOLEAN : %s\n", shift, cur->stringval);
        break;
    case XPATH_NUMBER:
        switch (xmlXPathIsInf(cur->floatval)) {
        case 1:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "Object is a number : Infinity\n");
            break;
        case -1:
            jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "Object is a number : -Infinity\n");
            break;
        default:
            if (isnan(cur->floatval)) {
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "Object is a number : NaN\n");
            } else {
                xmlChar *fromXPath;
                jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "Object is a number : %0g\n", cur->floatval);
                jstring_append_2(jctx->value, "#");
                if (TRUE == jctx->negative) {
                    jstring_append_2(jctx->value, "-");
                }
                fromXPath = xmlXPathCastToString(cur);
                jstring_append_2(jctx->value, (char const *) fromXPath);
                xmlFree(fromXPath);
                jctx->hasNumeric = TRUE;
            }
            break;
        }
        break;
    case XPATH_POINT:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a XPATH_POINT : %s\n", shift, cur->stringval);
        break;
    case XPATH_RANGE:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a XPATH_RANGE : %s\n", shift, cur->stringval);
        break;
    case XPATH_LOCATIONSET:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG,
                        "%sObject is a XPATH_LOCATIONSET : %s\n", shift, cur->stringval);
        break;
    case XPATH_USERS:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a XPATH_USERS : %s\n", shift, cur->stringval);
        break;
    case XPATH_XSLT_TREE:
        jxta_log_append(ENHANCED_QUERY_LOG, JXTA_LOG_LEVEL_DEBUG, "%sObject is a `XPATH_XSLT_TREE : %s\n", shift, cur->stringval);
        break;
    }
}

/**
 * jxta_wildxpath_init:
 * @xpathCtx:       The XPath context that will be initialized for wildcard matching.
 *
 * Prepares an XPath context to be used for wildcard matcing.  Registers the wildcard
 * handler callback function and associates it with the tag "wild".  If the XPath parser
 * encounters a function call in the form "wild(arg1, arg2)" it will call the hander callback.
 *
 * Returns 0 on success and a negative value otherwise.
 */
JXTA_DECLARE(int)
    jxta_wildxpath_init(const xmlXPathContextPtr xpathCtx)
{
    int status = -1;

    assert(xpathCtx);

    if (xpathCtx)
    {
        /* Add the handler to the list of xpath functions.  Associate it with the token "wild" */
        status = xmlXPathRegisterFunc(xpathCtx, (xmlChar *)"wild", jxta_wildxpath_handler);
    }
    return(status);
}


/**
 * jxta_wildxpath_cleanup:
 * @xpathCtx:       The XPath context that will be have wildcard matching uninitialized.
 *
 * Removes wildcard matching capabilities from an XPath context.
 *
 * Returns 0 on success and a negative value otherwise.
 */
JXTA_DECLARE(int)
    jxta_wildxpath_cleanup(const xmlXPathContextPtr xpathCtx)
{
    int status = -1;

    assert(xpathCtx);

    if (xpathCtx)
    {
        /*
         * Remove the handler to the list of xpath functions.  Disassociate it with
         * the token "wild". Note: passing NULL to xmlXPathRegisterFunc will delist whatever is assoicated
         * with "wild".
         */
        status = xmlXPathRegisterFunc(xpathCtx, (xmlChar *)"wild", NULL);
    }
    return(status);
}

/**
 * jxta_wildxpath_handler:
 * @ctxt:       the XPath context that is being used for wildcard matching
 * @nargs:      number of arguments for the "wild" function.
 *
 * Callback handler function for XPath wildcards.  This function should only be called by libxml.
 * If the XPath parser encounters a function call in the form "wild(arg1, arg2)" it
 * will call the hander callback.  This handler function was registered
 * in the call to jxta_wildxpath_init().  Be sure to call
 * jxta_wildxpath_init() before attempting to evaluate an XPath expression with wildcards. 
 * 
 * Returns NONE
 */
static void jxta_wildxpath_handler(xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlNodeSetPtr arg1 = NULL;      /* First argument from stack (third popped) */
    xmlXPathObjectPtr arg2 = NULL;  /* Second argument from stack (second popped) */
    xmlXPathObjectPtr arg3 = NULL;  /* Third argument from stack (first popped) */
    xmlNodeSetPtr cur = NULL;       /* Result nodeset */
    int i = 0;
    int want_match = 0;
    int is_match = 0;

    /*
     * Note: arguments are passed on libxml's internal stack.  We need
     * to use libxml's functions to pop argments off the stack and push results on.
     *
     * Note: the prototype of the handler must contain ctxt as the first arg.  Libxml
     * defines macros which expect it.
     *
     */

    assert(ctxt);

    /* Are there two arguments waiting on the stack for us? */
    CHECK_ARITY(3); 

    /* Is third argument a number? */
    CHECK_TYPE(XPATH_NUMBER); 
    arg3 = valuePop(ctxt);
    want_match = arg3->floatval == 1.0;

    /* Is second argument a string? */
    CHECK_TYPE(XPATH_STRING); 
    arg2 = valuePop(ctxt);

    /* Is the first argument a nodeset? */
    CHECK_TYPE(XPATH_NODESET); 
    arg1 = xmlXPathPopNodeSet(ctxt);

    /* Get ready to store the result */
    cur = xmlXPathNodeSetCreate(NULL); 

    /* Loop thru the node set */
    for(; i < arg1->nodeNr; i++)
    {
        /* Attempt to match pattern */
        is_match = apr_fnmatch((char *)arg2->stringval, (char *)arg1->nodeTab[i]->children->content, 0) == 0;

        /* Does the content of the current node match the wildcard expression? */
        if(want_match == is_match)
        {
            /* Yes, add the node to the resulting nodeset */
            xmlXPathNodeSetAdd(cur, arg1->nodeTab[i]);
        }
    }

    /* Put the resulting nodeset on the libxml stack. */
    valuePush(ctxt, xmlXPathWrapNodeSet(cur));

    /* Cleanup arguments */
    xmlXPathFreeNodeSet(arg1);  
    xmlXPathFreeObject(arg2);
    xmlXPathFreeObject(arg3);

    return;
}
 
 /**
 * jxta_wildxpath_eval_expr:
 * @xpathCtx:   the XPath context that is being used for wildcard matching
 * @xpathExpr:  the XPath expression to be evaluated.
 *
 * Evaluates an XPath expression containing wildcards.  Be sure to call
 * jxta_wildxpath_init() before attempting to evaluate an XPath expression with wildcards and
 * jxta_wildxpath_cleanup() when you are done evaluating XPath expressions on a context.
 * 
 * Returns xmlXPathObjectPtr pointer to a node list resulting from evaluating the expression.
 */
JXTA_DECLARE(xmlXPathObjectPtr)
    jxta_wildxpath_eval_expr(const xmlXPathContextPtr xpathCtx, const xmlChar* xpathExpr)
{
    xmlXPathCompExprPtr incomp = NULL;  /* Compiled XPath expression */
    xmlXPathObjectPtr outobj = NULL;    /* Resulting nodeset */

    assert(xpathCtx);
    assert(xpathExpr);

    incomp = xmlXPathCompile(BAD_CAST xpathExpr);           /* Compile the XPath expression. */ 
    outobj = jxta_wildxpath_eval_comp(xpathCtx, incomp);    /* Evaluate the compiled expression */
    xmlXPathFreeCompExpr(incomp);                           /* Cleanup the compiled expression */
    return outobj;  /* Give matching nodeset to caller. */
}

 /**
 * jxta_wildxpath_append_subexpr:
 * @subexpr:   the subexpression that you want to copy to the end of the XPath expression.
 * @outcomp:  the resulting compiled ouput.
 * @subcomps:  the caller's pointer or the compiled subexpression
 * @i: Index of caller's position into the main compiled expresssion.
 * @s:  Index into subcomps
 * @start: Position where compiled subexpression got appended.
 * @last: Position where compiled subexpression gets appended.
 *
 * Copies a compiled XPath subexpression onto the end of an XPath expression and links them together.
 *
 * Returns None.
 */
 static void jxta_wildxpath_append_subexpr(const char* subexpr, xmlXPathCompExprPtr outcomp, xmlXPathCompExprPtr* subcomps, int i, int* s, int* start, int* last)
 {
    int j = 0;

    assert(subexpr);
    assert(outcomp);
    assert(subcomps);
    assert(s);
    assert(start);
    assert(last);

    subcomps[*s] = xmlXPathCompile((xmlChar*)subexpr);

    /* Append the equivalent expresison to the end of the output */
    for(j = 0, *start = *last; j < subcomps[*s]->last; j++, (*last)++)
    {
        outcomp->steps[*last] = subcomps[*s]->steps[j];

        /* Adjust child references */
        if(outcomp->steps[*last].ch1 >= 0)
        {
            outcomp->steps[*last].ch1 += *start;
        }
        if(outcomp->steps[*last].ch2 >= 0)
        {
            outcomp->steps[*last].ch2 += *start;
        }
    }

    /* Make the original compiled expression point to the subexpression */
    outcomp->steps[i] = outcomp->steps[*last-1];
    s++;
} 


 /**
 * jxta_wildxpath_haswildcard:
 * @pattern:   search string.
 *
 * Tests a search string for any '*' or '?' wildcards.
 * 
 * Returns 1 if the pattern has a wildcard, 0 otherwise.
 */
static int
jxta_wildxpath_haswildcard(const char* pattern)
{
    int i = 0;
    int haswild = 0;

    assert(pattern);

    while(pattern[i] != 0)
    {
        if(pattern[i] == '*' || pattern[i] == '?')
        {
            haswild = 1;
            break;
        }
        i++;
    }

    return haswild;
}


 /**
 * jxta_wildxpath_eval_comp:
 * @xpathCtx:   the XPath context that is being used for wildcard matching
 * @incomp:     the compiled XPath expression to be evaluated.
 *
 * Evaluates an XPath compiled expression containing wildcards.  Be sure to call
 * jxta_wildxpath_init() before attempting to evaluate an XPath expression with wildcards and
 * jxta_wildxpath_cleanup() when you are done evaluating XPath expressions on a context.
 * 
 * This function takes a compiled expression and checks for comparisions between a nodeset and a string.
 * If it finds a comparision between the two, it creates an equivalent compiled expression
 * that calls the wildcard function handler.
 * 
 * Returns xmlXPathObjectPtr pointer to a node list resulting from evaluating the expression.
 */
JXTA_DECLARE(xmlXPathObjectPtr)
    jxta_wildxpath_eval_comp(const xmlXPathContextPtr xpathCtx, const xmlXPathCompExprPtr incomp)
{
    xmlXPathCompExprPtr* subcomps = NULL; /* Intermediate compiled xpath sub expressions.  Used to generate function call steps. */
    xmlXPathCompExprPtr outcomp = incomp; /* The equivalent compiled expression containing calls to wildcard handler. */
    xmlXPathStepOp* in = NULL;          /* Curent node */
    xmlXPathStepOp* ch1 = NULL;         /* First child of current node */
    xmlXPathStepOp* ch2 = NULL;         /* Second child of current node */
    xmlXPathObjectPtr obj = NULL;       /* Intermediate libxml object for holding a string */
    xmlXPathObjectPtr outobj = NULL;    /* Resulting node set */
    char subexpr[100] = "";             /* String to hold a sub expression */
    int last = 0;                       /* Position where compiled subexpression gets appended. */
    int start = 0;                      /* Position where compiled subexpression got appended. */
    int i = 0;                          /* Iterator */
    int n = 0;                          /* Count of equals operators */
    int s = 0;                          /* Index into subcomps */

    assert(xpathCtx);
    assert(incomp);

    /* Get rough guesstimate of how many equals operators we may need to evaluate. */
    for(i = 0, n = 0; i < incomp->nbStep; i++)
    {
        in = &incomp->steps[i]; /* Get current step */
        n += in->op == XPATH_OP_EQUAL;
    }

    /* If contains equals operator */
    if(n > 0) 
    {
        /* Allocate memory for compiled subexpresions */
        subcomps = (xmlXPathCompExprPtr*) calloc(n, sizeof(xmlXPathCompExprPtr));

        /* Allocate memory for results */
        outcomp = (xmlXPathCompExprPtr) calloc(1, sizeof(xmlXPathCompExpr));
        outcomp->steps = (xmlXPathStepOp*) calloc(incomp->nbStep + n * NB_WILD_STEPS, sizeof(xmlXPathStepOp));

        /* Calculate final step counts */
        outcomp->nbStep = incomp->nbStep + (n * NB_WILD_STEPS) - (n * NB_WILD_STEPS - 2) + 1;
        outcomp->maxStep = outcomp->nbStep;

        /* Initialize append position */
        outcomp->last = incomp->last;
        last = incomp->nbStep;

        /* Loop thru compiled input expression */
        for(i = 0; i < incomp->nbStep; i++)
        {
            /* Copy the step */
            in = &incomp->steps[i];
            outcomp->steps[i] = *in;

            /* If the step is an equals (with a wildcard) */
            if(in->op == XPATH_OP_EQUAL)
            {
                ch1 = &incomp->steps[in->ch1];
                ch2 = &incomp->steps[in->ch2];
                obj = (xmlXPathObjectPtr) ch2->value4; /* Get the object containing any string value */

                if(ch1->op == XPATH_OP_COLLECT && ch2->op == XPATH_OP_VALUE && obj->type == XPATH_STRING && ch1->value == AXIS_CHILD)
                {
                    if(jxta_wildxpath_haswildcard((char*)obj->stringval))
                    {
                        obj = (xmlXPathObjectPtr) ch2->value4;
                        sprintf(subexpr, " wild(%s,'%s',%d) ", (char*) ch1->value5, obj->stringval, in->value);
                        jxta_wildxpath_append_subexpr(subexpr, outcomp, subcomps, i, &s, &start, &last);
                    }
                }
                else if(ch1->op == XPATH_OP_COLLECT && ch2->op == XPATH_OP_VALUE && obj->type == XPATH_STRING && ch1->value == AXIS_ATTRIBUTE)
                {
                    if(jxta_wildxpath_haswildcard((char*)obj->stringval))
                    {
                        obj = (xmlXPathObjectPtr) ch2->value4;
                        sprintf(subexpr, " wild(@%s,'%s',%d) ", (char*) ch1->value5, obj->stringval, in->value);
                        jxta_wildxpath_append_subexpr(subexpr, outcomp, subcomps, i, &s, &start, &last);
                    }
                }
                else if(ch1->op == XPATH_OP_COLLECT && ch1->value == AXIS_ATTRIBUTE && ch2->op == XPATH_OP_COLLECT && ch2->value == AXIS_CHILD)
                {
                    if(jxta_wildxpath_haswildcard((char*) ch2->value5))
                    {
                        sprintf(subexpr, " wild(@%s,'%s',%d) ", (char*) ch1->value5, (char*) ch2->value5, in->value);
                        jxta_wildxpath_append_subexpr(subexpr, outcomp, subcomps, i, &s, &start, &last);
                    }
                }
                else if(ch1->op == XPATH_OP_NODE && ch2->op == XPATH_OP_VALUE && obj->type == XPATH_STRING)
                {
                    if(jxta_wildxpath_haswildcard((char*)obj->stringval))
                    {
                        obj = (xmlXPathObjectPtr) ch2->value4;
                        sprintf(subexpr, " wild(.,'%s',%d) ", obj->stringval, in->value);
                        jxta_wildxpath_append_subexpr(subexpr, outcomp, subcomps, i, &s, &start, &last);
                    }
                }
            }
        }
    }

/*#define _DUMP_EXPRESSIONS*/
#ifdef _DUMP_EXPRESSIONS
    FILE* fp;
    fp = fopen("expressions.txt", "w+");
    xmlXPathDebugDumpCompExpr(fp, incomp, 0);
    for(i = 0; i < s; i++) 
    {
        xmlXPathDebugDumpCompExpr(fp, subcomps[i], 0);
    }
    xmlXPathDebugDumpCompExpr(fp, outcomp, 0);
    fclose(fp);
#endif

    /* Evaluate the output expression and get the resulting nodeset. */
    outobj = xmlXPathCompiledEval(outcomp, xpathCtx);

    /* Free up objects */
    for(i = 0; i < s; i++) 
    {
        if(subcomps[i] != NULL)
        {
            xmlXPathFreeCompExpr(subcomps[i]);
        }
    }

    if(subcomps != NULL)
    {
        free(subcomps);
    }

    if(outcomp != NULL && outcomp != incomp)
    {
        if(outcomp->steps != NULL)
        {
            free(outcomp->steps);
        }
        free(outcomp);
    }

    return outobj;
}


/* vim: set ts=4 sw=4 et tw=130: */
