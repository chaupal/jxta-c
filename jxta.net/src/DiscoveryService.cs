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
 * $Id: DiscoveryService.cs,v 1.2 2006/02/13 21:04:46 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{
	/// <summary>
	/// Summary of DiscoveryService.
	/// </summary>
	public class DiscoveryService : JxtaObject
	{
        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_publish(IntPtr service, IntPtr adv, short type, Int64 lifetime, Int64 lifetimeForOthers);

        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_cancel_remote_query(IntPtr service, Int32 query_id, ref IntPtr listener);

        [DllImport("jxta.dll")]
        private static extern Int32 discovery_service_get_remote_advertisements(IntPtr self,
            String paddr, Int32 disc_peer, String attr, String value, Int32 responses, IntPtr ptr);

        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_get_local_advertisements(IntPtr _this,
            Int32 i, String attr, String value, ref IntPtr res_vec);

        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_flush_advertisements(IntPtr self, IntPtr ptr, Int32 i);

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_get_DISC_PEER();

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_get_DISC_GROUP();

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_get_DISC_ADV();

        [DllImport("jxta.dll")]
        private static extern Int64 jxta_get_DEFAULT_LIFETIME();

        [DllImport("jxta.dll")]
        private static extern Int64 jxta_get_DEFAULT_EXPIRATION();
        
        public static short DISC_PEER;
        public static short DISC_GROUP;
        public static short DISC_ADV;
        public static long DEFAULT_LIFETIME;
        public static long DEFAULT_EXPIRATION;

        public void publish(Advertisement adv, short type, long lifetime, long lifetimeForOthers)
        {
            Errors.check(discovery_service_publish(this.self, adv.self, type, lifetime, lifetimeForOthers));
        }

		public JxtaVector<Advertisement> getLocalAdvertisements(int type, String attr, String value)
		{
            JxtaVector<Advertisement> ret = new JxtaVector<Advertisement>();

            Errors.check(discovery_service_get_local_advertisements(self, type, attr, value, ref ret.self));
			
            return ret;
		}

        // TODO whats exactly up with the listener? Is there something to do??
		public Int32 getRemoteAdvertisements(Int32 type, String peerid, String attr, String val, Int32 threshold, Listener listener)
		{
            IntPtr tmp;

            if (listener != null)
                tmp = listener.self;
            else
                tmp = IntPtr.Zero;
            
            return discovery_service_get_remote_advertisements(self, peerid, type, attr, val, threshold, tmp);
		}

		public void flushAdvertisements(String id, Int32 type)
        {
            IntPtr ptr = Marshal.StringToHGlobalAnsi(id);
            discovery_service_flush_advertisements(self, ptr, type);
            Marshal.FreeHGlobal(ptr);
		}

        public Listener cancelRemoteQuery(Int32 query_id)
        {
            IntPtr ret = new IntPtr();
            discovery_service_cancel_remote_query(this.self, query_id, ref ret);
            return new Listener(ret);
        }

		internal DiscoveryService(IntPtr self) : base(self) {}
        internal DiscoveryService() : base() { }

        static DiscoveryService()
        {
            DISC_PEER = (short)jxta_get_DISC_PEER();
            DISC_GROUP = (short)jxta_get_DISC_GROUP();
            DISC_ADV = (short)jxta_get_DISC_ADV();
            DEFAULT_LIFETIME = jxta_get_DEFAULT_LIFETIME();
            DEFAULT_EXPIRATION = jxta_get_DEFAULT_EXPIRATION();
        }
	}
}
