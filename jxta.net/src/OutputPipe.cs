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
 * $Id$
 */
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace JxtaNET
{
    public interface OutputPipe
    {
        /// <summary>
        /// Send a message throught the pipe
        /// </summary>
        /// <param name="msg">is the message to be sent.</param>
        /// <returns></returns>
        bool Send(Message msg);

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
        String Type
        {
            get;
        }

        /// <summary>
        /// The pipe name
        /// </summary>
        String Name
        {
            get;
        }
    }

    internal class OutputPipeImpl : JxtaObject, OutputPipe
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
        private static extern int jxta_outputpipe_send(IntPtr op, IntPtr msg);
        #endregion

        public bool Send(Message msg)
        {
            if (this.self == IntPtr.Zero)
                return false;

            if (jxta_outputpipe_send(this.self, msg.self) != Errors.JXTA_SUCCESS)
                return false;

            return true;
        }

        private PipeAdvertisement adv = null;
        public PipeAdvertisement Advertisement
        {
            get
            {
                return adv;
            }
        }

        public String Type
        {
            get
            {
                return adv.Type;
            }
        }

        public String Name
        {
            get
            {
                return adv.Name;
            }
        }

        public void Close()
        {
            this.self = IntPtr.Zero;
        }

        internal OutputPipeImpl(IntPtr self, PipeAdvertisement a)
            : base(self)
        {
            adv = a;
        }

        internal OutputPipeImpl() : base() { }

        ~OutputPipeImpl()
        {
            Close();
        }
    }
 }
