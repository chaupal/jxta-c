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
 * $Id: util.cpp,v 1.3 2002/03/12 13:45:47 wiarda Exp $
 */

#include <qapplication.h>
#include <qfont.h>
#include <qpalette.h> 
#include <qcolor.h> 
#include <qwindowsstyle.h> 


void updateLookAndFeel(QApplication * qtApp){
    qtApp->setFont(QFont( "Times", 14 ),false);
    
    /*QPalette palette;

    QColorGroup active,disabled,inactive;

    active.setColor(QColorGroup::Background,QColor(102, 102, 153));
    active.setColor(QColorGroup::Foreground,Qt::white);
    active.setColor(QColorGroup::Base,Qt::white);
    active.setColor(QColorGroup::Text,Qt::black);
    active.setColor(QColorGroup::Button,QColor(102, 102, 153));
    active.setColor(QColorGroup::ButtonText,Qt::white);

    inactive.setColor(QColorGroup::Background,QColor(102, 102, 153));
    inactive.setColor(QColorGroup::Foreground,Qt::white);
    inactive.setColor(QColorGroup::Base,Qt::white);
    inactive.setColor(QColorGroup::Text,Qt::black);
    inactive.setColor(QColorGroup::Button,QColor(102, 102, 153));
    inactive.setColor(QColorGroup::ButtonText,Qt::white);

    disabled.setColor(QColorGroup::Background,QColor(102, 102, 153));
    disabled.setColor(QColorGroup::Foreground,Qt::black);
    disabled.setColor(QColorGroup::Base,QColor(255,255,255));
    disabled.setColor(QColorGroup::Text,Qt::black);
    disabled.setColor(QColorGroup::Button,QColor(102, 102, 153));
    disabled.setColor(QColorGroup::ButtonText,Qt::gray);

    palette.setActive(active);
    palette.setInactive(inactive);
    palette.setDisabled(disabled);
        
    qtApp->setPalette(palette,true);
    */
}




