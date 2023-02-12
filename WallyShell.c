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

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>

// Handles SIGINT signal.
void SIGINT_handler(int signo);
// Splits input line.
char* split_input(char* input_line);
// String search and replace.
char* str_gsub(char *restrict *restrict input_line, char const *restrict old_word, char const *restrict new_word); 


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
  // SIGTSTP normally casuses a process to halt, which is undesirable.
  // This shell does not respond to this signal and it sets its disposition to SIG_IGN.
  signal(SIGTSTP, SIG_IGN);
  /*
  // Fill out the ignore_action struct, which will ignore SIGTSTP. 
  ignore_action.sa_handler = SIG_IGN;
  sigaction(SIGTSTP, &ignore_action, NULL);
  */

  char* input_line = NULL;
  size_t n = 0;
  for (;;) {
    // Printing a input prompt.
    fprintf(stderr, "%s", getenv("PS1"));
    // Grab user input.
    ssize_t line_length = getline(&input_line, &n, stdin);
    if (line_length != -1) {
      // Read was successful.
      printf("Read was successful\n");
      printf("Here is what was read: %s\n", input_line);
      //printf("After splitting:\n");     
      //char* token = split_input(input_line);
      //split_input(input_line);
      

      printf("After replaceing $$ with !!\n");
      char *result = str_gsub(&input_line, "$$", "!!");
      if (!result) {
        exit(1);
      }
      input_line = result;
      printf("%s", input_line);
      free(input_line);
      return 0;



    } else {
      // Read was successful.
      fprintf(stderr, "Read was unsuccessful. getline() returned %li and errno is %s\n", line_length, strerror(errno));
    }
  }
}


void SIGINT_handler(int signo) {
  char* message = "\nCaught SIGINT\n";
  write(STDOUT_FILENO, message, 16);
  raise(SIGUSR2);
}


char* split_input(char* input_line) {
  char* tokens = strtok(input_line, " \t\n");
  while (tokens != NULL) {
    printf("%s\n", tokens);
    tokens = strtok(NULL, " \t\n");
  }
  return tokens;
}


char *str_gsub(char *restrict *restrict input_line, char const *restrict old_word, char const *restrict new_word) {
  char *str = *input_line;
  size_t input_line_len = strlen(str);
  size_t const old_word_len = strlen(old_word), new_word_len = strlen(new_word);

  for (; (str = strstr(str, old_word)); ) {
    ptrdiff_t off = str - *input_line;
    if (new_word_len > old_word_len) {
      str = realloc(*input_line, sizeof **input_line * (input_line_len + new_word_len - old_word_len + 1));
      if (!str) {
        return str;
      }
      *input_line = str;
      str = *input_line + off;
    }
    memmove(str + new_word_len , str + old_word_len, input_line_len + 1 - off - old_word_len);
    memcpy(str, new_word, new_word_len);
    input_line_len = input_line_len + new_word_len - old_word_len;
    str += new_word_len;
  }
  str = *input_line;
  if(new_word_len < old_word_len) {
    str = realloc(*input_line, sizeof **input_line * (input_line_len + 1));
    if (!str) {
      return str;
    }
    *input_line = str;
  }

  return str;
}

/* INPUT
 *
 *
 * Managing Background Process
 *
 * Before printing a prompt message, the shell will check for any un-waited-for background 
 * processes in the same process group ID as the shell itself.  And it will print the following
 * messages to stderr:
 *
 * If exited: "Child process %d done. Exit status %d.\n", <pid>, <exit status>
 * If signaled: "Child process %d done. Signaled %d.\n", <pid>, <signal number>
 *
 * If a child process is stopped, then the shell will send it the SIGCONT signal and print the
 * following message to stderr:
 *
 * "Child process %d stopped.  Continuing.\n", <pid>"
 *
 * Any other child state changes (WIFCONTINUED) will be ignored.
 *
 *
 *
 * The prompt
 *
 * The shell will print a prompt to stderr by expanding the PS1 parameter ($ for users and # for root user).
 *
 *
 *
 * Reading a line of input
 *
 * After printing the command prompt, the shell will read a line of input from stdin (getline()).
 * If reading is interrupted by a signal, a newline will be printed, then a new command prompt will be printed,
 * including checking for background processes.  And reading a line of input will resume.  (clearerr() and reset errno).
 */


/* INPUT SPLITTING
 *
 *
 * The input line will be split into words, delimited by the " \t\n".  A minimum of 512 words will be supported.
 */


/* EXPANSION
 *
 *
 * Any occurrence of "~/" at the beginning of a word will be replaecd with the value of the HOME environment variable.
 * The "/" will be retained (getenv()).
 *  
 * Any occurrence of "$$" within a word will be replaced with the process ID of the shell process (getpid()).
 *
 * Any occurrence of "$?" within a word will be replaced with the exit status of the last foreground command.
 *
 * Any occurrence of "$!" within a word will be replaced with the process ID of the most recent background process.
 */


/* PARSING
 *
 *
 * The first occurrence of the character "#" and any additional characters following it will be ignored as a comment.
 *
 * If the last character is "&", it will indicate that the command is to be run in the background.
 *
 * If the last word is immediately preceded by the character "<", it will be interpreted as the filename operand of the input
 * redirection operator.
 *
 * If the last word is immediately preceded by the character ">", it will be interpreted as the filename operand of the output
 * redirection operator.
 */


/* EXECUTION
 *
 *
 * If at this point no command word is present, then the shell will silently return to INPUT and print a new prompt message.  
 * This is not an error and "$?" will not be modified.
 *
 *
 *
 * Built-in commands
 *
 * If the command to be executed is "exit" or "cd", the following built-in procedures will be executed.  The redirection and background
 * operators mentioned in PARSING will be ignored by built-in commands.
 *
 * exit: The "exit" built-in takes one argument.  If not provided, the argument is implied to be the expansion of "$?", the exit status
 *       of the last foreground command.  It will be an error if more than one artument is provided or if an argument provided is not an integer.
 *       This shell will print "\nexit\n" to stderr then exit with the specified (or implied) value.  All child processes in the same process group 
 *       will be sent a SIGINT signal before exiting (kill()).  The shell does not need to wait on these child processes and may exit immediately (exit()).
 *
 * cd: The "cd" built-in takes one argument.  If not provided, the argument is implied to be the expansion of "~/", the value of the HOME enrionment variable.
 *     It will be an error if more than one argument is provided.  The shell will change its own current working directory to the specified or implied path. 
 *     It will be an error if the operation fails (chdir()).
 *
 *
 *
 * Non built-in commands
 *
 * Otherwise, the command and its arguments will be executed in a new child process.  If the command name does not include a "/", the command will be searched
 * for in the system's PATH environment variable (execvp()).  If a call to fork() fails, it will be an error.  In the child process(es):
 *      - All signals will be rest to their original dispositions when the shell was first invoked.  This is not the same is SIG_DFL.  oldact() in SIGACTION.
 *      - If a filename is specified as the operand to the input ("<") redirection operator, the specified file will be opened for reading on stdin.  It will 
 *        be an error if the file cannot be opened of readhing of does not already exist.
 *      - If file name is specified as the operand to the output(">") redirection operator, the specified file will be opened for writing on stdout.  If the file
 *        does not exist, it will be created with permissions 0777.  It will be an error if the file cannot be opened or created for writing.
 *      - If the child process fails to exec (such as if the specified command cannot be found), it will be an error.
 *      - When an error occurs in the child, the child will immediately print an error mesage to stderr and exit with a non-zero exit status.
 */


/* WAITING
 *
 *
 * 
 *
 */
