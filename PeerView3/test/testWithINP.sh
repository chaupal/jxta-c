#!/bin/sh
#
# Copyright (c) 2002 Sun Microsystems, Inc.  All rights
# reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. The end-user documentation included with the redistribution,
#    if any, must include the following acknowledgment:
#       "This product includes software developed by the
#       Sun Microsystems, Inc. for Project JXTA."
#    Alternately, this acknowledgment may appear in the software itself,
#    if and wherever such third-party acknowledgments normally appear.
#
# 4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
#    not be used to endorse or promote products derived from this
#    software without prior written permission. For written
#    permission, please contact Project JXTA at http://www.jxta.org.
#
# 5. Products derived from this software may not be called "JXTA",
#    nor may "JXTA" appear in their name, without prior written
#    permission of Sun.
#
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
# ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# ====================================================================
#
# This software consists of voluntary contributions made by many
# individuals on behalf of Project JXTA.  For more
# information on Project JXTA, please see
# <http://www.jxta.org/>.
#
# This license is based on the BSD license adopted by the Apache Foundation.
#
# $Id: testWithINP.sh,v 1.1 2005/10/13 16:59:44 exocetrick Exp $
#
 
###  Run all of the test code sequentially.

#set -vx
echo "Options selected"
verbose=0
logfile=0
alltests=1
detailsummary=0
toutput=0
parametertests=0
logfileExt="log"
custprogramsfile="tests.inp"
toutputfile=""
declare -a programs
declare -a testsPassed
declare -a testsFailed
declare -a custtestsPassed
declare -a custtestsFailed
declare -a testGroups
declare -a ignoreGroups
declare -a summaryReport
declare -a pgms
totalcust=0
totaladv=0
totalfunc=0
teeout=""

#
# This function executes the the command lines that are in the programs array
# The programs array contains 2 extra positions for failed/passed status and
# the return code from the test application.
# 
# It also populates the testsFailed and testsPassed arrays.
# 
function runTest() {
tcnt=${#programs[@]}
unset testsFailed[*]
unset testsPassed[*]
#
# Iterate throught the programs to be executed
#
for (( l = 0 ; l < tcnt ; l+=3 ))    
do
  entry=${programs[$l]}
  cmdline="$entry "
  # extract just the program name without parameters
  for whichp in $entry
    do
       whichprogram=$whichp
       break
    done
  passed="true"
  echo "Executing" $whichprogram "..."
  timestamp=`date`
  # start creating a log file for the run of this program
  echo "Test $cmdline LOG ---------------------- $timestamp" >$whichprogram.$logfileExt
  echo "Test Output BEGIN ------------------------------------------------------" >>$whichprogram.$logfileExt
  if ([ $verbose -eq 1 ]) 
     then
        # If the output is tee'd as well 
        if ([ $toutput -eq 1 ]) then
           ./$cmdline | tee $whichprogram.$logfileExt 
        else 
           ./$cmdline
        fi
     else
        ./$cmdline >>$whichprogram.$logfileExt
  fi
  ret=$?
  if [ $ret -ne 0 ] 
  then   
    passed="false"
  fi
  if ([ "$passed" = "false" ])
  then
    echo "..." $whichprogram " failed"
    testsFailed=( "${testsFailed[@]}" "$whichprogram rc=$ret")
    programs[l+1]="rc = $ret"
    programs[l+2]="failed"
  else
    echo "..." $whichprogram "passed"
    testsPassed=( "${testsPassed[@]}" "$whichprogram rc=$ret")
    programs[l+1]="rc = $ret"
    programs[l+2]="passed"
  fi
  timestamp=`date` 
  echo "Test Output END  ------------------------------------------------------  $timestamp" >>$whichprogram.$logfileExt
done

}

#
# retrieve the options from the command line
#
while getopts ":vl:i:tdx:?" Option
do 
   case $Option in
     # Verbose - do not redirect the stdout to a log file 
     v ) echo "$Option verbose [OPTIND=${OPTIND}]"
         verbose=1
         ;;
     # Log file extension - Create log files with this extension
     l ) logfileExt=$OPTARG
         echo "Log file Extension $OPTARG"
         ;;
     i ) echo "Custom program file name  \"$OPTARG\" "
         custprogramsfile=$OPTARG
         ;;
     t ) toutput=1
         ;;
     d ) detailsummary=1
         echo "Summary detail"
         ;;
     x ) ignoreGroups=( "${ignoreGroups[@]}" "$OPTARG" )
         echo "Ignore group " "$OPTARG"
         ;;
     ? | h) echo "Usage -v -- verbose (Output all messages from tests)"
        echo "      -l extension -- log file extension"
        echo "      -i name -- custom tests file name"
        echo "      -t name -- tee output file name"
        echo "      -d --- include test detail in summary report"
        echo "      -x group name --- ignore this test group"
        echo "      -h/? - print this"
        exit 0
         ;;
     *) echo "Unimplemented option chosen."
         ;;
     esac
done
             
if ([ $verbose -eq 1 ])
then
  echo "HA HA Be verbose"           
fi
if ([ $logfile -eq 1 ])
then
  echo "Create a log file"
fi

            
timestamp=`date`
if ([ "$custprogramsfile" != "noprograms" ])
  then
  echo
  filename=$custprogramsfile
  # line stores the one single line from $filename
  lastTestGroup=""
  tests=0
  lineread=0
  ignore=0
  while read line
   do
       lineread=1
       linelength=${#line}
       if ([ "$linelength" = "0" ]) then
          continue 
       fi
       pos="`expr index "$line" \#`"
       if ([ "$pos" = "1" ]) then
          continue
       fi
       indicator="`expr match "$line" '.*\(TestGroup:\)'`"
       header=${line#*:*}
       if ([ "$indicator" != "TestGroup:" ]) then
          if ([ $ignore -eq 1 ]) then
             continue
          fi
          tmpPrograms=( "${tmpPrograms[@]}" "$line ")
          tmpPrograms=( "${tmpPrograms[@]}" "rc")
          tmpPrograms=( "${tmpPrograms[@]}" "passed/failed")
          let "tests = tests + 1"
       else
          igroupcnt=${#ignoreGroups[@]}
          ignore=0
          for (( z = 0 ; z < igroupcnt ; z += 1))
             do
                if ([ "${ignoreGroups[$z]}" = "$header" ]) then
                   echo "Group $header being ignored"
                   ignore=1
                   break
                fi
             done
          if ([ $ignore -eq 1 ]) then
              continue
          fi
          if ([ "$lastTestGroup" != "" ]) then
             testGroups=( "${testGroups[@]}" "$tests")
          fi
          testGroups=( "${testGroups[@]}" "$header")
          lastTestGroup="$header"
          tests=0
       fi
   done < $filename

   if ([ $lineread -eq 0 ]) then
      echo "**************** No test input records in $filename ******************"
      exit
   fi
   dateTest=`date`
   echo "************************************************************************************"
   echo "                           JXTA Testing started at: $dateTest"
   echo "************************************************************************************"
   echo
   testGroups=( "${testGroups[@]}" "$tests" )
   cnt=${#testGroups[@]}
   globalcnt=0
   for (( i = 0 ; i < cnt ; i+=2 ))
     do
        groupcnt=${testGroups[i+1]}
        if ([ "$groupcnt" = "0" ]) then
           continue
        fi
        starttime=`date`
        echo "Test Group ------ ${testGroups[i]} with ${testGroups[i+1]} tests"
        unset programs[*]
        for (( j = 0; j < groupcnt; j++ ))
           do
              programs=( "${programs[@]}" "${tmpPrograms[globalcnt]}" )
              programs=( "${programs[@]}" "rc" )
              programs=( "${programs[@]}" "passed/fail" )
              let "globalcnt = globalcnt + 3"
           done
        runTest
        endtime=`date`
        custpassedcnt=${#testsPassed[@]}
        custfailedcnt=${#testsFailed[@]}
        totalcust=$custpassedcnt
        let "totalcust = totalcust + custfailedcnt"
        if ([ $totalcust -eq 0 ]) then
             continue
        fi
        passedcnt=$custpassedcnt
        echo "------------------ number of tests passed $passedcnt in ${testGroups[i]}"
        for (( k = 0 ; k < passedcnt ; k++ ))
           do
              custtestsPassed[$k]="${testsPassed[$k]}"
           done
        failedcnt=$custfailedcnt
        echo "------------------ number of tests failed $failedcnt in ${testGroups[i]}"
        for (( k = 0 ; k < failedcnt ; k++ ))
           do
             custtestsFailed[$k]="${testsFailed[$k]}"
           done
        sumcnt=${#summaryReport[@]}
        summaryReport[sumcnt]="${testGroups[i]}"
        summaryReport[sumcnt+1]="$passedcnt"
        summaryReport[sumcnt+2]="$failedcnt"
        summaryReport[sumcnt+3]="$starttime"
        summaryReport[sumcnt+4]="$endtime"
        summaryReport[sumcnt+5]=${testsPassed[*]}
        summaryReport[sumcnt+6]=${testsFailed[*]}
     done
     echo "sumcnt = $sumcnt"
     dateTest=`date`
     echo "************************************************************************************"
     echo "                           JXTA Testing ended at: $dateTest  " 
     echo "************************************************************************************"
     echo
     echo  
     sumcnt=${#summaryReport[@]}
     if ([ "$sumcnt" = "0" ]) then
        echo "********************  No results to report *********************************"
     else
        echo "**************************** JXTA Test Group Summary ***************************************"
        flagErr=0
        for (( x = 0 ; x < sumcnt ; x+=7))
           do
              passedcnt=${summaryReport[$x+1]}
              failedcnt=${summaryReport[$x+2]}
              let "totalcnt = passedcnt + failedcnt"
              echo "     ${summaryReport[$x]}"
              echo "                       Total  $totalcnt     Passed $passedcnt      Failed $failedcnt"
              echo
              if ([ $failedcnt -gt 0 ]) then
                 flagErr=1
              fi
           done
        if ([ $detailsummary -eq 1 ]) then
           for (( x = 0 ; x < sumcnt ; x+=7))
              do
                 passedcnt=${summaryReport[$x+1]}
                 failedcnt=${summaryReport[$x+2]}
                 start=${summaryReport[$x+3]}
                 stop=${summaryReport[$x+4]}
                 let "totalcnt = passedcnt + failedcnt"
                 echo "**********************************************************************************"
                 echo "Test Group --------------  ${summaryReport[$x]} "
                 echo
                 echo "                       Total  $totalcnt     Passed $passedcnt      Failed $failedcnt"
                 echo
                 echo "                       Start: $start"
                 echo "                        Stop: $stop "
                 echo
                 echo " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"
                 echo
                 echo "----------------------------------"
                 echo "        Passed"
                 echo "----------------------------------"
                 passeddetail=${summaryReport[$x+5]}
                 m=0
                 for detail in $passeddetail
                    do 
                       if ([ "$m" != "0" ]) then
                          echo "$outstring $detail"
                          m=0
                       else
                          outstring="$detail"
                          m=1
                       fi
                    done
                 echo
                 echo "----------------------------------"
                 echo "        Failed"
                 echo "----------------------------------"
                 faileddetail=${summaryReport[$x+6]}
                 m=0
                 for detail in $faileddetail
                    do 
                       if ([ "$m" != "0" ]) then
                          echo "$outstring $detail"
                          m=0
                       else
                          outstring="$detail"
                          m=1
                       fi
                    done
                 echo
              done
           else
              if ([ $flagErr -eq 1 ]) then
                 echo "************************************************************************************"
                 echo "There were failed tests - To see the detail run with -d option"
              fi
           fi
        echo "************************************************************************************"
     fi

fi

    

