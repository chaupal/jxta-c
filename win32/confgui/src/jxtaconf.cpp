
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>

#include <windows.h>
#include <direct.h>

#include "jxtaconf.h"
#include "jxta_pa.h"
#include "error.h"



char pwd[512];
//typedef void  (*Print_Func)(void *, const char *, const char *);
//void error_register_handler(Error * e, void * stream, Print_Func callback, const char * title);



Print_Func
error_handler(void * stream, const char * title, const char * message) {

   MessageBox((HWND)stream,message,title,MB_OK);
   return 0;
}


static Error * e;

Jxtaconf::Jxtaconf() {

   struct _stat buf;
   int stat_result;
   int filesize;

   FILE * fp = NULL;
   //FILE * tmpfp = NULL;
   char * peername;
   //char * new_conf;

   jxta_initialize();

   e = error_new();
   pa = jxta_PA_new();

   error_register_handler(e,NULL,(Print_Func)error_handler,"Woops");
   //error_display(e,_getcwd(pwd,512));

   stat_result = _stat("PlatformConfig",&buf );


   if (stat_result != 0) {
     error_display(e, "Stat failed on Platform config");
     exit(0);
   }

   filesize = buf.st_size;

   original_conf = (char*)malloc(sizeof(filesize));

   fp = fopen("PlatformConfig","r");
   if (fp == NULL) {
     error_display(e, "Null file pointer");
     exit(0);
   }
   jxta_PA_parse_file(pa,fp);
   fclose(fp);

   fp = fopen("PlatformConfig","rb");
   fread(original_conf,filesize,1,fp);
   fclose(fp);

   //just for testing
   write_original_data();

   peername = jxta_advertisement_get_string((Jxta_advertisement*)pa,"Name");
   error_display(e,peername);



   peername[0] = '\0';
   hostname[0] = '\0';
}



bool
Jxtaconf::validate() {

   return false;
}




void 
Jxtaconf::write_jxta_conf() {

   FILE * fp;
   JString * js;
JString * name;
JString * dbg;

error_display(e,"In write_jxta_conf");

   //validate();
   fp = fopen("jxtaconf.ini","w");
   //fprintf(fp,"PEERNAME=%s\n",get_peer_name());
   //fprintf(fp,"HOSTNAME=%s\n",get_host_name());

name = jstring_new_2(get_peer_name());

   jxta_PA_set_Name(pa,name);

   dbg = jstring_new_2("foobar");
   jxta_PA_set_Dbg(pa,dbg);

   jxta_PA_get_xml(pa,&js);
   fprintf(fp,"%s",jstring_get_string(js));

   fclose(fp);
}



/**
 * @todo Pass in the file name to write the data,
 * or make the file name symbolic.
 */
void
Jxtaconf::write_original_data() {
   FILE * fp = NULL;
   fp = fopen("foo","wb");
   fwrite(original_conf,strlen(original_conf),1,fp);
   fclose(fp);
}



void
Jxtaconf::set_peer_name(const char * pname) {

strcpy(peername, pname);
   //error_display(e,pname);
}


const char *
Jxtaconf::get_peer_name() {

   return peername;
}


void
Jxtaconf::set_host_name(const char * hname) {

   strcpy(hostname, hname);


}

const char *
Jxtaconf::get_host_name() {

   return hostname;
}
