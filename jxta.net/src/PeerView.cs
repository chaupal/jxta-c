/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: PeerView.cs,v 1.2 2006/08/04 10:33:20 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace JxtaNET
{
	/// <summary>
	/// Summary of PeerView.
	/// </summary>
	public class PeerView : JxtaObject
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
		private static extern UInt32 jxta_peerview_get_self_peer(IntPtr self, ref IntPtr selfPVE);

		[DllImport("jxta.dll")]
        private static extern UInt32 jxta_peerview_get_down_peer(IntPtr self, ref IntPtr down);

		[DllImport("jxta.dll")]
        private static extern UInt32 jxta_peerview_get_up_peer(IntPtr self, ref IntPtr up);

		[DllImport("jxta.dll")]
        private static extern UInt32 jxta_peerview_get_localview(IntPtr self, ref IntPtr peers);
        #endregion

        public Peer SelfPeer
		{
            get
            {
                IntPtr ret = new IntPtr();
                Errors.check(jxta_peerview_get_self_peer(this.self, ref ret));
                return new Peer(ret);
            }
		}

		public Peer DownPeer
		{
            get
            {
                IntPtr ret = new IntPtr();
                Errors.check(jxta_peerview_get_down_peer(this.self, ref ret));
                return new Peer(ret);
            }
        }
		
		public Peer UpPeer
		{
            get
            {
                IntPtr ret = new IntPtr();
                Errors.check(jxta_peerview_get_up_peer(this.self, ref ret));
                return new Peer(ret);
            }
		}

        public List<Peer> LocalView
		{
            get
            {
                JxtaVector jVec = new JxtaVector();
                Errors.check(jxta_peerview_get_localview(this.self, ref jVec.self));

                List<Peer> peers = new List<Peer>();

                foreach (IntPtr ptr in jVec)
                    peers.Add(new Peer(ptr));

                return peers;
            }
		}

		internal PeerView(IntPtr self) : base(self) {}
	}
}
