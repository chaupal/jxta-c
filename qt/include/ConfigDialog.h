/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: ConfigDialog.h,v 1.3 2002/03/12 13:46:27 wiarda Exp $
 */

#include <qfont.h>
#include <qdialog.h>
#include <qlineedit.h>
#include <qcheckbox.h> 
#include <qtabwidget.h> 
#include <qlabel.h> 
#include <qcombobox.h>
#include <qlistbox.h> 

class ServerConnectWidget:public QWidget{
   Q_OBJECT

 public:
    /** 
     * Constructs a new ServerConnect Widget 
     */
     ServerConnectWidget(QWidget * parent=0, const char * name=0, WFlags f=0);

     ~ServerConnectWidget();

    /** Retrieve a list of servers from a URL and fills the table */
    void fillList(const QString & location);

     /** 
      * Returns a string that collects the selected server 
      * @return the list of the selected servers
      */
     const QString * getServer();
  private:
     /** The line where we add the server address */ 
     QLineEdit *server;

     /** The line where we add the server port */ 
     QLineEdit *port;

     /** The list that displays the selected servers */
     QListBox *serverList;

  protected slots:
     /** 
      * Adds a newly entered server to the list
      */ 
     void addServer();

     /** 
      * Removes a server from the list
      */ 
     void removeServer();
};

class JxtaConfigDialog:public QDialog{
   Q_OBJECT
  public:
    /** The enumerations of possible result codes */ 
     enum ResultCodes{ 
        OKAY,          // we are done configuring
        CANCEL         // we want to cancel
     };

    /**
    *  Creates a new ConfigDialog object
    *  @param parent the parent widget of this dialog
    *  @param name the name of the dialog - will be used as title
    *  @param  ad the peerad we are editing
    */
    JxtaConfigDialog(QWidget * parent=0, 
                     const char * name=0,
                     void * ad = 0);


    ~JxtaConfigDialog();

  private:
     /**
     * Adds a new tab that allows to change the peer name 
     * @param tabber the QTabWidget to which to add the tab
     */
     void addBasic( QTabWidget *tabber ); 

     /**
     * Adds a new advanced tab
     * @param tabber the QTabWidget to which to add the tab
     */
     void addAdvanced( QTabWidget *tabber ); 

     /**
     * Adds a new rendezvous tab
     * @param tabber the QTabWidget to which to add the tab
     */
     void addRendezvous( QTabWidget *tabber ); 

      /** 
      * Adds  controls  for the TCP (index 0) or HTTP (index 1)
      * settings of the network parameters 
      * @param parent the widget to which to add the controls
      * @param index the index  (TCP 0, HTTP 1) for which we want to add 
      *        the controls
      */
      void addNetworkSlot(QWidget *parent,int index);


     /** 
      * Changes the net work option if we select enabled or not 
      * @param index if 0, the TCP options are effected, if 1 th http 
      *              options are changed
      */ 
     void toggleNetworkOptions(int index);

     /** 
      * Sets the data from the supplied PeerAdvertisement object 
      */
     void setData();

     /** The line widget that holds the peer name information */
     QLineEdit *peerName;

     /** The checkbox that indicates whether we use a proxy server */
     QCheckBox *proxy;

     /** The label asking to set the proxyAddress */
     QLabel *proxyAddress;

     /** The text box where the proxy server is set */
     QLineEdit *proxyServer;

     /** The text box that reads the  port number of the proxy server */
     QLineEdit *proxyPort;

     /** The combo box that allows to edit the trace level desired */
     QComboBox  *traceLevel;

     /** 
      *  Possible choices for the debug string level.
      *  The value for each index is the term as written 
      *  to the PeerAdvertisement, the second the string presented to the user
      */
     char ***traceLevelValues;

     /** The number of possible trace levels */
     int traceLevelNumber;

     /** 
      *  The check boxes that enable TCP (index 0) 
      *  and HTTP (index 1) options 
      */
     QCheckBox **networkSettings;

     /** 
     * The Text fields that hold the network ports:
     * Index 0 is TCP and index 1 is HTTP. 
     */
     QLineEdit **networkPorts;

     /** 
      * The check box that allows to switch the network to always manual
      * with  TCP at index 0 and http and index 1
      */
     QCheckBox **networkManualAlways;

     /** 
      * The lable that accompanies networkManualAlways
      * with  TCP at index 0 and http and index 1
      */
     QLabel **networkManualLabel;

      /**  
      * The (writable) combo boxes tha allow to  add the host name.
      * Index 0 is TCP and index 1 is HTTP. 
      */
     QComboBox **networkHostname;

     /** The label for the option public nat address for TCP connections */
     QLabel *publicNATLabel;

     /** The text box to enter the server of the public NAT address  */
     QLineEdit *publicNATServer;

     /** The text box to enter the server port  of the public NAT address  */
     QLineEdit *publicNATPort;

     /** 
      * The check box that  indicates whether we want to act as a rendezvous
      */
     QCheckBox *isRendezVous;
    
     /** The listing of the available TCP rendezvous */
     ServerConnectWidget *TCPRendezVous;     
    
     /** The label that goes with the TCPRendezVous widget */
     QLabel *TCPRendezVousLabel;

     /** The listing of the available HTTP rendezvous */
     ServerConnectWidget *HTTPRendezVous;     

     /** The label that goes with the HTTPRendezVous widget */
     QLabel *HTTPRendezVousLabel;

     /** 
     * The check box that  indicates whether we want to act as a relay  
     */
     QCheckBox *isRelay;

     /** 
     * The check box that  indicates whether we want to use  as a relay  
     */
     QCheckBox *useRelay;

     /** The listing of the available  relay servers */
     ServerConnectWidget *RelayServers;     

     /** The label that goes with the RelayServers widget */
     QLabel *RelayServersLabel;

     /** The server name if we present a static NAT address */
     QLineEdit *relayNATServer;

     /** The server port if we present a static NAT address */
     QLineEdit *relayNATPort;
 
     /** The label that goes with the public NAT address */
     QLabel *relayNATLabel;

     /** The peer advertisement object we are editing */
     void * peerAd;
  protected slots:
     /** 
      * Changes the different
      * fields from enabled or disabled depending on 
      * whether we are behind a firewall 
      */ 
     void toggleProxy();

     /**
      * Enables or disables the TCP options 
      */ 
     void toggleTCPOptions(){  
       toggleNetworkOptions(0);
     }

     /**
      * Enables or disables the HTTP options 
      */ 
     void toggleHTTPOptions(){  
       toggleNetworkOptions(1);
     }

     /**
      * Enables or disables the relay options  if acting as a relay
      */
     void toggleRelayActOptions();

     /**
      * Enables or disables the relay options  if using  a relay
      */
     void toggleRelayUseOptions();

     virtual void done ( int r );
};

