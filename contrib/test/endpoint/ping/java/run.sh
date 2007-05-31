#!/bin/sh
#
# $Id: run.sh,v 1.3 2002/03/08 02:55:25 lomax Exp $
#

exec /usr/jdk1.3/bin/java -classpath j2sepeer.jar -Dnet.jxta.tls.principal="relay" -Dnet.jxta.tls.password="testingnow" net.jxta.impl.peergroup.Boot





