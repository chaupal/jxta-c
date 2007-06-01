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
 * $Id: RendezVousService.cs,v 1.2 2006/01/27 18:28:25 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{

	public enum RdvConfig
	{
		adhoc,
		edge,
		rendezvous
	};

	/// <summary>
	/// Summary of RendezVousService.
	/// </summary>
	public class RendezVousService : JxtaObject
	{
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_rdv_service_get_peers(IntPtr self, ref IntPtr peers);

        [DllImport("jxta.dll")]
        private static extern bool jxta_rdv_service_peer_is_connected(IntPtr self, IntPtr peers);

        [DllImport("jxta.dll")]
        private static extern int jxta_rdv_service_config(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern Int64 jxta_rdv_service_auto_interval(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_rdv_service_get_peerview_priv(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern uint jxta_rdv_service_peer_get_expires(IntPtr self, IntPtr peer);

        [DllImport("jxta.dll")] 
        private static extern UInt32 jxta_get_config_adhoc();
        
        [DllImport("jxta.dll")] 
        private static extern UInt32 jxta_get_config_edge();
        
        //[DllImport("jxta.dll")] 
        //private static extern UInt32 jxta_get_config_rendezvous();

		public JxtaVector<Peer> getPeers()
		{
            JxtaVector<Peer> peers = new JxtaVector<Peer>();
            Errors.check(jxta_rdv_service_get_peers(self, ref peers.self));
            return peers;
		}

		public bool isConnected(Peer peer)
		{
			return jxta_rdv_service_peer_is_connected(self, peer.self);
		}

		public uint getExpires(Peer peer)
		{
			return jxta_rdv_service_peer_get_expires(this.self, peer.self);
		}

		public RdvConfig getConfiguration()
		{
			int conf = jxta_rdv_service_config(this.self);

            if (conf == jxta_get_config_adhoc())
                return RdvConfig.adhoc; 

            if (conf == jxta_get_config_edge())
				return RdvConfig.edge;

            return RdvConfig.rendezvous;
		}

		public long getAutoInterval()
		{
			return jxta_rdv_service_auto_interval(this.self);
		}

		public PeerView getPeerView()
		{
			return new PeerView(jxta_rdv_service_get_peerview_priv(this.self));
		}

		internal RendezVousService(IntPtr self) : base(self) {}
        internal RendezVousService() : base() { }
    }
}
