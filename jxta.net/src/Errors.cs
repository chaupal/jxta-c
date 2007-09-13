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
    /// <summary>
    /// Represents possible jxta-c error values
    /// </summary>
    public class Errors
    {
        #region import of jxta-c functions
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_SUCCESS();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_INVALID_ARGUMENT();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_ITEM_NOTFOUND();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_NOMEM();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_TIMEOUT();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_BUSY();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_VIOLATION();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_FAILED();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_CONFIG_NOTFOUND();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_IOERR();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_ITEM_EXISTS();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_NOT_CONFIGURED();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_UNREACHABLE_DEST();
        [DllImport("jxta.dll")] private static extern UInt32 jxta_get_JXTA_TTL_EXPIRED();
        #endregion

        // some necessary constants, which are previously defined in JXTA-C
        public static readonly UInt32 JXTA_SUCCESS;
        public static readonly UInt32 JXTA_INVALID_ARGUMENT;
        public static readonly UInt32 JXTA_ITEM_NOTFOUND;
        public static readonly UInt32 JXTA_NOMEM;
        public static readonly UInt32 JXTA_TIMEOUT;
        public static readonly UInt32 JXTA_BUSY;
        public static readonly UInt32 JXTA_VIOLATION;
        public static readonly UInt32 JXTA_FAILED;
        public static readonly UInt32 JXTA_CONFIG_NOTFOUND;
        public static readonly UInt32 JXTA_IOERR;
        public static readonly UInt32 JXTA_ITEM_EXISTS;
        public static readonly UInt32 JXTA_NOT_CONFIGURED;
        public static readonly UInt32 JXTA_UNREACHABLE_DEST;
        public static readonly UInt32 JXTA_TTL_EXPIRED;

        static Errors()
        {
            // intialization of the constants, which are previously defined in JXTA-C
            JXTA_SUCCESS = jxta_get_JXTA_SUCCESS();
            JXTA_INVALID_ARGUMENT = jxta_get_JXTA_INVALID_ARGUMENT();
            JXTA_ITEM_NOTFOUND = jxta_get_JXTA_ITEM_NOTFOUND();
            JXTA_NOMEM = jxta_get_JXTA_NOMEM();
            JXTA_TIMEOUT = jxta_get_JXTA_TIMEOUT();
            JXTA_BUSY = jxta_get_JXTA_BUSY();
            JXTA_VIOLATION = jxta_get_JXTA_VIOLATION();
            JXTA_FAILED = jxta_get_JXTA_FAILED();
            JXTA_CONFIG_NOTFOUND = jxta_get_JXTA_CONFIG_NOTFOUND();
            JXTA_IOERR = jxta_get_JXTA_IOERR();
            JXTA_ITEM_EXISTS = jxta_get_JXTA_ITEM_EXISTS();
            JXTA_NOT_CONFIGURED = jxta_get_JXTA_NOT_CONFIGURED();
            JXTA_UNREACHABLE_DEST = jxta_get_JXTA_UNREACHABLE_DEST();
            JXTA_TTL_EXPIRED = jxta_get_JXTA_TTL_EXPIRED();          
        }

        internal static void check(UInt32 err)
        {
            if (err != Errors.JXTA_SUCCESS)
                throw new JxtaException(err);
        }
    }

    /// <summary>
    /// Represents errors that occur during JXTA.NET application execution
    /// </summary>
    public class JxtaException : Exception
    {
        public String ErrorMessage = "";
        public int ErrorCode = 0;

        /// <summary>
        /// Initializes a new instance of the JxtaException class. 
        /// </summary>
        /// <param name="errorcode">jxta-c error code</param>
        public JxtaException(UInt32 errorcode)
        {
            String error = "JXTA-Error(" + errorcode + "): ";

            if (errorcode == Errors.JXTA_SUCCESS) error += "JXTA_SUCCESS";
            if (errorcode == Errors.JXTA_INVALID_ARGUMENT) error += "JXTA_INVALID_ARGUMENT";
            if (errorcode == Errors.JXTA_ITEM_NOTFOUND) error += "JXTA_ITEM_NOTFOUND";
            if (errorcode == Errors.JXTA_NOMEM) error += "JXTA_NOMEM";
            if (errorcode == Errors.JXTA_TIMEOUT) error += "JXTA_TIMEOUT";
            if (errorcode == Errors.JXTA_BUSY) error += "JXTA_BUSY";
            if (errorcode == Errors.JXTA_VIOLATION) error += "JXTA_VIOLATION";
            if (errorcode == Errors.JXTA_FAILED) error += "JXTA_FAILED";
            if (errorcode == Errors.JXTA_CONFIG_NOTFOUND) error += "JXTA_CONFIG_NOTFOUND";
            if (errorcode == Errors.JXTA_IOERR) error += "JXTA_IOERR";
            if (errorcode == Errors.JXTA_ITEM_EXISTS) error += "JXTA_ITEM_EXISTS";
            if (errorcode == Errors.JXTA_NOT_CONFIGURED) error += "JXTA_NOT_CONFIGURED";
            if (errorcode == Errors.JXTA_UNREACHABLE_DEST) error += "JXTA_UNREACHABLE_DEST";
            if (errorcode == Errors.JXTA_TTL_EXPIRED) error += "JXTA_TTL_EXPIRED";

            this.ErrorMessage = error;
            this.ErrorCode = (int)errorcode;
        }

        /// <summary>
        /// Initializes a new instance of the JxtaException class with a specified error message.
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        public JxtaException(String message) : base(message) { }
    }
}
