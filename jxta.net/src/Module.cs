/*
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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

namespace JxtaNET
{
    public interface Module
    {
        /// <summary>
        /// Initialize the module, passing it its peer group and advertisement.
        /// </summary>
        /// <param name="group">The PeerGroup from which this Module can obtain services. If this module is a Service, 
        /// this is also the PeerGroup of which this module is a service.</param>
        /// <param name="assignedID">Identity of Module within group. modules can use it 
        /// as a the root of their namespace to create names that are unique within the
        /// group but predictible by the same module on another peer. This is normaly the 
        /// ModuleClassID which is also the name under which the module is known by other 
        /// modules. For a group it is the PeerGroupID itself. The parameters of a service, 
        /// in the Peer configuration, are indexed by the assignedID of that service, and a 
        /// Service must publish its run-time parameters in the Peer Advertisement under its 
        /// assigned ID.</param>
        /// <param name="implAdv">The implementation advertisement for this Module. It is 
        /// permissible to pass null if no implementation advertisement is available. This 
        /// may happen if the implementation was selected by explicit class name rather than 
        /// by following an implementation advertisement. Modules are not required to support 
        /// that style of loading, but if they do, then their documentation should mention it.</param>
        void init(PeerGroup group, ID assignedID, Advertisement implAdv);

        /// <summary>
        /// Complete any remaining initialization of the module. The module should be fully 
        /// functional after startApp() is completed. That is also the opportunity to supply 
        /// arbitrary arguments (mostly to applications).
        /// </summary>
        /// <param name="args">An array of Strings forming the parameters for this Module.</param>
        /// <returns>status indication. By convention 0 means that this Module started succesfully.</returns>
        uint startApp(String[] args);

        /// <summary>
        /// Stop a module. This may be called any time after init()  completes and 
        /// should not assume that startApp() has been called or completed.
        /// </summary>
        void stopApp();
    }
}
