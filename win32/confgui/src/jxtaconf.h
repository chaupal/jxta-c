

#ifndef __JXTACONF_H__
#define __JXTACONF_H__

#include <stdio.h>
#include <jxta.h>
#include <apr_general.h>



class Jxtaconf {

public:

   Jxtaconf();

   void set_peer_name(const char * pname);
   void set_host_name(const char * hname);
   void write_original_data();
   void write_jxta_conf();
   void read_jxta_conf();
   bool validate();
   const char * get_peer_name();
   const char * get_host_name();

private:
   Jxta_PA * pa;
   char * original_conf;
   char peername[80];
   char hostname[80];
};



#endif /* __JXTACONF_H__ */

