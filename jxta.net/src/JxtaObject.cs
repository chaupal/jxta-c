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
 * $Id: JxtaObject.cs,v 1.4 2006/02/16 10:46:36 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{
	/// <summary>
	/// Summary of JxtaObject.
	/// </summary>
	public class JxtaObject
	{
        [DllImport("jxta.dll")]
        private static extern void jxta_initialize();

        [DllImport("jxta.dll")]
        private static extern void jxta_terminate();

        [DllImport("jxta.dll")]
        private static extern Int32 _jxta_object_release(IntPtr self, String s, Int32 i);

        /* If you enable object checking, you have also to enable this feature in jxta-c. */
#if JXTA_OBJECT_CHECKING_ENABLE
        [DllImport("jxta.dll")]
        private static extern bool _jxta_object_check_valid(IntPtr obj, String file, Int32 line);
#else
        private static bool _jxta_object_check_valid(IntPtr obj, String file, Int32 line)
        {
            return true;
        }
#endif

        private IntPtr pself;
        internal IntPtr self
        {
            get { this.valid();  return pself; }
            set { pself = value; this.valid(); }
        }

        private void valid()
        {
            if (!_jxta_object_check_valid(this.pself, "JxtaObject.valid()", 0))
                throw new JxtaException("JxtaObject not valid!");
        }

		~JxtaObject()
		{
            try
            {
                this.valid();

                if (_destroy)
                    if (_jxta_object_release(this.self, "JxtaObject release", 0) < 0)
                        Console.WriteLine("Object-Release-Error!! This should not happen! Please report this error...");

                jxta_terminate();
            }
            catch (JxtaException)
            {
            }

#if TRACK_OBJECT_COUNT
            objCount--;
            Console.WriteLine("objCounter -: " + objCount);
#endif
		}

#if TRACK_OBJECT_COUNT
        private static int objCount = 0;
#endif

        internal JxtaObject(IntPtr self)
		{
            jxta_initialize();

            this.self = self;

            _destroy = false;

#if TRACK_OBJECT_COUNT
            objCount++;
            Console.WriteLine("objCounter +: " + objCount);
#endif
        }

        private bool _destroy;

        public JxtaObject()
        {
            jxta_initialize();

            this.pself = IntPtr.Zero;

            _destroy = true;

#if TRACK_OBJECT_COUNT
            objCount++;
            Console.WriteLine("objCounter +: " + objCount);
#endif
        }
    }
}
