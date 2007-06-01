#ifndef WIN32
#include <unistd.h>
#define WRITE write
#define READ read
#define OPEN open
#else
#include <stdlib.h>
#include <io.h>
#define WRITE _write
#define READ _read
#define OPEN _open
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <jxta.h>

typedef struct filebytes {
   int numbytes;
   char * bytes;
   char * filename;
}Filebytes;

Filebytes * 
read_bytes_from_file(char * filename)
{
   struct stat filestat;
   Filebytes * fb;
   int fp; //FILE *fp;
   stat(filename, &filestat);  
   fb = (Filebytes*)malloc(sizeof(Filebytes));
   fb->numbytes = filestat.st_size;
   fb->bytes = (char*)calloc(1,fb->numbytes);
   fb->filename = filename;

   if( ((int) fp = OPEN(fb->filename,_O_RDONLY)) == -1)
     return (Filebytes*)-1;
   READ((int)fp,(void*)fb->bytes,filestat.st_size);
   _close((int)fp);
   return fb;
}

int
write_bytes_to_file(Filebytes * fb)
{
   FILE * ofp;
   ofp = (FILE*)OPEN("foo", _O_CREAT | O_WRONLY);
   WRITE((int)ofp,fb->bytes,fb->numbytes);
   _close((int)ofp);
   return 0;
}

void
filebytes_free(Filebytes * fb)
{   
   free(fb->bytes);
   free(fb);
}

#ifdef STANDALONE
int 
main(int argc, char ** argv)
{
   Filebytes * fb;
   char * filename = argv[1];

   fb = read_bytes_from_file(filename);
   write_bytes_to_file(fb);
   filebytes_free(fb);

   return 0;
}
#endif
