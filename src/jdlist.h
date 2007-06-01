/*
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights
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
 * $Id: jdlist.h,v 1.10 2005/09/21 21:16:46 slowhog Exp $
 */


#ifndef __JDLIST_H__
#define __JDLIST_H__

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _Dlist Dlist;

struct _Dlist {
    Dlist *flink;
    Dlist *blink;
    void *val;
};


typedef void (*DlFreeFunc) (void *);
typedef void (*DLPRINTFUNC) (void *, void *user_data);


/* Nil, first, next, and prev are macro expansions for list traversal 
 * primitives. */

#define dl_nil(l) (l)

#define dl_first(l) (l->flink)
#define dl_first_val(l) (l->flink->val)

#define dl_last(l) (l->blink)
#define dl_lst_val(l) (l->blink->val)

#define dl_next(n) (n->flink)

#define dl_prev(n) (n->blink)

/* These are the routines for manipluating lists */

Dlist *dl_make(void);

 /*
  * Makes a new node, and inserts it before
  * the given node -- if that node is the 
  * head of the list, the new node is 
  * inserted at the end of the list
  */
void dl_insert_b(Dlist * /* node */ , void * /* val */ );

#define dl_insert_a(n, val) dl_insert_b(n->flink, val)

/*
 * Deletes and free's a node.
 */
void dl_delete_node(Dlist * /* node */ );

/*
 * Deletes the entire list from existance.
 */
void dl_delete_list(Dlist * /* head_node */ );

/*
 * Returns node->val (used to shut lint up).
 */
void *dl_val(Dlist * /* node */ );

int dl_size(Dlist * /* list */ );

#define dl_traverse(ptr, list) \
  for (ptr = dl_first(list); ptr != dl_nil(list); ptr = dl_next(ptr))
#define dl_empty(list) (list->flink == list)

void dl_free(Dlist *, DlFreeFunc);
void dl_print(Dlist *, DLPRINTFUNC, void *user_data);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JDLIST_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
