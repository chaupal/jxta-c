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

/* some useful defines for SQL */

#define SQL_SELECT " SELECT "
#define SQL_UPDATE " UPDATE "
#define SQL_INSERT_INTO " INSERT INTO "
#define SQL_DROP_TABLE "DROP TABLE "
#define SQL_DELETE "DELETE FROM "
#define SQL_SET " SET "
#define SQL_VALUES " VALUES "
#define SQL_FROM " FROM "
#define SQL_JOIN " JOIN "
#define SQL_INNER " INNER "
#define SQL_OUTER " OUTER "
#define SQL_WHERE " WHERE "
#define SQL_ORDER " ORDER BY "
#define SQL_GROUP " GROUP BY "
#define SQL_ON " ON "
#define SQL_DOT "."
#define SQL_AND " AND "
#define SQL_OR " OR "
#define SQL_ASTRIX " * "
#define SQL_LEFT_PAREN "("
#define SQL_RIGHT_PAREN ")"
#define SQL_COMMA ","
#define SQL_SINGLE_QUOTE "\'"
#define SQL_EQUAL "="
#define SQL_NOT_EQUAL "!="
#define SQL_LESS_THAN "<"
#define SQL_GREATER_THAN ">"
#define SQL_LIKE " LIKE "
#define SQL_NOT_LIKE " NOT LIKE "
#define SQL_WILDCARD "%"
#define SQL_WILDCARD_SINGLE "_"
#define SQL_CREATE_TABLE "CREATE TABLE "
#define SQL_CREATE_INDEX "CREATE INDEX "
#define SQL_INDEX_ON " ON "
#define SQL_END_SEMI ";"

#define SQL_VALUE(st, v) jstring_append_2(st, "'"); \
                         jxta_sql_escape_and_wc_value(v, FALSE); \
                         jstring_append_1(st, v); \
			 jstring_append_2(st, "'");

#define SQL_VARCHAR_64 " varchar(64) "
#define SQL_VARCHAR_128 " varchar(128) "
#define SQL_VARCHAR_256 " varchar(256) "
#define SQL_VARCHAR_4K " varchar(4096) "
#define SQL_VARCHAR_8K " varchar(8192) "
#define SQL_VARCHAR_16K " varchar(16384) "
#define SQL_INTEGER " integer "

/* these are specific to SQLite */

#define SQL_TBL_MASTER " sqlite_master "

#define SQL_COLUMN_Type " type "
#define SQL_COLUMN_Type_table "table"
#define SQL_COLUMN_Type_index "index"

#define SQL_COLUMN_Name " name "

Jxta_boolean jxta_sql_escape_and_wc_value(JString * jStr, Jxta_boolean replace);
