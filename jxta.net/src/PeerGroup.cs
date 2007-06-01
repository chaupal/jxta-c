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
 * $Id: PeerGroup.cs,v 1.2 2006/02/05 16:05:30 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{
	/// <summary>
	/// Summary of PeerGroup.
	/// </summary>
	public class PeerGroup : JxtaObject
	{
		[DllImport("jxta.dll")]
		private static extern void jxta_PG_get_PA(IntPtr self, ref IntPtr adv);

		[DllImport("jxta.dll")]
        private static extern void jxta_PG_get_membership_service(IntPtr self, ref IntPtr membership);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_groupname(IntPtr self, ref IntPtr gname);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_peername(IntPtr self, ref IntPtr pname);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_GID(IntPtr self, ref IntPtr gid);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_PID(IntPtr self, ref IntPtr pid);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_PGA(IntPtr self, ref IntPtr pga);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_pipe_service(IntPtr self, ref IntPtr pipe);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_discovery_service(IntPtr self, ref IntPtr disco);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_rendezvous_service(IntPtr self, ref IntPtr rdv);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_PG_new_netpg(ref IntPtr p);

        [DllImport("jxta.dll")]
        private static extern void jxta_module_stop(IntPtr p);

		public MembershipService getMembershipService()
		{
            IntPtr ret = new IntPtr();
			jxta_PG_get_membership_service(self, ref ret);
            return new MembershipService(ret);
		}

		public PeerAdvertisement getPeerAdvertisement()
		{
            IntPtr ret = new IntPtr();
			jxta_PG_get_PA(self, ref ret);
            return new PeerAdvertisement(ret);
        }

		public String getPeerGroupName()
		{
            IntPtr ret = new IntPtr();
            jxta_PG_get_groupname(self, ref ret);
			return new JxtaString(ret);
		}

		public DiscoveryService getDiscoveryService()
		{
            IntPtr ret = new IntPtr();
            jxta_PG_get_discovery_service(self, ref ret);
            return new DiscoveryService(ret);
		}

		public String getPeerName()
		{
            IntPtr ret = new IntPtr();
			jxta_PG_get_peername(self, ref ret);
            return new JxtaString(ret);
		}
		
		public ID getPeerGroupID()
		{
            IntPtr ret = new IntPtr();
            jxta_PG_get_GID(self, ref ret);
            return new ID(ret);
		}

		public ID getPeerID()
		{
            IntPtr ret = new IntPtr();
            jxta_PG_get_PID(self, ref ret);
            return new ID(ret);
		}

		public PeerGroupAdvertisement getPeerGroupAdvertisement()
		{
            IntPtr ret = new IntPtr();
            jxta_PG_get_PGA(self, ref ret);
            return new PeerGroupAdvertisement(ret);
		}

		public PipeService getPipeService()
		{
            IntPtr ret = new IntPtr();
			jxta_PG_get_pipe_service(self, ref ret);
			return new PipeService(ret);
		}
		
		public RendezVousService getRendezVousService()
		{
            IntPtr ret = new IntPtr();
			jxta_PG_get_rendezvous_service(self, ref ret);
			return new RendezVousService(ret);
		}

        internal PeerGroup(IntPtr self) : base(self) { }

        public PeerGroup()
        {
            IntPtr ret = new IntPtr();
            Errors.check(jxta_PG_new_netpg(ref ret));
            self = ret;
        }

        ~PeerGroup()
        {
            jxta_module_stop(self);
        }
	}
}
