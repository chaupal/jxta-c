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
 * $Id: PipeService.cs,v 1.1 2006/01/18 20:31:09 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{
	/// <summary>
	/// Summary of PipeService.
	/// </summary>
	public class PipeService : JxtaObject
	{
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_pipe_service_timed_accept(IntPtr self, IntPtr adv, Int64 timeout, ref IntPtr pipe);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_pipe_service_timed_connect(IntPtr self, IntPtr adv, Int64 timeout, IntPtr peers, ref IntPtr pipe);

        public Pipe timedAccept(Advertisement adv, Int64 timeout)
        {
            IntPtr ret = new IntPtr();
            Errors.check(jxta_pipe_service_timed_accept(this.self, adv.self, timeout, ref ret));
            return new Pipe(ret);
        }

        public Pipe timedConnect(Advertisement adv, Int64 timeout, JxtaVector<Peer> peers)
        {
            IntPtr ret = new IntPtr();
            if (peers != null)
                jxta_pipe_service_timed_connect(this.self, adv.self, timeout, peers.self, ref ret);
            else
                jxta_pipe_service_timed_connect(this.self, adv.self, timeout, IntPtr.Zero, ref ret);
            return new Pipe(ret);
        }

		internal PipeService(IntPtr self) : base(self) {}
        internal PipeService() { }
    }
}
