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
 * $Id: jxta_query.c,v 1.25 2006/10/31 18:12:47 slowhog Exp $
  *
  */
#include <stdlib.h>
#include <math.h>
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
            res = xmlXPathCompiledEval(comp, ctxt);
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

/* vim: set ts=4 sw=4 et tw=130: */
