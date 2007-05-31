/* 
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: ConfigDialog.cpp,v 1.4 2002/04/25 21:59:14 wiarda Exp $
 */

#include <qdialog.h> 
#include <qpushbutton.h> 
#include <qhbox.h>
#include <qvbox.h>
#include <qsignalmapper.h> 
#include <qtabwidget.h> 
#include <qframe.h> 
#include <qlabel.h> 
#include <qlineedit.h> 
#include <qcheckbox.h> 
#include <qsizepolicy.h> 
#include <qlayout.h>
#include <qnamespace.h> 
#include <qcombobox.h> 
#include <qgroupbox.h> 
#include <qwhatsthis.h>
#include <qlistbox.h> 
#include <qmessagebox.h> 

#include "ConfigDialog.h"
#include "c_wrapper.h"

#include <iostream>
#include <string.h>

// defintions for ServerConnectWidge
ServerConnectWidget::ServerConnectWidget(QWidget * parent, const char * name, WFlags f):QWidget(parent,name,f){
  QGridLayout *grid = new QGridLayout(this,2,3);  
  grid->setColStretch(0,6);
  grid->setColStretch(1,4);
  grid->setColStretch(2,0);
  grid->setRowStretch(0,0);
  grid->setRowStretch(1,10);
  server = new QLineEdit(this,"ServerConnectWidget::server");
  grid->addWidget(server,0,0);
  port = new QLineEdit(this,"ServerConnectWidget::port");
  grid->addWidget(port,0,1);
  QPushButton *button = new QPushButton("+",this,"ServerConnectWidget::+");
  grid->addWidget(button,0,2,Qt::AlignLeft);
  connect(button,SIGNAL(clicked()),this,SLOT(addServer()));
  serverList = new QListBox(this,"ServerConnectWidget::ListBox");
  serverList->setColumnMode(1);
  grid->addMultiCellWidget(serverList,1,1,0,1);
  button = new QPushButton("-",this,"ServerConnectWidget::-");
  connect(button,SIGNAL(clicked()),this,SLOT(removeServer()));
  grid->addWidget(button,1,2,Qt::AlignLeft|Qt::AlignTop);
}

ServerConnectWidget::~ServerConnectWidget(){
}

void ServerConnectWidget::fillList(const QString & location){
}

const QString * ServerConnectWidget::getServer(){
   QString *result = new QString();
   QListBoxItem *item = serverList->firstItem();
   bool addLineBreak = false;

   while( item != 0){
     if( addLineBreak) result->append("\n");
     addLineBreak = true;
     result->append(item->text());
     item = item->next();
   }
   return result;
}

void ServerConnectWidget::addServer(){
  QString text = server->text().stripWhiteSpace();
  if( !text.isEmpty() ){
     text.append(":");
     QString portNo = port->text().stripWhiteSpace();
     if( !portNo.isEmpty()){     
       text.append(portNo);
       serverList->insertItem(text);        
     }
  } 
}

void ServerConnectWidget::removeServer(){
  int i = serverList->currentItem ();
  if( i >=  0 ) serverList->removeItem(i);
  
}
// end defintions for ServerConnectWidge


// definitions for the ConfigDialog class 
JxtaConfigDialog::JxtaConfigDialog(QWidget * parent,
                                   const char * name,
                                   void * ad):
                                   QDialog(parent,name,true){
    peerAd = ad;

    traceLevelNumber = 5;
    traceLevelValues = new (char**)[traceLevelNumber];
    traceLevelValues[0] = new (char*)[2];
    traceLevelValues[0][0] = "error";
    traceLevelValues[0][1] = "Error level";
    traceLevelValues[1] = new (char*)[2];
    traceLevelValues[1][0] = "warn";
    traceLevelValues[1][1] = "Warning level";
    traceLevelValues[2] = new (char*)[2];
    traceLevelValues[2][0] = "info";
    traceLevelValues[2][1] = "Info level";
    traceLevelValues[3] = new (char*)[2];
    traceLevelValues[3][0] = "debug";
    traceLevelValues[3][1] = "Debug level";
    traceLevelValues[4] = new (char*)[2];
    traceLevelValues[4][0] = "user default";
    traceLevelValues[4][1] = "User defined level";

    QVBoxLayout *container = new QVBoxLayout( this);
    QSizePolicy policy(QSizePolicy::Preferred,QSizePolicy::Preferred);

    setCaption(tr(name));

    // Add the what is this option
    QHBoxLayout *whatIs = new QHBoxLayout( container);
    QHBox *box = new QHBox(this);
    QWhatsThis::whatsThisButton (box);       
    whatIs->addStretch(0);    
    whatIs->addWidget(box);


    // Add the different tabs 
    QTabWidget *tabber = new QTabWidget(this);
    container->addWidget(tabber);       
    addBasic(tabber);
    addAdvanced(tabber);
    addRendezvous(tabber);

    // Center the okay and cancel buttons at the bottom
    QHBoxLayout *buttonPanel = new QHBoxLayout( container);
    QPushButton * cancel =  new QPushButton(tr("Cancel"),this);
    QPushButton * okay = new QPushButton(tr("Okay"),this);
    buttonPanel->addStretch(10);
    buttonPanel->addWidget(cancel);
    buttonPanel->addWidget(okay);
    buttonPanel->addStretch(10);

    QSignalMapper *signal = new QSignalMapper(this);
    connect(signal,SIGNAL(mapped(int)),this,SLOT(done(int)));
    signal->setMapping(cancel,CANCEL);
    signal->setMapping(okay,OKAY);
    connect(cancel,SIGNAL(clicked()),signal,SLOT(map()));
    connect(okay,SIGNAL(clicked()),signal,SLOT(map()));

    setData();
}

void JxtaConfigDialog::setData(){
   const char *previous = 0;
   int selected = -1;

   if( peerAd != 0){
      peerName->setText(w_jxta_PA_get_Name(peerAd));

      previous = w_jxta_PA_get_Dbg(peerAd); 
      for(int i = 0; i < traceLevelNumber; i++){
          if( previous != 0 && strcmp(previous,traceLevelValues[i][0]) == 0 ){
	     selected = i;
	  }
      }
      if( selected != -1){
         traceLevel->setCurrentItem(selected); 
      }
   }
}

JxtaConfigDialog::~JxtaConfigDialog(){
  for(int i  = 0; i < traceLevelNumber; i++){
    delete []traceLevelValues[i];
  }
  delete []traceLevelValues;
  traceLevelValues = 0;
}

void JxtaConfigDialog::done ( int r ) {
  bool oneChecked;
  int i;

  if( r == OKAY){
    if( peerAd != 0){
      if( peerName->text().stripWhiteSpace().isEmpty()){
	QMessageBox::warning(this,tr("Missing Peer Name"),
                              tr("Please supply a peer name"));
	return;
      }
      w_jxta_PA_set_Name(peerAd,peerName->text().latin1());
      if( proxy->isChecked() ){
         if( proxyServer->text().stripWhiteSpace().isEmpty() ||
             proxyPort->text().stripWhiteSpace().isEmpty() ){
            QMessageBox::warning(this,tr("Missing Proxy Address"),
                            QString("You selected to use a proxy address.\n")+
                            "However you did not supply the proxy server");
	    return;		
	 }
      }

      w_jxta_PA_set_Dbg(peerAd,traceLevelValues[traceLevel->currentItem()][0]);
      oneChecked = false;
      for(i = 0; i < 2; i++){
         if( networkSettings != 0 && 
             networkSettings[i] != 0 && 
             networkSettings[i]->isChecked() ){
 	     oneChecked = true;
             if( (networkPorts != 0 && 
                 networkPorts[i] != 0 &&
                 networkPorts[i]->text().stripWhiteSpace().isEmpty() ) ||
                 (networkHostname != 0 && 
                 networkHostname[i] != 0 &&
                 networkHostname[i]->currentText().stripWhiteSpace().isEmpty()) ){
 	         QString message;
		 if( i = 0 ) {
                    message.append("Please supply your TCP server and port");
		 }
                 else{
                     message.append("Please supply your HTTP server and port");
		 } 
                 QMessageBox::warning(this,tr("Missing Network Address"),
                                      message);
	        return;		
	     }
	 }
      }
      if( oneChecked == false){
	QMessageBox::warning(this,tr("Missing Network Address"),
                             "Please enable  either TCP or HTTP");
	return;
      }
    }
  }
  QDialog::done(r);
}

 
void JxtaConfigDialog::addBasic( QTabWidget *tabber ){
    static const char * peerNameInfo = "<b>Peer name:</b><br>The name your peer identifies itself to other peers.<p>While the unique identifier is the PID, this name is the one presented to users.";
    static const char * proxyInfo = "If your are behind a firewall, other peers cannot connect directly to you.<br>Connection will be through a proxy.";

   QFrame *panel = new QFrame(tabber);
   QVBoxLayout *container = new QVBoxLayout( panel);

   container->setSpacing(3); 
   container->setMargin(5); 

   QHBoxLayout *boxLayout = new QHBoxLayout( container );
   QLabel *label = new QLabel(tr("Peer Name"),panel);
   QWhatsThis::add(label,peerNameInfo);
   peerName = new QLineEdit(panel);
   boxLayout->addWidget(label);
   boxLayout->addWidget(peerName);
   label  = new QLabel(tr("(Mandatory)"),panel);
   boxLayout->addWidget(label);
   boxLayout->addStretch(0);

   boxLayout = new QHBoxLayout( container );
   proxy = new QCheckBox(panel);
   label = new QLabel(proxy ,tr("Use proxy server if behind firewall"),panel);
   QWhatsThis::add(label,proxyInfo);
   boxLayout->addWidget(proxy);
   boxLayout->addWidget(label);
   label->setAlignment( AlignLeft );
   boxLayout->addStretch(0);

   boxLayout = new QHBoxLayout( container);
   proxyAddress = new QLabel(tr("Proxy Address"),panel);
   proxyServer = new QLineEdit(panel);
   proxyPort = new QLineEdit(panel);
   proxyAddress->setEnabled(false);
   proxyServer->setEnabled(false);
   proxyPort->setEnabled(false);
   boxLayout->addWidget(proxyAddress);
   boxLayout->addWidget(proxyServer);
   boxLayout->addWidget(proxyPort);
   boxLayout->addStretch(0);

   container->addStretch(0);

   connect(proxy,SIGNAL(toggled(bool)),this,SLOT(toggleProxy()));

   tabber->addTab(panel,tr("Basic"));
}

void JxtaConfigDialog::addAdvanced( QTabWidget *tabber ){
  static const char * traceLevelInfo = "Control the debug level.<br>The higher the level, the more output is written to the terminal window";

   QFrame *panel = new QFrame(tabber);
   QVBoxLayout *container = new QVBoxLayout( panel);

   container->setSpacing(3); 
   container->setMargin(5); 

   QHBoxLayout *boxLayout = new QHBoxLayout( container);
   QLabel *label  = new QLabel(tr("Trace Level"),panel);
   QWhatsThis::add(label,traceLevelInfo);
   boxLayout->addWidget(label);
   traceLevel = new QComboBox(false,panel);
   for(int i = 0; i < traceLevelNumber; i++){
      traceLevel->insertItem(tr(traceLevelValues[i][1]));   
   }
   boxLayout->addWidget(traceLevel);
   boxLayout->addStretch();
 
   // set the network options   
   networkSettings = new (QCheckBox*)[2];
   networkPorts = new (QLineEdit*)[2];
   networkManualAlways = new (QCheckBox*)[2];
   networkManualLabel = new (QLabel*)[2];
   networkHostname = new (QComboBox*)[2];

   QGroupBox * group = new QGroupBox(3,Vertical, tr("TCP setting"),panel);
   container->addWidget(group);
   label = new QLabel(tr("Enable if direct LAN connection"),group);
   QFrame *network = new QFrame(group);
   addNetworkSlot(network,0);   
   networkPorts[0]->setText("9701");
   connect(networkSettings[0],SIGNAL(toggled(bool)),this,
                              SLOT(toggleTCPOptions()));
   

   group = new QGroupBox(2,Vertical, tr("HTTP setting") , panel);
   container->addWidget(group);
   label = new QLabel(tr("Must be enabled if behind firewall/NAT"),group);
   network = new QFrame(group);
   addNetworkSlot(network,1);   
   networkPorts[1]->setText("9700");
   connect(networkSettings[1],SIGNAL(toggled(bool)),this,
                              SLOT(toggleHTTPOptions()));

   container->addStretch(0);
   tabber->addTab(panel,tr("Advanced"));
} 


void JxtaConfigDialog::addNetworkSlot(QWidget *parent,int index){
  static const char * enableInfo = "Enable tcp and/or http connection from your peer to the jxta network<br>If you want to act as a rendezvous peer, both http and tcp need to be enabled";
  static const char * alwaysManual ="If this is checked, you will see a configuration screen everytime JXTA is started.";

    QVBoxLayout *parentLayout = new QVBoxLayout( parent);

    parentLayout->setSpacing(3); 
    parentLayout->setMargin(5); 

    QHBoxLayout *layout = new QHBoxLayout( parentLayout);
    networkSettings[index] = new QCheckBox(parent);  
    networkSettings[index]->setChecked(true);
    layout->addWidget(networkSettings[index]);
    QLabel *label = new QLabel(networkSettings[index],tr("Enabled"),parent);
    QWhatsThis::add(label,enableInfo);
    layout->addWidget(label);
    layout->addStretch(0);
    
    layout = new QHBoxLayout( parentLayout);
    networkManualAlways[index] = new QCheckBox(parent); 
    layout->addWidget(networkManualAlways[index]);
    networkManualLabel[index]  = new QLabel(tr("Always manual"),parent); 
    QWhatsThis::add(networkManualLabel[index],alwaysManual);
    layout->addWidget(networkManualLabel[index]);
    networkHostname[index] = new QComboBox(true,parent);
    networkHostname[index]->insertItem("localhost.localdomain");
    layout->addWidget(networkHostname[index] );
    networkPorts[index] = new QLineEdit(parent);  
    layout->addWidget(networkPorts[index]);
    layout->addStretch(0);    

    // if index is 0, add the option public NAT address
    if( index == 0){
      layout = new QHBoxLayout( parentLayout);
      publicNATLabel = new QLabel(tr("(Optional) Public NAT address"),parent);
      layout->addWidget(publicNATLabel);
      publicNATServer = new QLineEdit(parent);
      layout->addWidget(publicNATServer);
      publicNATPort = new QLineEdit(parent);
      layout->addWidget(publicNATPort);
      layout->addStretch(0);  
    }
}

void JxtaConfigDialog::addRendezvous( QTabWidget *tabber ){
    QFrame *panel = new QFrame(tabber);
    QGridLayout *container = new QGridLayout(panel,3,1);

    container->setSpacing(3); 
    container->setMargin(5); 
    container->setRowStretch(0,5);
    container->setRowStretch(1,5);
    container->setRowStretch(2,0);

    // The rendezvous settings 
    QGroupBox * group = new QGroupBox(1,Vertical, tr("Rendezvous Settings") ,
                                      panel,"GGG");
    container->addWidget(group,0,0);    
    QFrame *frame = new QFrame(group,"RendezVous group");
    QGridLayout *grid = new QGridLayout(frame,3,2);  
    grid->setColStretch(0,5);
    grid->setColStretch(1,5);
    grid->setRowStretch(0,0);
    grid->setRowStretch(1,0);
    grid->setRowStretch(2,10);    
    QHBox *box = new QHBox(frame);
    grid->addMultiCellWidget(box,0,0,0,1,Qt::AlignLeft);
    isRendezVous = new QCheckBox(box);
    QLabel *label = new QLabel(tr("Act as a Rendezvous"),box);
    TCPRendezVousLabel = new QLabel(tr("Available TCP Rendezvous"),frame);
    grid->addWidget(TCPRendezVousLabel,1,0);
    HTTPRendezVousLabel = new QLabel(tr("Available HTTP Rendezvous"),frame);
    grid->addWidget(HTTPRendezVousLabel,1,1);
    TCPRendezVous = new   ServerConnectWidget(frame);
    grid->addWidget(TCPRendezVous,2,0); 
    HTTPRendezVous = new   ServerConnectWidget(frame);
    grid->addWidget(HTTPRendezVous,2,1); 

    // add the relay options
    group = new QGroupBox(1,Vertical, tr("HTTP Relay Settings"), panel);
    container->addWidget(group,1,0);    
    frame = new QFrame(group);
    grid = new QGridLayout(frame,5,1);
    grid->setRowStretch(0,0); 
    grid->setRowStretch(1,0); 
    grid->setRowStretch(2,0); 
    grid->setRowStretch(3,10);
    box = new QHBox(frame);
    grid->addWidget(box,0,0,Qt::AlignLeft|Qt::AlignTop);
    isRelay = new QCheckBox(box);
    connect(isRelay,SIGNAL(toggled(bool)),this,SLOT(toggleRelayActOptions()));
    label = new QLabel(
                    tr("Act as a relay (Both TCP and HTTP must be enabled)"),
                       box);    
    box = new QHBox(frame);
    grid->addWidget(box,1,0,Qt::AlignLeft|Qt::AlignTop); 
    relayNATLabel = new QLabel(tr("Public address (static NAT address)"),box);
    relayNATServer = new QLineEdit(box);
    relayNATPort = new QLineEdit(box);
    relayNATServer->setEnabled(false);
    relayNATPort->setEnabled(false);
    relayNATLabel->setEnabled(false);

    box = new QHBox(frame);
    grid->addWidget(box,2,0,Qt::AlignLeft|Qt::AlignTop);
    useRelay = new QCheckBox(box);
    useRelay->setChecked(true);
    connect(useRelay,SIGNAL(toggled(bool)),this,SLOT(toggleRelayUseOptions()));
    label = new QLabel(tr("Use a relay (Required if behind firewall/NAT"),box);
    RelayServersLabel = new QLabel(tr("Available relays"),frame);
    grid->addWidget(RelayServersLabel,3,0,Qt::AlignLeft|Qt::AlignTop);
    RelayServers = new ServerConnectWidget(frame);
    grid->addWidget(RelayServers,4,0,Qt::AlignLeft);
   
    tabber->addTab(panel,tr("Rendezvous/Relays"));
}

void JxtaConfigDialog::toggleProxy(){
   proxyAddress->setEnabled(proxy->isChecked ());
   proxyServer->setEnabled(proxy->isChecked ());
   proxyPort->setEnabled(proxy->isChecked ());
}

void JxtaConfigDialog::toggleNetworkOptions(int index){
  if( index < 0 || index > 1) return;
 
  if( networkSettings == 0 || networkSettings[index] == 0) return;
  if( networkPorts != 0 && networkPorts[index] != 0){
     networkPorts[index]->setEnabled(networkSettings[index]->isChecked() );
  }
  if( networkManualAlways != 0 && networkManualAlways[index] != 0){
     networkManualAlways[index]->setEnabled(networkSettings[index]->isChecked() );
  }
  if( networkManualLabel!= 0 && networkManualLabel[index] != 0){
     networkManualLabel[index]->setEnabled(networkSettings[index]->isChecked() );
  }
  if( networkHostname != 0 && networkHostname[index] != 0){
     networkHostname[index]->setEnabled(networkSettings[index]->isChecked() );
  }
  if( index == 0){
    if( publicNATLabel != 0) {
      publicNATLabel->setEnabled(networkSettings[index]->isChecked());
    }
    if( publicNATServer != 0) {
      publicNATServer->setEnabled(networkSettings[index]->isChecked());
    }
    if( publicNATPort != 0) {
      publicNATPort->setEnabled(networkSettings[index]->isChecked());
    }
    if( TCPRendezVous != 0){
        TCPRendezVous->setEnabled(networkSettings[index]->isChecked());
    }
    if( TCPRendezVousLabel != 0){
        TCPRendezVousLabel->setEnabled(networkSettings[index]->isChecked());
    }
  }
  else{
    if( HTTPRendezVous != 0){
        HTTPRendezVous->setEnabled(networkSettings[index]->isChecked());
    }
    if( HTTPRendezVousLabel != 0){
        HTTPRendezVousLabel->setEnabled(networkSettings[index]->isChecked());
    }
  }
}

void JxtaConfigDialog::toggleRelayActOptions(){
  if( isRelay == 0) return;
  if( relayNATServer != 0){
    relayNATServer->setEnabled( isRelay->isChecked() );
  }
  if( relayNATPort != 0){
    relayNATPort->setEnabled( isRelay->isChecked() );
  }
  if( relayNATLabel != 0){
    relayNATLabel->setEnabled( isRelay->isChecked() );
  }
}

void JxtaConfigDialog::toggleRelayUseOptions(){
  if( useRelay == 0) return;
  if( RelayServersLabel != 0){
    RelayServersLabel->setEnabled( useRelay->isChecked() );
  }
  if( RelayServers != 0){
    RelayServers->setEnabled( useRelay->isChecked() );
  }
}
// end definitions for the ConfigDialog class 








