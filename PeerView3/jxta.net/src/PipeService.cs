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
 * $Id: PipeService.cs,v 1.2 2006/08/04 10:33:20 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace JxtaNET
{
    public interface PipeService : Service
    {
        /// <summary>
        /// Create an InputPipe from a pipe Advertisement
        /// </summary>
        /// <param name="adv">is the advertisement of the PipeService</param>
        /// <param name="listener">PipeMsgListener to receive msgs</param>
        /// <returns>InputPipe object created</returns>
        InputPipe CreateInputPipe(PipeAdvertisement adv, PipeMsgListener listener);

        /// <summary>
        /// Create an InputPipe from a pipe Advertisement
        /// </summary>
        /// <param name="adv">is the advertisement of the PipeService</param>
        /// <returns>InputPipe object created</returns>
        InputPipe CreateInputPipe(PipeAdvertisement adv);

        /// <summary>
        /// Attempt to ceate an OutputPipe using the specified Pipe Advertisement. 
        /// The pipe will be be resolved within the provided timeout.
        /// </summary>
        /// <param name="adv">The advertisement of the pipe being resolved.</param>
        /// <param name="timeout">time duration in milliseconds to wait for a successful pipe resolution. 
        /// 0 will wait indefinitely.</param>
        OutputPipe CreateOutputPipe(PipeAdvertisement adv, long timeout);
    }

	/// <summary>
	/// Summary of PipeServiceImpl.
	/// </summary>
	internal class PipeServiceImpl : JxtaObject, PipeService
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_pipe_service_timed_accept(IntPtr self, IntPtr adv, Int64 timeout, ref IntPtr pipe);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_pipe_service_timed_connect(IntPtr self, IntPtr adv, Int64 timeout, IntPtr peers, ref IntPtr pipe);

        [DllImport("jxta.dll")]
        private static extern void jxta_service_get_MIA(IntPtr self, out IntPtr mia);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_module_start(IntPtr self, String[] args);

        [DllImport("jxta.dll")]
        private static extern void jxta_module_stop(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_module_init(IntPtr self, IntPtr group, IntPtr assigned_id, IntPtr impl_adv);

        [DllImport("jxta.dll")]
        public static extern IntPtr jxta_vector_new(UInt32 initialSize);

        #endregion

        public void init(PeerGroup group, ID assignedID, Advertisement implAdv)
        {
            jxta_module_init(this.self, ((PeerGroupImpl)group).self, assignedID.self, implAdv.self);
        }

        public uint startApp(string[] args)
        {
            return jxta_module_start(this.self, args);
        }

        public void stopApp()
        {
            jxta_module_stop(this.self);
        }


        private Dictionary<PipeAdvertisement, Pipe> _pipeDictionary = new Dictionary<PipeAdvertisement,Pipe>();

        /// <summary>
        /// Accept an incoming connection.
        /// This function waits until the pipe is ready to receive messages, or until
        /// the timeout is reached. The semantics of being ready to receive messages
        /// depends on the type of pipe. After the first call to connect, the pipe
        /// is set to wait for connection request. If a connection request arrives after
        /// the timeout has been reached, it will be queued up, and a following call to
        /// TimedAccept() will retrieve the connection request, until the pipe is released.
        /// </summary>
        /// <param name="adv">Pipe Advertisment of the pipe to connect to.</param>
        /// <param name="timeout">timeout in micro-seconds</param>
        /// <returns></returns>
        internal Pipe TimedAccept(PipeAdvertisement adv, long timeout)
        {
            IntPtr ret = new IntPtr();
            Errors.check(jxta_pipe_service_timed_accept(this.self, adv.self, timeout, ref ret));
            return new Pipe(ret, adv);
        }

        /// <summary>
        ///  Try to synchronously connect to the remote end of a pipe.
        ///  If peers is set to a vector containing a list of peers,
        ///  the connection will be attempted to be established with the listed
        ///  peer(s). The number of peers that the vector may contain depends on
        ///  the type of type. Unicast typically can only connect to a single peer.
        /// </summary>
        /// <param name="adv">Pipe Advertisement</param>
        /// <param name="timeout">timeout in micro-seconds</param>
        /// <param name="peers">optional vector of peers</param>
        /// <returns></returns>
        internal Pipe TimedConnect(PipeAdvertisement adv, long timeout, List<Peer> peers)
        {
            IntPtr ret = new IntPtr();

            if (peers != null)
            {
                JxtaVector jVec = new JxtaVector();

                jVec.self = jxta_vector_new(0);

                foreach (Peer peer in peers)
                    jVec.Add(peer.self);

                jxta_pipe_service_timed_connect(this.self, adv.self, timeout, jVec.self, ref ret);
            }
            else
                jxta_pipe_service_timed_connect(this.self, adv.self, timeout, IntPtr.Zero, ref ret);

            if (ret != IntPtr.Zero)
                return new Pipe(ret, adv);
            else return null;
        }

        public InputPipe CreateInputPipe(PipeAdvertisement adv)
        {
            Pipe pipe = new Pipe();

            if (_pipeDictionary.TryGetValue(adv, out pipe) == true)
                return pipe.InputPipe;

            pipe = TimedAccept(adv, 1000);
            _pipeDictionary.Add(adv, pipe);

            return pipe.InputPipe;
        }

        public InputPipe CreateInputPipe(PipeAdvertisement adv, PipeMsgListener pipeListener)
        {
            InputPipeImpl pipe = (InputPipeImpl) CreateInputPipe(adv);

            pipe.AddListener(pipeListener);
        
            return pipe;
        }

        public OutputPipe CreateOutputPipe(PipeAdvertisement adv, long timeout)
        {
            Pipe pipe = new Pipe();
            
            if (_pipeDictionary.TryGetValue(adv, out pipe) == true)
                return pipe.OutputPipe;
            
            pipe = TimedConnect(adv, timeout*1000, null);
            _pipeDictionary.Add(adv, pipe);

            return (pipe != null) ? pipe.OutputPipe : null;
        }

        public Advertisement ImplAdvertisement
        {
            get
            {
                IntPtr adv = new IntPtr();
                jxta_service_get_MIA(this.self, out adv);
                return new ModuleAdvertisement(adv);
            }
        }

		internal PipeServiceImpl(IntPtr self) : base(self) {}
        internal PipeServiceImpl() { }
    }
}
