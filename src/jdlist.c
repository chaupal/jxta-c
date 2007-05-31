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
 * $Id: jdlist.c,v 1.8 2005/02/16 23:26:47 bondolo Exp $
 */

/* 
 * $Source: /cvs/jxta-c/src/jdlist.c,v $
 * $Revision: 1.8 $
 * $Date: 2005/02/16 23:26:47 $
 * $Author: bondolo $
 */

#include <stdio.h>    /* Basic includes and definitions */
#include <stdlib.h>
#include "jdlist.h"
#include "jxta_object.h"

#define TRUE 1
#define FALSE 0


/*---------------------------------------------------------------------*
 * PROCEDURES FOR MANIPULATING DOUBLY LINKED LISTS 
 * Each list contains a sentinal node, so that     
 * the first item in list l is l->flink.  If l is  
 * empty, then l->flink = l->blink = l.            
 *---------------------------------------------------------------------*/


Dlist * dl_make()
{
  Dlist * d;

  d = (Dlist *) malloc (sizeof (Dlist));
  d->flink = d;
  d->blink = d;
  d->val = (void *) 0;
  return d;
}


void 
dl_insert_b(Dlist * node, void * val)	/* Inserts to the end of a list */
{
  Dlist * last_node, *new;

  new = (Dlist *) malloc (sizeof (Dlist));
  new->val = val;

  last_node = node->blink;

  node->blink = new;
  last_node->flink = new;
  new->blink = last_node;
  new->flink = node;
}


void
dl_insert_list_b(Dlist * node, Dlist* list_to_insert)
{
  Dlist * last_node, *f, *l;

  if (dl_empty(list_to_insert)) {
    free(list_to_insert);
    return;
  }
  f = list_to_insert->flink;
  l = list_to_insert->blink;
  last_node = node->blink;

  node->blink = l;
  last_node->flink = f;
  f->blink = last_node;
  l->flink = node;
  free(list_to_insert);
}


void
dl_delete_node(Dlist * item)		/* Deletes an arbitrary iterm */
{
  item->flink->blink = item->blink;
  item->blink->flink = item->flink;
  free(item);
}





void *
dl_val(Dlist * l)
{
  return l->val;
}


int
dl_size(Dlist * l)
{
  int i = 0;
  Dlist * tmp;

  dl_traverse(tmp, l)
      i++;

  return i;
}




void
dl_delete_list(Dlist * l)
{
  Dlist * d;
  Dlist * next_node;

  d = l->flink;
  while(d != l) {
    next_node = d->flink;
    free(d);
    d = next_node;
  }
  free(d);
}



void
dl_free(Dlist * l, DlFreeFunc free_val) {
  Dlist * d;
  Dlist * next_node;

  d = l->flink;
  while(d != l) {
    next_node = d->flink;
    if (free_val != NULL) {
      free_val(d->val);
    } else {
      JXTA_OBJECT_RELEASE( d->val);
    }
    free(d);
    d = next_node;
  }
  free(d);
}


void
dl_print(Dlist * l, DLPRINTFUNC printer, void * user_data) {
  Dlist * d;
  Dlist * next_node;

  d = l->flink;
  while(d != l) {
    next_node = d->flink;
    printer(d->val,user_data);
    d = next_node;
  }
}

