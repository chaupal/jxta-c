﻿/*
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
    /// <summary>
    /// Represents JStrings. JString provides an encapsulation of c-style character buffers.
    /// </summary>
    public class JxtaString : JxtaObject
    {
        #region import of jxta-c functions
        [DllImport("jxta.dll")]
        private static extern IntPtr jstring_get_string(IntPtr ptr);

        [DllImport("jxta.dll")]
        private static extern IntPtr jstring_new_0();

        [DllImport("jxta.dll")]
        private static extern IntPtr jstring_new_2(String val);

        [DllImport("jxta.dll")]
        private static extern Int32 jstring_length(IntPtr js);
        #endregion 

        public static implicit operator String(JxtaString js)
        {
            return js.ToString();
        }

        /// <summary>
        /// Each JString has a data length and buffer size.
        /// This returns the length of the data in the char buffer.
        /// </summary>
        public int Length
        {
            get
            {
                return jstring_length(this.self);
            }
        }

        public override String ToString()
        {
            if (this.self != IntPtr.Zero) 
                return Marshal.PtrToStringAnsi(jstring_get_string(this.self));
            else 
                return "";
      }

        internal JxtaString(IntPtr self) : base(self) { }

        /// <summary>
        /// Create an empty JString
        /// </summary>
        public JxtaString()
            : base(jstring_new_0())
        {
        }

        /// <summary>
        /// Constructs a JString from a constant character array.
        /// </summary>
        /// <param name="val"></param>
        public JxtaString(String val)
            : base(jstring_new_2(val))
        {
        }
    }
}
