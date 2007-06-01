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
 * $Id: InputPipe.cs,v 1.2 2006/08/04 10:33:19 lankes Exp $
 */
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace JxtaNET
{
    public interface InputPipe
    {
        /// <summary>
        /// Wait (block) for a message to be received.
        /// </summary>
        /// <returns>a message or null if the pipe has been closed</returns>
        Message WaitForMessage();

        /// <summary>
        /// Poll for a message from the pipe. If there is no message immediately 
        /// available then wait the specified amount of time for a message to arrive.
        /// </summary>
        /// <param name="timeout">Maximum number of milliseconds to wait (block) for a message to be received. 
        /// If zero then wait indefinitely for a message.</param>
        /// <returns>Message received or null if the pipe has closed or the timeout expired 
        /// without a message being received.</returns>
        Message Poll(int timeout);

        /// <summary>
        /// Close the pipe.
        /// </summary>
        void Close();

        /// <summary>
        /// The pipe advertisement
        /// </summary>
        PipeAdvertisement Advertisement
        {
            get;
        }

        /// <summary>
        /// The pipe type
        /// </summary>
        string Type
        {
            get;
        }

        /// <summary>
        /// The pipe name
        /// </summary>
        string Name
        {
            get;
        }
    }

    internal class InputPipeImpl : JxtaObject, InputPipe
    {
        #region import of jxta-c functions
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_inputpipe_add_listener(IntPtr ip, IntPtr listener);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_inputpipe_timed_receive(IntPtr ip, Int64 timeout, out IntPtr msg);
        #endregion

        private Listener<MessageImpl> msgListener = null;
        private PipeMsgListener pipeListener = null;

        private void MessageListener(Message msg)
        {
            if (pipeListener != null)
                pipeListener.PipeMsgEvent(new PipeMsgEvent((PipeID)adv.ID, msg));
        }

        /// <summary>
        /// Add a receive listener to a pipe. The listener is invoked for each received
        /// message.
        /// </summary>
        /// <param name="pListener"></param>
        internal void AddListener(PipeMsgListener pListener)
        {
            if (pipeListener != null)
                throw new JxtaException(Errors.JXTA_BUSY);
            if (this.self == IntPtr.Zero)
                throw new JxtaException(Errors.JXTA_FAILED);

            pipeListener = pListener;
            msgListener = new Listener<MessageImpl>(MessageListener, 1, 200);
            Errors.check(jxta_inputpipe_add_listener(this.self, msgListener.self));
            msgListener.Start();
        }

        public Message WaitForMessage()
        {
            uint status;
            IntPtr msg = new IntPtr();

            while (true)
            {
                if (this.self == IntPtr.Zero)
                    return null;

                status = jxta_inputpipe_timed_receive(this.self, 300 * 1000 * 1000, out msg);
                if (status == Errors.JXTA_SUCCESS)
                    return new Message(msg);
                else if (status != Errors.JXTA_TIMEOUT)
                    return null;
            }
        }

        public Message Poll(int timeout)
        {
            IntPtr msg = new IntPtr();
            
            if (this.self == IntPtr.Zero)
                return null;

            uint status = jxta_inputpipe_timed_receive(this.self, timeout*1000, out msg);
            if (status == Errors.JXTA_SUCCESS)
                return new Message(msg);

            return null;
        }

        private PipeAdvertisement adv = null;
        public PipeAdvertisement Advertisement
        {
            get
            {
                return adv;
            }
        }
        
        public string Type
        {
            get
            {
                return adv.Type;
            }
        }

        public string Name
        {
            get
            {
                return adv.Name;
            }
        }

        public void Close()
        {
            if (msgListener != null)
                msgListener.Stop();

            msgListener = null;
            this.self = IntPtr.Zero;
        }

        internal InputPipeImpl(IntPtr self, PipeAdvertisement a)
            : base(self)
        {
            adv = a;
        }

        internal InputPipeImpl() : base() { }

        ~InputPipeImpl()
        {
            Close();
        }
    }
}
