/*===========================================================================

  Test client for Kevin's tiny C library
  main.c
  Copyright (c)2020 Kevin Boone. Distributed uner the terms of the 
    GNU PUblic Licence, v3.0

  This program does simple I/O, basic process management, environment
    management, etc., all without the benefit of a C standard library.

===========================================================================*/
// cnolib.h provides prototypes and definitions for all the C-library-like
//   functions that program requires. Don't #include stdio.h or anything
//   of that nature.
#include "cnolib.h"

/* Write a number without newline */
void putn (int n)
  {
  char s[20];
  itoa (n, s, 10);
  fputs (s, stdout);
  fflush (stdout);
  }


/* Change directory -- built-in command */
void do_chdir (const char *dir)
  {
  if (chdir (dir) != 0)
    {
    perror ("Can't change directory");
    }
  }

/* Process an internal command. If the command line does not match an
    internal command, return FALSE. */
BOOL do_internal_cmd (int argc, char **argv, BOOL *exit)
  {
  if (strcmp (argv[0], "exit") == 0) 
    {
    *exit = TRUE;
    return TRUE;
    } 

  if (strcmp (argv[0], "cd") == 0) 
    {
    if (argc == 1)
      {
      do_chdir (getenv ("HOME"));
      return TRUE;
      }
    else
      {
      do_chdir (argv[1]);
      return TRUE;
      }
    } 

  if (strcmp (argv[0], "echo") == 0) 
    {
    for (int i = 1; i < argc; i++)
      {
      fputs (argv[i], stdout);
      }
    fputs ("\n", stdout);
    fflush (stdout);
    return TRUE;
    } 

  return FALSE;
  }

/* Process the command line. Return TRUE is the shell should continue. */
BOOL do_command (const char *cmdline)
  {
  // First parse the cmdline to count the tokens
  char *s = strdup (cmdline);

  int myargc = 0;
  char *tok = strtok (s, " \t");
  while (tok)
    {
    myargc++;
    tok = strtok (NULL, " \t");
    };

  free (s);

  char **myargv = malloc ((myargc + 1) * sizeof (char *)); 

  // Parse the cmdline again to allocate memory for the tokens 
  
  s = strdup (cmdline);
  myargc = 0;
  tok = strtok (s, " \t");
  while (tok)
    {
    myargv[myargc] = strdup (tok);
    myargc++;
    tok = strtok (NULL, " \t");
    };
  myargv[myargc] = NULL;
  free (s);

  BOOL doexit = FALSE; // With be set by the "exit" command

  if (do_internal_cmd (myargc, myargv, &doexit) == FALSE)
    {
    int pid = fork();
    if (pid == 0)
      {
      if (execvp (myargv[0], myargv) == -1)
        {
	perror ("Can't execute");
	exit (errno);
        }
      }
    else if (pid == -1)
      {
      perror ("Can't fork"); 
      }
    else
      {
      int status = 0;
      waitpid (pid, &status, 0); 
      }
    }

  // Free memory
  for (int i = 0; i < myargc; i++)
    free (myargv[i]);
  free (myargv);

  return !doexit; 
  }

// Execute file line by line
void do_file (const char *filename)
  {
  FILE *fin = fopen (filename, "r");
  if (fin)
    {
    BOOL done = FALSE;
    char line[512];
    while (fgets (line, sizeof (line), fin) && !done)
      {
      int l = strlen (line);
      if (l > 1)
        {
        if (line[l-1] == '\n')
          line[l-1] = 0;
        if (line[0])
          done = (do_command (line) == FALSE); 
        }
      }
    fclose (fin);
    }
  else
    {
    fputs (filename, stderr);
    fputs (": ", stderr);
    perror ("Can't open file for reading");
    }
  }

/* main -- start here */
int main (int argc, char **argv)
  {
  BOOL done = FALSE;

  if (argc > 1)
    {
    for (int i = 1; i < argc; i++)
      do_file (argv[i]);
    done = TRUE;
    }

  if (!done) do
    {
    char line[512];
    fputs ("$ ", stdout);
    fflush (stdout);
    if (fgets (line, sizeof (line), stdin))
      {
      int l = strlen (line);
      if (l > 1)
        {
        if (line[l-1] == '\n')
          line[l-1] = 0;
        if (line[0])
          done = (do_command (line) == FALSE); 
        else
          putchar ('\n');
        }
      }
    else
      done = TRUE;
    } while (!done);

  exit (0);
  }

