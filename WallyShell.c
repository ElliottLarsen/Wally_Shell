// Author: Elliott Larsen
// Date: 
// Description:

/* This shell will:
 * Print an interactive inpput prompt.
 * Parse command line input into semantic tokens.
 * Implement parameter expansion ($$, $?, $!, and ~).
 * Implement two shell built-in commands (exit and cd).
 * Execute non-built-in commands using the appropriate exec() function (<, >, &, etc.)
 * Implement custom behavior for SIGINT and SIGTSTP signals */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

void SIGINT_handler(int signo){
  char* message = "\nCaught SIGINT\n";
  write(STDOUT_FILENO, message, 16);
  raise(SIGUSR2);
}

int main() {
  struct sigaction SIGINT_action = {0}, ignore_action = {0};
  // Fill out the SIGINT_action struct.
  SIGINT_action.sa_handler = SIGINT_handler;
  // Check the following two lines.
  // Block all catchable signals while SIGINT_handler is running.
  sigfillset(&SIGINT_action.sa_mask);
  // No flags set.
  SIGINT_action.sa_flags = 0;
  sigaction(SIGINT, &SIGINT_action, NULL);

  // SIGTSTP is ignored and its disposition is set to SIG_IGN.
  signal(SIGTSTP, SIG_IGN);
  /*
  // Fill out the ignore_action struct, which will ignore SIGTSTP. 
  ignore_action.sa_handler = SIG_IGN;
  sigaction(SIGTSTP, &ignore_action, NULL);
  */
  while(1)
    pause();
  return 0;
}

// Input


// Input Splitting


// Expansion


// Parsing


// Execution


// Waiting
