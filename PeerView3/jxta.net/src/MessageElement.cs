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
 * $Id: MessageElement.cs,v 1.2 2006/08/04 10:33:19 lankes Exp $
 */
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace JxtaNET
{
    public class MessageElement : JxtaObject
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_message_element_get_value(IntPtr el);

        [DllImport("jxta.dll")]
        private static extern IntPtr jstring_get_string(IntPtr ptr);

        [DllImport("jxta.dll")]
        private static extern IntPtr jstring_new_3(IntPtr ptr);

        [DllImport("jxta.dll")]
        private static extern UInt32 _jxta_object_release(IntPtr self, String s, Int32 i);

        [DllImport("jxta.dll", EntryPoint = "jxta_message_element_new_1")]
        private static extern IntPtr jxta_message_element_new_bytes(String qname, String mime_type, Byte[] value, Int32 length, IntPtr sig);

        [DllImport("jxta.dll", EntryPoint = "jxta_message_element_new_1")]
        private static extern IntPtr jxta_message_element_new_string(String qname, String mime_type, String value, Int32 length, IntPtr sig);
        
        // [DllImport("jxta.dll")]
        // private static extern IntPtr jxta_message_element_new_3(String qname, String ncname, String mime_type, IntPtr value, IntPtr sig);
        #endregion

        /*public ByteStream GetValue()
        {
            return new ByteStream(jxta_message_element_get_value(this.self));
        }*/

        public override string ToString()
        {
            IntPtr str = jstring_new_3(jxta_message_element_get_value(this.self));
            String ret = Marshal.PtrToStringAnsi(jstring_get_string(str));
            _jxta_object_release(str, "", 0);
            return ret;
        }

        internal MessageElement(IntPtr self) : base(self) { }
        internal MessageElement() : base() { }

        // what does the last parameter in the native-call do?
        public MessageElement(string qname, string mimetype, ByteStream value) : base()
        {
            byte[] buffer = new byte[value.Length];
            value.Read(buffer, 0, (int)value.Length);
            this.self = jxta_message_element_new_bytes(qname, mimetype, buffer, buffer.Length, IntPtr.Zero);
        }

        public MessageElement(string qname, string mimetype, string value)
            : base()
        {
            if (value != null)
                this.self = jxta_message_element_new_string(qname, mimetype, value, (int)value.Length, IntPtr.Zero);
            else
                this.self = jxta_message_element_new_string(qname, mimetype, null, 0, IntPtr.Zero);
        }
    }
}
