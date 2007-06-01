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
 * $Id: ByteStream.cs,v 1.2 2006/01/27 18:28:23 lankes Exp $
 */
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Text;

namespace JxtaNET
{
    public class ByteStream : Stream
    {
        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_bytevector_new_1(Int32 initialSize);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_bytevector_add_bytes_at(IntPtr vector, String bytes,
                                                       UInt32 at_index, Int32 length);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_bytevector_new_0();

        //[DllImport("jxta.dll")]
        //private static extern IntPtr jstring_new_3(IntPtr ptr);

        //[DllImport("jxta.dll")]
        //private static extern IntPtr jstring_get_string(IntPtr ptr);

        //[DllImport("jxta.dll")]
        //private static extern UInt32 _jxta_object_release(IntPtr self, string s, int i);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_bytevector_get_bytes_at(IntPtr vector, byte[] bytes, UInt32 at_index, Int32 length);

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_bytevector_size(IntPtr vector);

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_bytevector_add_bytes_at(IntPtr vector, byte[] bytes,
                                                       UInt32 at_index, Int32 length);

        [DllImport("jxta.dll")]
        private static extern bool _jxta_object_check_valid(IntPtr obj, String file, Int32 line);


        private IntPtr pself;
        internal IntPtr self
        {
            get { this.valid(); return pself; }
            set { pself = value; this.valid(); }
        }

        private void valid()
        {
            if (!_jxta_object_check_valid(this.pself, "JxtaObject.valid()", 0))
                throw new JxtaException("JxtaObject not valid!");
        }


        public override bool CanRead { get { return true; } }
        public override bool CanWrite { get { return true; } }

        public override bool CanSeek { get { return false; } }

        public override long Length
        {
            get
            {
                int size = 0;
                try
                {
                    size = jxta_bytevector_size(this.self);
                }
                catch (JxtaException) { }
                return size;
            }
        }

        private long pos;
        public override long Position
        {
            get
            {
                return pos;
            }
            set
            {
                pos = value;
            }
        }

        public override void Flush()
        {
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public override void SetLength(long value)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            if (pos < this.Length)
            {

                if (count > (this.Length - this.pos))
                    count = (int)(this.Length - this.pos);

                byte[] tmp = new byte[count];

                jxta_bytevector_get_bytes_at(this.self, tmp, (UInt32)pos, count);

                tmp.CopyTo(buffer, offset);

                this.pos += count;

                return count;
            }
            else
            {
                return 0;
            }
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            byte[] tmp = new byte[count];
            System.Array.ConstrainedCopy(buffer, offset, tmp, offset, count);
            jxta_bytevector_add_bytes_at(this.self, tmp, (UInt32)pos, count);
            pos += count;
        }

        public ByteStream(String s)
        {
            pos = 0;

            this.self = jxta_bytevector_new_1(s.Length);

            Errors.check(jxta_bytevector_add_bytes_at(this.self, s, 0, s.Length));
        }

        public ByteStream(IntPtr self)
        {
            pos = 0;

            this.pself = self;
        }

        public ByteStream() : base() 
        {
            this.self = jxta_bytevector_new_0();
        }
    }
}
