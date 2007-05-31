
/**
 * simpleclient takes a port to connect to.
 */


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include "tcpheader.h"
#define SOCKET int
#define SOCKET_ERROR -1




/* 9701 is the jxta tcp port */
#define PORT 9701


#define SERVERHOST "209.25.154.233"
#define SOURCEHOST "10.10.10.28"


#define MAXMSG 512


void
init_sockaddr (struct sockaddr_in *name, const char *hostname, short port) {
  struct hostent *hostinfo;

  name->sin_family = AF_INET;
  name->sin_port = htons (port);
  hostinfo = gethostbyname (hostname);
  if (hostinfo == NULL)
    {
      fprintf (stderr, "Unknown host %s.\n", hostname);
      exit (EXIT_FAILURE);
    }
  name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}






void
write_to_server (SOCKET filedes)
{
  int nbytes;
  char buffer[MAXMSG] = {0};
  char tcpheader[20] = {0};


  /*
  uint hostaddr, * hostaddr_p;
  uint16_t port, * port_p;


  tcpheader[0] = (char)1;

  hostaddr = inet_addr ("10.10.10.28");
  hostaddr_p = &hostaddr;

  port = htons(9701);
  port_p = &port;

  memcpy(&tcpheader[1],hostaddr_p, 4);
  memcpy(&tcpheader[5],port_p,2);

  tcpheader[11] = (char)1;

  tcp_print_header(tcpheader);
  */


  tcp_build_network_header(tcpheader,SOURCEHOST,9701,0);


 /* We can use a write here on the unix side, but win32  
  * complains about bad file descriptors.  So use send
  * with flags set to zero.  This is the same as write
  * for unix, and it makes win32 happy.
  */
  nbytes = send (filedes, tcpheader, 20,0);
  nbytes = recv (filedes, buffer, MAXMSG,0);

  if (buffer[0] == 1) {
    printf("Received keepalive from peer\n");
  } else {
    printf("Did not receive keepalive from peer\n");
  }

}





int
main (int argc, char ** argv)
{

  SOCKET sock;
  struct sockaddr_in servername;

  if (argc < 2) {
    printf("Usage: simpleclient  <port>\n");
    return 0;
  }


  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket (client)");
      exit (EXIT_FAILURE);
    }

  /* Connect to the server. */
  init_sockaddr (&servername, SERVERHOST, PORT);

  if (SOCKET_ERROR ==  connect (sock,(struct sockaddr *) &servername, sizeof (servername)))
    {
      perror ("connect (client)");
      exit (EXIT_FAILURE);
    }

  /* Send data to the server. */
  write_to_server (sock);
  close (sock);
  exit (EXIT_SUCCESS);
}

