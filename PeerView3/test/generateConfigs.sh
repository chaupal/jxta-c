#!/bin/sh
#
# This script will create runtime environments for the jxtaShell along with executable
# script files that remove any log files, the cm cache and run the jxtaShell. Your customization
# should be put in the back of the file. Take a look.
#
#
# build a PlatformConfig file using the variables
# This is a simple PlatformConfig. Of course, you can 
# create any elements that you need with any variables you need.
#

function buildPlatform() {
  NL=$'\r'
  pidstart="<PID>urn:jxta:uuid-59616261646162614E504720503250335637AE3FDD5341D9B490B36198$port"
  pidend="03</PID>"
  platform="<?xml version=\"1.0\"?>$NL
<!DOCTYPE jxta:PA>$NL
<jxta:PA xmlns:jxta=\"http://jxta.org\">$NL
$pidstart$pidend$NL
<GID>urn:jxta:jxta-NetGroup</GID>$NL
<Name>JXTA-C Peer $type $port</Name>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000605</MCID>$NL
<Parm>$NL
<jxta:RdvConfig xmlns:jxta='http://jxta.org' config='$type' $autointerval$NL
     leaseDuration='180000'>$NL
$seeds$NL
<peerview $NL
    pIntervalPeerView='30000'$NL
    pveExpiresPeerView='12000000'$NL
    maxProbed='1' rdvaRefreshPeerView='300000'/>$NL
</jxta:RdvConfig>$NL
</Parm>$NL
</Svc>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000905</MCID>$NL
<Parm>$NL
<jxta:TransportAdvertisement xmlns:jxta='http://jxta.org' type='jxta:TCPTransportAdvertisement'>$NL
<Protocol>tcp</Protocol>$NL
<Port>$port</Port>$NL
$multicastoff<MulticastAddr>224.0.1.85</MulticastAddr>$NL
<MulticastPort>1234</MulticastPort>$NL
<MulticastSize>16384</MulticastSize>$NL
<InterfaceAddress>127.0.0.1</InterfaceAddress>$NL
<Server></Server>$NL
<ConfigMode>auto</ConfigMode>$NL
</jxta:TransportAdvertisement>$NL
</Parm>$NL
</Svc>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000A05</MCID>$NL
<Parm>$NL
<jxta:TransportAdvertisement xmlns:jxta='http://jxta.org' type='jxta:HTTPTransportAdvertisement'>$NL
<Protocol>http</Protocol>$NL
<InterfaceAddress>0.0.0.0</InterfaceAddress>$NL
<ConfigMode>auto</ConfigMode>$NL
<Port>$httpport</Port>$NL
<Proxy></Proxy>$NL
<ProxyOff/><ServerOff/>$NL
</jxta:TransportAdvertisement>$NL
</Parm>$NL
</Svc>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000F05</MCID>$NL
<Parm>$NL
<jxta:RelayAdvertisement xmlns:jxta='http://jxta.org' type='jxta:RelayAdvertisement'>$NL
<isClient>true</isClient>$NL
<isServer>true</isServer>$NL
</jxta:RelayAdvertisement>$NL
</Parm>$NL
</Svc>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000002105</MCID>$NL
<Parm>$NL
<!-- JXTA SRDI Configuration Advertisement -->$NL
<jxta:SrdiConfig xmlns:jxta='http://jxta.org' type='jxta:SrdiConfig' SRDIDeltaSupport = 'no'>$NL
<NoRangeReplication withValue='false'/>$NL
<ReplicationThreshold>4</ReplicationThreshold>$NL
</jxta:SrdiConfig>$NL
</Parm>$NL
</Svc>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000805</MCID>$NL
<Parm>$NL
<!-- JXTA EndPoint Configuration Advertisement -->$NL
<jxta:EndPointConfig xmlns:jxta='http://jxta.org' type='jxta:EndPointConfig'>$NL
<!-- NegativeCache timeout - seconds -->$NL
<NegativeCache$NL
  timeout='15' maxTimeOut='300'$NL
  queueSize='5' msgToRetry='20'$NL
/>$NL
</jxta:EndPointConfig>$NL
</Parm>$NL
</Svc>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000305</MCID>$NL
<Parm>$NL
</Parm>$NL
</Svc>$NL
<Svc>$NL
<MCID>urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000002205</MCID>$NL
<Parm>$NL
<!-- JXTA Cache Configuration Advertisement -->$NL
<jxta:CacheConfig xmlns:jxta='http://jxta.org' type='jxta:CacheConfig'$NL
        sharedDB='yes' threads='0' threads_max='0'>$NL

    <AddressSpace  name='highPerformanceAddressSpace'$NL
        type='persistent'$NL
        highPerformance='yes'$NL
        autoVacuum='no'$NL
        default='yes'$NL
        DBName='highPerf'$NL
        DBEngine='sqlite3'$NL
        location='local'$NL
        xactionThreshold='50'$NL
    />$NL

</jxta:CacheConfig>$NL
</Parm>$NL
</Svc>$NL
</jxta:PA>$NL
"
}

# build the commands to execute within the run script

function buildExecLines {
removelog="rm ./$type$port/log*"
removevalgrind="rm ./$type$port/valgrind*.*"
removedb="rm ./$type$port/.cm/*.sqdb"
  if ([ "$logfile" = "true" ])
  then
      log=" -f log$type$port"
  else
      log=""
  fi
execline="gnome-terminal --geometry=180x7+0+0 --hide-menubar --title=$type$port --working-directory=$PWD/$type$port -e \"jxtaShell $verbosity$log\"$NL
"
}

# build environment for individual peers

function buildEnvirons {
    echo Creating environment "for" $peers $type peers --- script name $runfile.sh
    for (( i = 0 ; i < $peers ; i+=1 )) {
        let "port = port + 1"
        let "httpport = httpport + 1"
	if ( [ -d $type$port ] ) 
        then
            rm ./$type$port/PlatformConfig
        else
            mkdir $type$port
        fi
        buildPlatform
        echo $platform >>./$type$port/PlatformConfig

        buildExecLines

# --------------------   MODIFY THE FOLLOWING CODE TO BUILD THE BASH SCRIPTS ----------------------------
#
# see the function buildExecLines() as an example how the script can be built
#
        echo $removevalgrind >>$runfile.sh
        echo $removelog >>$runfile.sh
        echo $removedb >>$runfile.sh
        echo $execline >>$runfile.sh
        echo sleep 3 >>$runfile.sh

# --------------------   END MODIFY ----------------------------

    }
}
#  ------------------------   DEFAULT VARIABLES ----------------------------
# name of bash script for running peers
runfile="run"

# number of peers to create 
peers=1

# create a log file from JXTA
logfile="false"

#
#   variables to set for creating the PlatformConfig
#
# type of rendezvous peer
#      rendezvous
#      client
#      adhoc
type="rendezvous"

# port numbers for tcp and http services these are the starting port numbers.
# For each subsequent peer created the port number is incremented
port=9700
httpport=9600

# for auto rendezvous this is the interval for the auto cycle
autointerval="autoRendezvousInterval='10000'"

# seeds xml for PlatformConfig
seeds=""

# xml to turn multicast off
multicastoff="<multicastOff />"

platform=""


# --------------------   MODIFY THE FOLLOWING TO SUIT YOUR NEEDS  ----------------------------
# build PlatformConfigs and create scripts
#

# remove any that are there

rm ./run1.sh
rm ./run2.sh
rm ./run3.sh
rm ./run4.sh

# -------------- a set of client peers executed with run1.sh
runfile=run1
type="client"
verbosity="-v -v"
logfile="true"
peers=5
buildEnvirons

# -------------- a set of rendezvous peers executed with run2.sh

runfile=run2
type="rendezvous"
logfile="true"
peers=2
buildEnvirons

# -------------- a set of client peers executed with run3.sh

runfile=run3
peers=2
buildEnvirons

# -------------- a set of client peers executed with run4.sh

runfile=run4
peers=2
buildEnvirons

# change to executable

chmod +x ./run1.sh
chmod +x ./run2.sh
chmod +x ./run3.sh
chmod +x ./run4.sh
# --------------------   END MODIFY ----------------------------

