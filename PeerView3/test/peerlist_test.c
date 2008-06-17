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
 */


#include "peerlist.h"
#include "jxta_peer.h"
#include "jxta.h"
#include "jxta_apr.h"


/** An example function.  A filter for returning the 
 *  rdv peers should be "built in" to the class. This
 *  demo just provides something 
 *
 * lomax@jxta.org FIXME this function should do something...
 */

Jxta_boolean get_Name(Jxta_peer * c)
{
    return TRUE;
}




Jxta_boolean peer_test(void)
{

    Jxta_boolean passed = FALSE;
    Jxta_peer *c1;
    Jxta_peer *c2;
    Jxta_peer *c3;
    Jxta_peer *c4;
    Jxta_peer *c5;
    Jxta_peer *c6;

    Peerlist *rdvlist;
    Peerlist *cl = peerlist_new();

    c1 = jxta_peer_new();
    c2 = jxta_peer_new();
    c3 = jxta_peer_new();
    c4 = jxta_peer_new();
    c5 = jxta_peer_new();
    c6 = jxta_peer_new();

    peerlist_add_peer(cl, c1);
    peerlist_add_peer(cl, c2);
    peerlist_add_peer(cl, c3);
    peerlist_add_peer(cl, c4);
    peerlist_add_peer(cl, c5);
    peerlist_add_peer(cl, c6);

    printf("Original list:\n");
    peerlist_print(cl);
    rdvlist = peerlist_filter(cl, (Peer_Filter_Func) get_Name);
    printf("Filtered list:\n");
    peerlist_print(rdvlist);

    peerlist_delete(cl);

    return passed;
}




#ifdef STANDALONE
int main(int argc, char **argv)
{

    if (peer_test()) {
        printf("Passed peerlist_test\n");
        return TRUE;
    } else {
        printf("Failed peerlist_test\n");
        return FALSE;
    }

}
#endif
