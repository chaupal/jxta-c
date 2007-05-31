#!/bin/bash


######################################################################
######################################################################
##
## This will ultimately replace the "adparsegen.sh" script, and will
## later be moved to a different location in the source tree.
 


# The name of the xml ad must be the first command line 
# argument because it is used to set the output file names.
cfile=$1.c
hfile=$1.h


if [ -e $cfile ]; then
#   echo Removing old file...
   rm $cfile
fi

if [ -e $hfile ]; then
#   echo Removing old file...
   rm $hfile
fi



function copyrightfunc
{

echo "/* 
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
 *       \"This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA.\"
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names \"Sun\", \"Sun Microsystems, Inc.\", \"JXTA\" and \"Project JXTA\" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact Project JXTA at http://www.jxta.org.
 *
 * 5. Products derived from this software may not be called \"JXTA\",
 *    nor may \"JXTA\" appear in their name, without prior written
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
 * \$Id: newads.sh,v 1.8 2002/03/02 18:50:07 ddoolin Exp $
 */

   " >> $1

}

function compilefunc 
{

echo "/* 
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/" >> $cfile 	
echo "/*
* gcc -DSTANDALONE jxta_advertisement.c $1.c  -o $1 \\
  \`/usr/local/apache2/bin/apr-config --cflags --includes --libs\` \\
  -lexpat -L/usr/local/apache2/lib/ -lapr -laprutil 
  -L./.libs -ljxta -Ljpr/.libs -ljpr -lgdbm -ldb -ldbm -ldb1 \\
*/" >> $cfile 

}

# Function for producing the #includes.
function includefunc
{
   echo \  >> $cfile

   echo "#include <stdio.h>"   >> $cfile
   echo "#include <string.h>"  >> $cfile
   echo "#include \"$hfile\""  >> $cfile

   echo \  >> $cfile

   echo "#ifdef __cplusplus"   >> $cfile
   echo "extern \"C\" {"       >> $cfile
   echo "#endif"               >> $cfile

}


# Function for producing the enum.
function enumfunc
{
 
   echo \ >> $cfile
   echo "/** Each of these corresponds to a tag in the 
 * xml ad.
 */" >> $cfile


   echo  enum tokentype { >> $cfile
    
   echo    "                "Null_,  >> $cfile
  
   for name in "$@" 
   do
      echo "                "$name"_," >> $cfile
   done

   echo "               };" >> $cfile
   echo \  >> $cfile

}


# Function for producing the main struct
function structfunc
{

   echo \ >> $cfile
   echo "/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */" >> $cfile
   echo "struct _$1 {" >> $cfile
   echo "   Jxta_advertisement jxta_advertisement;"  >> $cfile
   for name in "$@" 
   do
      echo "   char * $name;" >> $cfile
   done

   echo "};" >> $cfile

   echo \ >> $cfile
}




# Function for producing a dispatch table.
function dispatchfunc
{
   echo \ >> $cfile
   echo "/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */" >> $cfile
   echo "const static Kwdtab $1_tags[] = {" >> $cfile

   echo "{\"Null\",Null_,{NULL},NULL}," >> $cfile
   for name in "$@" 
   do
      echo "{\"$name\",$name"_,{*handle$name},NULL}, >> $cfile
   done
   echo "{NULL,0,{0},NULL}" >> $cfile
   echo "};"  >> $cfile

}



function handlerfunc
{
   echo \ >> $cfile
   echo "/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */"  >> $cfile
   for name in "$@" 
   do
      echo "static void" >> $cfile
      echo "handle$name(void * userdata, const XML_Char * cd, int len) {" >> $cfile
      echo "   /* $1 * ad = ($1*)userdata; */" >> $cfile
      echo "   printf(\"In $name element\n\");" >> $cfile
      echo "}" >> $cfile 
      echo \ >> $cfile
   done

   echo \ >> $cfile

}


# Function for producing get/set methods.
function getsetfunc
{
   echo \ >> $cfile
   echo "/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */" >> $cfile
   for name in "$@" 
   do
      echo "char *" >> $cfile
      echo "$1_get_$name($1 * ad) {" >> $cfile
      echo "   return NULL;"  >> $cfile
      echo "}"   >> $cfile
      echo \ >> $cfile

      echo "void" >> $cfile
      echo "$1_set_$name($1 * ad, char * name) {" >> $cfile
      echo " "  >> $cfile
      echo "}"   >> $cfile
      echo \ >> $cfile

   done

   echo \ >> $cfile

}




# Function for XML output.
function xmlfunc
{
   echo \ >> $cfile 

   echo "void
$1_print_xml($1 * ad, PrintFunc printer, void * stream) {" >> $cfile
   
   for name in "$@" 
   do
      echo "   printer(stream,\"<$name>%s</$name>\n\",$1_get_$name(ad));" >> $cfile
   done

   echo "}" >> $cfile

}


# Function for memory management.
function memfunc
{
   echo "/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */"  >> $cfile
echo "$1 *
$1_new(void) {

  $1 * ad;
  ad = ($1 *) malloc (sizeof ($1));
  memset (ad, 0xda, sizeof ($1));

  jxta_advertisement_initialize((Jxta_advertisement*)ad,
                                \"$1\",
                                $1_tags,
                                $1_print_xml,
                                $1_delete);

 /* Fill in the required initialization code here. */

  return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
void
$1_delete ($1 * ad) {
 /* Fill in the required freeing functions here. */ 

  memset (ad, 0xdd, sizeof ($1));
  free (ad);
}" >> $cfile

}



## This needs to point to jxta_advertisement_set_handlers
## somehow, so that the jxta_advertisement api stays private.
function sethandlersfunc
{

echo \ >> $cfile

echo "void" >> $cfile
echo "$1_set_handlers($1 * ad, XML_Parser parser, void * parent) {

  if (parent == NULL) {
    ad->generic.parent = (void*)ad;
  }  else {
    ad->generic.parent = parent;
  }

  ad->generic.parser = parser;
  
  XML_SetCharacterDataHandler (ad->generic.parser, ad->generic.char_data);
  XML_SetUserData (ad->generic.parser, ad);
  XML_SetElementHandler (ad->generic.parser, ad->generic.start, ad->generic.end);

}" >> $cfile

}



function parsefunc
{
echo "
void 
$1_parse_charbuffer($1 * ad, const char * buf, int len) {

  jxta_advertisement_parse_charbuffer((Jxta_advertisement*)ad,buf,len);
}


  
void 
$1_parse_file($1 * ad, FILE * stream) {

   jxta_advertisement_parse_file((Jxta_advertisement*)ad, stream);
}" >> $cfile

}



# Function for producing an optional main method.
function mainmethodfunc
{

echo "
#ifdef STANDALONE
int
main (int argc, char **argv) {
   $1 * ad;
   FILE *testfile;

   if(argc != 2)
     {
       printf(\"usage: ad <filename>\n\");
       return -1;
     }

   ad = $1_new();

   testfile = fopen (argv[1], \"r\");
   $1_parse_file(ad, testfile);
   fclose(testfile);

   //$1_print_xml(ad,fprintf,stdout);
   $1_delete(ad);

   return 0;
}
#endif" >> $cfile

echo "#ifdef __cplusplus"     >> $cfile
echo "}"                      >> $cfile
echo "#endif"                 >> $cfile


}


function headerfunc
{

copyrightfunc $hfile


echo "#ifndef __$1_H__
#define __$1_H__

#include \"jxta.h\"
#include \"jxta_advertisement.h\""  >> $hfile

echo "#ifdef __cplusplus"      >> $hfile
echo "extern \"C\" {"          >> $hfile
echo "#endif"                  >> $hfile

echo "
typedef struct _$1 $1;

$1 * $1_new();
void $1_set_handlers($1 *, XML_Parser, void *);
void $1_delete($1 *);
void $1_print_xml($1 *, PrintFunc, void * stream);
void $1_parse_charbuffer($1 *, const char *, int len); 
void $1_parse_file($1 *, FILE * stream);" >> $hfile



   echo \ >> $hfile

   for name in "$@" 
   do
      echo "char * $1_get_$name($1 *);" >> $hfile
      echo "void $1_set_$name($1 *, char *);" >> $hfile
      echo \ >> $hfile

   done

   echo \ >> $hfile


echo "#endif /* __$1_H__  */" >> $hfile


echo "#ifdef __cplusplus"     >> $hfile
echo "}"                      >> $hfile
echo "#endif"                 >> $hfile


}


copyrightfunc $cfile
compilefunc $1
includefunc $1
enumfunc  $@
structfunc $@
handlerfunc $@
getsetfunc $@
dispatchfunc $@
xmlfunc $@
memfunc $@
#sethandlersfunc $@
parsefunc $1
mainmethodfunc $1
headerfunc $@



