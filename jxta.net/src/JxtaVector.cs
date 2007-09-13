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
using System.Runtime.InteropServices;
using System.Collections;

namespace JxtaNET
{
	/// <summary>
	/// Summary of JxtaVector.
	/// </summary>
	internal class JxtaVector : IEnumerable
    {
        #region import of jxta-c functions
        [DllImport("jxta.dll")]
        public static extern uint jxta_vector_size(IntPtr ptr);

        [DllImport("jxta.dll")]
        public static extern UInt32 jxta_vector_get_object_at(IntPtr _this, ref IntPtr obj, int i);

        [DllImport("jxta.dll")]
        public static extern UInt32 jxta_vector_add_object_last(IntPtr vector, IntPtr obj);
        #endregion

        internal IntPtr self;

		public JxtaVectorEnumerator GetEnumerator() 
		{
			return new JxtaVectorEnumerator(this);
		}

		IEnumerator IEnumerable.GetEnumerator() 
		{
			return GetEnumerator();
		}

        public class JxtaVectorEnumerator : IEnumerator
		{
			private int enumerator;
			private JxtaVector vec;

			public JxtaVectorEnumerator(JxtaVector vec)
			{
				this.vec = vec;
				this.enumerator = -1;
			}

			public IntPtr Current
			{
				get
				{
					if ((enumerator >= 0) && (enumerator < vec.Length))
						return vec[enumerator];
					else
						return IntPtr.Zero;
				}
			}

			object IEnumerator.Current 
			{
				get 
				{
					return (Current);
				}
			}


			public bool MoveNext()
			{
				enumerator++;
				if (enumerator < vec.Length)
					return true;
				else return false;
			}

			public void Reset()
			{
				enumerator = -1;
			}
		}

        public void Add(IntPtr obj)
        {
            jxta_vector_add_object_last(self, obj);
        }

		public uint Length
		{
			get
			{
                if (this.self == IntPtr.Zero)
                    return 0;
                return jxta_vector_size(self);
			}
		}

		public IntPtr this [int i]
		{
			get
			{
                IntPtr ptr = new IntPtr();
                Errors.check(jxta_vector_get_object_at(this.self, ref ptr, i));
                return ptr;
			}
		}

		public JxtaVector(IntPtr self)
		{
			this.self = self;
		}

        public JxtaVector()
        {
            this.self = IntPtr.Zero;
        }

    }
}
