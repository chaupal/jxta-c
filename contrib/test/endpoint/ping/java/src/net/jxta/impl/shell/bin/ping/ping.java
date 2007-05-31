/*
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: ping.java,v 1.4 2002/02/26 01:09:24 yaroslav Exp $
 */


package net.jxta.impl.shell.bin.ping;

import net.jxta.impl.shell.*;

import java.io.*;
import java.util.*;

import net.jxta.exception.*;
import net.jxta.discovery.*;
import net.jxta.endpoint.*;
import net.jxta.pipe.*;
import net.jxta.impl.pipe.*;
import net.jxta.peergroup.*;
import net.jxta.document.*;
import net.jxta.id.*;
import net.jxta.protocol.*;

/**
 * talk Shell command: send and receive message from other users.
 *
 */

public class ping extends ShellApp implements EndpointListener {


    private ShellEnv env = null;
    private Thread thread = null;

    private EndpointService endpoint = null;

    private static final String TEST_SERVICE_NAME = "jxta:EndpointTest";
    private static final String TEST_SERVICE_PARAMS = "EndpointTestParams";
    private static final String ENV_NAME = "ping-deamon";
    private static final String MSG_ELEMENT_NAME = "PingElementName";
    private static final String MSG_ELEMENT_CONTENT = "This is a test message (ping)";
    private static final String DEFAULT_ADDRESS = "http://127.0.0.1:9700";

    public ping() {
    }

    public void stopApp() {
    }

    private int syntaxError() {

        println("Usage: ping -start");
        println("Usage: ping -stop");
        println("Usage: ping -send [<host>]");

        return ShellApp.appParamError;
    }

    public int startApp(String[] args) {

        if ((args == null) || (args.length == 0)) {
            return syntaxError();
        }

        env = getEnv();
        endpoint = group.getEndpointService();

        if (args[0].equals("-start")) {
            return startReceive(args);
        }

        if (args[0].equals("-stop")) {
            return stopReceive(args);
        }

        if (args[0].equals("-send")) {
            return sendMessage(args);
        }

        return ShellApp.appNoError;
    }


    private boolean deamonRunning() {

        ShellObject obj = env.get(ENV_NAME);
        if (obj == null) {
            return false;
        } else {
            return true;
        }
    }

    private int startReceive(String[] args) {

	if (deamonRunning()) {
            println("ping is already listening");
            return ShellApp.appMiscError;
        }

	// Create the Endpoint Listener
	try {
	    endpoint.addListener(TEST_SERVICE_NAME + TEST_SERVICE_PARAMS, this);
	} catch (Exception ez1) {
	    // Not much we can do here.
	    println ("Cannot add endpoint listener");
	    return ShellApp.appMiscError;
	}

        // Store this object
        ShellObject obj = env.add(ENV_NAME, new ShellObject("Ping Deamon",this));

        return ShellApp.appNoError;
    }


    public void doStopReceive() {
	// Remove the listener
	endpoint.removeListener(TEST_SERVICE_NAME + TEST_SERVICE_PARAMS, this);
    }


    private int stopReceive(String[] args) {

        ShellObject obj = env.get(ENV_NAME);

        ping d = null;
        try {
            d = (ping) obj.getObject();
	    d.doStopReceive();
            env.remove(ENV_NAME);
            return ShellApp.appNoError;
        } catch (Exception e) {
            println("ping: cannot stop deamon");
            return ShellApp.appMiscError;
        }
    }

    private int sendMessage (String[] args) {

	String host = null;
	if (args.length <= 2) {
	    // Use default address
	    host = DEFAULT_ADDRESS;
	} else {
	    host = args [1];
	}

	if (!deamonRunning()) {
	    println ("ping: you must first do ping -start");
	    return ShellApp.appMiscError;
	}

        try {
	    // Build the destination endpoint address
	    EndpointAddress dst = endpoint.newEndpointAddress (host);
	    dst.setServiceName (TEST_SERVICE_NAME);
	    dst.setServiceParameter (TEST_SERVICE_PARAMS);

	    // Get a messenger
            EndpointMessenger messenger =
		endpoint.getMessenger(dst);
            if (messenger == null) {
		println ("ping: cannot get a messenger for " + dst.toString());
		return ShellApp.appMiscError;
            }

            Message msg = endpoint.newMessage();

            msg.setString (MSG_ELEMENT_NAME, MSG_ELEMENT_CONTENT);
            messenger.sendMessage(msg);
	    println ("Message has been sent to " + dst.toString());

        } catch (Exception e) {
		println ("ping: cannot send a mesage to " + host);
		return ShellApp.appMiscError;
        }
	return ShellApp.appNoError;
    }


    public void processIncomingMessage(Message message,
				       EndpointAddress srcAddr,
				       EndpointAddress dstAddr) {

	println ("PING: received message from " + dstAddr.toString());
	// Send it back
	Message newMsg = (Message) message.clone();

	srcAddr.setServiceName (TEST_SERVICE_NAME);
	srcAddr.setServiceParameter (TEST_SERVICE_PARAMS);

        try {
	    // Get a messenger
            EndpointMessenger messenger =
		endpoint.getMessenger(srcAddr);
            if (messenger == null) {
		println ("ping: cannot get a messenger for " + srcAddr.toString());
		return;
            }

            messenger.sendMessage(newMsg);
	    println ("Message has been sent back to " + srcAddr.toString());

        } catch (Exception e) {
		println ("ping: cannot send a mesage to " + srcAddr.toString());
        }
	
    }


    public String getDescription() {
        return "Talk to another peer";
    }

    public void help() {
        println("NAME");
        println("     ping - endpoint ping service ");
        println(" ");
        println("SYNOPSIS");
        println(" ");
        println("    ping -start");
        println("    ping -stop");
        println("    talk -send <host>");
        println(" ");
        println("DESCRIPTION");
        println(" ");
	println("The ping commands registers an Endpoint Listener to the following");
	println("address:");
	println(" ");
	println("   proto://protocol_address/jxta:EndpointTest/EndpointTestParams");
	println(" ");
	println("All messages received are sent back to the source address.");
	println(" ");
        println("OPTIONS");
        println(" ");
        println("    -start start listening");
        println("    -stop stop listening");
        println("    -send <host> send a message to a remote host.");
	println("                 default host is http://127.0.0.1:9701");
        println(" ");
        println("EXAMPLE");
        println(" ");
        println("      JXTA>talk -start");
        println("      JXTA>talk -send");
        println("      JXTA>talk -send http://192.168.1.10:6002");
        println("SEE ALSO");
        println(" ");
    }

}


