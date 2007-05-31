
/* Since all the socket stuff was removed in light of 
 * new design, this just has stubs for now.  It should 
 * be pretty easy to wire up though.  
 *
 * License: Either GPL or JXTA, depending on how readline is 
 * linked.  Contrary to the initial cvs commit log message,
 * this is GPL with the current static link to libreadline.
 * This was cleared with Juan last year.  However, moving to 
 * a dynamic link should allow it to be JXTA licensed.  In any 
 * case, it should not be too hard to resolve the issue.
 */



/* compile problems on mandrake: */
/*
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `tgetnum'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `tgoto'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `tgetflag'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `BC'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `tputs'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `PC'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `tgetent'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `UP'
 
/usr//bin/../lib/gcc-lib/i586-mandrake-linux-gnu/2.96/../../../libreadline.
so: undefined reference to `tgetstr'
*/

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>


static char *prompt = "JXTA> ";


/* A static variable for holding the line.
 */
static char *line_read = (char *) NULL;


/* Read a string, and return a pointer to it.
 * Returns NULL on EOF. 
 */
char *
rl_gets () {

   if (line_read) {
	free (line_read);
	line_read = (char *) NULL;
   }

   line_read = readline (prompt);

   if (line_read && *line_read) {
      add_history (line_read);
   }

   return (line_read);
}


/* Leftover from original work.  Could be useful,
 * delete later if not.
 */
void 
getenv_func (char *args) {

   printf ("Executed getenv, args: %s\n", args);
   printf ("%s\n", getenv ("PATH"));
}




struct _command_tab {

   char *command;
   void *(*command_func) (char *);
};



void
rdvstatus_func() {
  printf("rdvstatus: \n");
}


void
peers_func() {
  printf("peers: \n");
}


void
groups_func() {
  printf("groups: \n");
}


void
join_func() {
  printf("join: \n");
}


void
talk_func() {
  printf("talk: \n");
}


struct _command_tab com_tab[] = {
   {"rdvstatus", *rdvstatus_func},
   {"peers", *peers_func},
   {"groups", *groups_func},
   {"join", *join_func},
   {"talk",*talk_func},
   {NULL, NULL}
};




/* This function will strip out the first token,
 * then match it against commands in the dispatch 
 * table.
 */
static int
dispatch (char *line) {

   int i = 0;
   char *command = NULL;
   char *args = NULL;

   if (strlen(line) == 0) {
      return 0;
   }
   
   command = strtok (line, " ");
   args = strtok (NULL, "\n");

   while (com_tab[i].command != NULL) {
      if (!strcmp (command, com_tab[i].command)) {
         com_tab[i].command_func (args);
	 return 1;
      }
      i++;
   }

   printf ("%s: command not found\n", command);
   return 0;
}



extern char **environ;

int
main () {

   char *line;
   int retval;

   while (1) {
       line = rl_gets ();
       if (line)
        retval = dispatch (line);
   }

   return 0;
}
