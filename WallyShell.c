// Author: Elliott Larsen
// Date:
// Description: Second pass with no modular approach.

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
#include <ctype.h>

char *str_gsub(char *restrict *restrict input_line, char const *restrict old_word, char const *restrict new_word, int *is_home_dir_expanded) {
  char *str = *input_line;
  size_t input_line_len = strlen(str);
  size_t const old_word_len = strlen(old_word), new_word_len = strlen(new_word);

  for (; (str = strstr(str, old_word)); ) {
    ptrdiff_t off = str - *input_line;
    if (new_word_len > old_word_len) {
      str = realloc(*input_line, sizeof **input_line * (input_line_len + new_word_len - old_word_len + 1));
      if (!str) goto exit;
      *input_line = str;
      str = *input_line + off;
    }
    memmove(str + new_word_len , str + old_word_len, input_line_len + 1 - off - old_word_len);
    memcpy(str, new_word, new_word_len);
    input_line_len = input_line_len + new_word_len - old_word_len;
    str += new_word_len;

    if (is_home_dir_expanded) goto exit;
  }

  str = *input_line;
  if(new_word_len < old_word_len) {
    str = realloc(*input_line, sizeof **input_line * (input_line_len + 1));
    if (!str) goto exit;
    *input_line = str;
  }
  
  exit:
    return str;
}

void SIGINT_handler(int signo) {
  char *message = "\nCaught SIGINT\n";
  write(STDOUT_FILENO, message, 16);
  raise(SIGUSR2);
}

int main(void) {
  char *user_input = NULL;
  int args_num = 0;
  size_t n = 0;
  char *IFS;
  char *home_dir;
  char pid[1024];
  sprintf(pid, "%d", getpid());
  pid_t child_pid;
  // "$$" defaults to "0"
  char *fg_status = "0";
  // ""$!" defaults to "" if no background process ID is available.
  char *bg_status = "";
  

  char *out_file_name = NULL;
  char *in_file_name = NULL;
  char *args[1024 + 5] = {NULL};
  char *tokens = NULL;

  int is_home_dir_expanded = 0;
  int should_run_in_bg = 0;


  struct sigaction SIGINT_action = {0}, ignore_action = {0};
  // Fill up SIGINT_action struct.
  SIGINT_action.sa_handler = SIGINT_handler;
  // Block all catchable signals while SIGINT_handler is running.
  sigfillset(&SIGINT_action.sa_mask);
  // No flags set.
  SIGINT_action.sa_flags = 0;
  sigaction(SIGINT, &SIGINT_action, NULL);
  // SIGTSTP normally causes a process to halt, which is undesireable.  This shell does not respond to this signal and it sets its disposition to SIG_IGN.
  signal(SIGTSTP, SIG_IGN);

  for (;;) {
    // Reset variables.
    tokens = NULL;
    args_num = 0;

    should_run_in_bg = 0;

    // Managing Background Processes
    
    
    // Prompt
    char *prompt;
    if (!(prompt = getenv("PS1"))) {
      prompt = "\0";
    }
    fprintf(stderr, "%s", prompt);
    fflush(stderr);

    ssize_t line_length = getline(&user_input, &n, stdin);

    // Error from reading an input.
    if (line_length == -1) {
      clearerr(stderr);
      clearerr(stdin);
      errno = 0;
      continue;
    }

    // Word Splitting.
    IFS = getenv("IFS");
    if (IFS == NULL) {
      IFS = " \t\n";
    }

    tokens = strtok(user_input, IFS);
    while (tokens != NULL) {
      if (strdup(tokens) != NULL) {
        args[args_num] = strdup(tokens);
        args_num += 1;
      } else {
        // free args?
        fprintf(stderr, "\nString split error\n");
        exit(1);
      }
      tokens = strtok(NULL, IFS);
    }


    // If no input is given.
    if (args_num == 0 || (strcmp(args[0], "#") == 0) || (strncmp(args[0], "#", 1) == 0)) {
      continue;
    }



    // Expansion
    // If args[0] is not a command or a comment? Should I worry about it in the Execusion stage?

    int i = 0;
    while (i < args_num && args[i] != NULL) {
      // "~/" expansion with the HOME environment variable.
      is_home_dir_expanded = 0;
      if (strncmp(args[i], "~/", 2) == 0) {
        home_dir = getenv("HOME");
        if (home_dir) {
          is_home_dir_expanded = 1;
          str_gsub(&args[i], "~", home_dir, &is_home_dir_expanded);
        } else {
          // What to do if there is no getenv("HOME")?
        }
      }
      // "$$" expansion with the shell's PID.
      str_gsub(&args[i], "$$", pid, &is_home_dir_expanded);
      // "$?" expansion.
      if (strcmp(fg_status, "0") != 0) {
        str_gsub(&args[i], "$?", fg_status, &is_home_dir_expanded);
      } else {
        str_gsub(&args[i], "$?", "0", &is_home_dir_expanded);
      }

      // "$!" expansion.
      if (strcmp(bg_status, "") != 0) {
        str_gsub(&args[i], "$!", bg_status, &is_home_dir_expanded);
      } else {
        str_gsub(&args[i], "$!", "", &is_home_dir_expanded);
      }
      i++;
    }



    // Parsing
    i = 0;
    // Find the index number to either # or end of the input.
    while (i < args_num) {
      if ((strcmp(args[i], "#") == 0) || i == args_num -1) {
        break;
      }
      i++;
    }

    // cat myfile > output.txt & #
    if (i >= 3 && i == args_num - 1 && strcmp(args[i], "#") == 0 && strcmp(args[i - 1], "&") == 0) {
      args[i] = NULL;
      should_run_in_bg = 1;
      if(strcmp(args[i - 3], ">") == 0) {
        out_file_name = args[i - 2];
        args[i - 3] = NULL;
      } else if (strcmp(args[i - 3], "<") == 0) {
        in_file_name = args[i - 2];
        args[i - 3] = NULL;
      }
    }
    // cat myfile > output.txt #
    else if (i >= 3 && i == args_num - 1 && strcmp(args[i], "#") == 0) {
      args[i] = NULL;
      if(strcmp(args[i - 2], ">") == 0) {
        out_file_name = args[i - 1];
        args[i - 2] = NULL;
      } else if (strcmp(args[i - 2], "<") == 0) {
        in_file_name = args[i - 1];
        args[i - 2] = NULL;
      }
    }
    // cat myfilie > output.txt &
    else if (i >= 3 && strcmp(args[i], "&") == 0) {
      args[i] = NULL;
      should_run_in_bg = 1;
      if (strcmp(args[i - 2], ">") == 0) {
        out_file_name = args[i - 1];
        args[i - 2] = NULL;
      } else if (strcmp(args[i - 2], "<") == 0) {
        in_file_name = args[i - 1];
        args[i - 2] = NULL;
      }
    }
    // cat myfile > output.txt
    else if (i >= 3) {
      if (strcmp(args[i - 1], ">") == 0) {
        out_file_name = args[i];
        args[i - 1] = NULL;
      } else if (strcmp(args[i - 1], "<") == 0) {
        in_file_name = args[i];
        args[i - 1] = NULL;
      }
    }
   
    else if (i >= 2 && strcmp(args[i], "#") == 0) {
      args[i] = NULL;
    }

    else if (i >= 2 && strcmp(args[i], "&") == 0) {
      should_run_in_bg = 1;
      args[i] = NULL;
    }

    else if (i >= 1 && strcmp(args[i], "#") == 0) {
      args[i] = NULL;
    }

    else if (i >= 1 && strcmp(args[i], "&") == 0) {
      should_run_in_bg = 1;
      args[i] = NULL;
    }
    
    // Execution - Do I account for the comments here?
    // Executing "exit".
    if (strcmp(args[0], "exit") == 0) {
      if (args_num > 2) {
        // If too many arguments are given.
        fprintf(stderr, "'exit' command takes only one argument.\n");
        exit(1);
      }

      if (args_num == 2) {
        if (isdigit(*args[1]) == 0) {
          // Correct number of argument is given but it is not an integer.
          fprintf(stderr, "'exit' command only takes an integer argument.\n");
          exit(1);
        } else {
          // Correct number of argument is given and it IS an integer.
          //printf("Print to stderr, sent SIGINT to all child processes, and exit immediately with the given integer value.\n");
          exit(0);
        }
      } else {
        // No argument is given so it exits.
        //printf("Expand $? and exit.\n");
        //printf("%d", atoi(fg_status));
        fprintf(stderr, "\nexit\n");
        exit(atoi(fg_status));
      }

    }

    
    // Executing "cd".
    else if (strcmp(args[0], "cd") == 0) {
      int cd_result;
      if (args_num > 2) {
        fprintf(stderr, "Incorrect number of arguments.\n");
        continue;
      }

      else if (args_num == 2) {
        cd_result = chdir(args[1]);
        if (cd_result != 0) {
          fprintf(stderr, "Cannot change directory.\n");
        }
      }

      else {
        home_dir = getenv("HOME");
        if (home_dir) {
          cd_result = chdir(home_dir);
          if (cd_result != 0) {
            fprintf(stderr, "Cannot change directory to home.\n");
          }
        } else {
          fprintf(stderr, "getenv('HOME') failed.");
        }
      }
    }
    /*
    else if (strcmp(args[0], "cd") == 0) {
      if (args_num > 2) {
        // If cd is given more than one argument.
        fprintf(stderr, "Incorrect number of arguments\n");
        continue;
      } 

      else if (args_num == 2) {
        // If cd is given exactly one artument.
        // Check for the error.
        if (chdir(args[1]) != 0) {
          // If chdir cannot be performed with the argument given, restart the main loop.
          fprintf(stderr, "Cannot change directory\n");
          continue;
        } else {
          // If chdir with the given argument is successful.
          //fprintf(stdout, "successful change of directory\n");
        }
      } 

      else {
        // If there is no argument given to cd.
        home_dir = getenv("HOME");
        if (home_dir) {
          if (chdir(home_dir) != 0) {
            fprintf(stderr, "Cannot change directory\n");
            continue;
          } else {
            //fprintf(stdout, "successful change of directory\n");
          }
        } else {
          // What to do if "HOME" environment variable doesn't exist?
          fprintf(stderr, "Cannot go to HOME\n");
          continue;
        }
      }
    } 
    */
    // Executing built-in commands.
    else {

      int child_status;
      child_pid = fork();
      //printf("\nHere is child_pid from fork(): %d\n", child_pid);
      
      switch(child_pid) {
        case -1:
          fprintf(stderr, "fork() error. Handle this.\n");
          exit(1);
          break;

        case 0:
          //printf("Message from the child process.\n");
          execvp(args[0], args);

          fprintf(stderr, "execvp() error. Handel this.\n");
          exit(1);
          break;

        default:
          //printf("Message from the shell.\n");
          //printf("Here is child_pid from default: %d\n", child_pid);
          if (!should_run_in_bg) {
            //printf("\nPerform a blocking wait on the foreground child process.\n");
            child_pid = waitpid(child_pid, &child_status, 1);
            fg_status = calloc(1024, sizeof(char));
            //printf("Here is child_pid after waitpid: %d", child_pid);
            sprintf(fg_status, "%d", child_pid);
            //printf("Here is fg_status: %s", fg_status);
          } else {
            bg_status = calloc(1024, sizeof(char));
            sprintf(bg_status, "%d", child_pid);
          }
      }
      continue;
      //printf("BUILT-IN");
    }


    // Free realloc() from str_gsub();
    for (int i = 0; i < args_num; i++) {
      free(args[i]);
    }
  }
  return 0;
}

