// Author: Elliott Larsen
// Date:
// Description: 

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
#include <fcntl.h>
#include <stdint.h>


char *str_gsub(char *restrict *restrict input_line, char const *restrict old_word, char const *restrict new_word, int *is_home_dir_expanded);
void reset_vars(pid_t *child_pid, int *args_num, int *infile_descriptor, int *outfile_descriptor, int *redirection_result);
void reset_flags(int *should_run_in_bg, int *is_infile, int *is_outfile);
void reset_strs(char *tokens, char *in_file_name, char *out_file_name, char pid[1024]);
void split_input(int *args_num, char *IFS, char *tokens, char *user_input, char *args[1029]);

void expand_input(int *args_num, int *is_home_dir_expanded, char *home_dir, char pid[1024], char *fg_status, char *bg_status, char *args[1029]);
int is_invalid_input(int *args_num, char *args[1029]);
void parse_input(int *args_num, int *should_run_in_bg, int *is_infile, int *is_outfile, char **in_file_name, char **out_file_name, char *args[1029]);
int execute_exit(int *args_num, char *fg_status, char *args[1029]);
int execute_cd(int *args_num, char *home_dir, char *args[1029]);
void free_memory(int *args_num, char *args[1029]);

void SIGINT_handler(int signo) {
}

void SIGINT_default(int signo) {
  exit(0);
}

int main(void) {
  char *user_input = NULL;
  int args_num = 0;
  size_t n = 0;
  char *IFS = " \t\n";
  char *home_dir;
  char pid[1024];
  //sprintf(pid, "%d", getpid());
  pid_t child_pid;
  pid_t bg_children;
  // "$$" defaults to "0"
  char *fg_status = "0";
  // ""$!" defaults to "" if no background process ID is available.
  char *bg_status = "";
  
  char *out_file_name = NULL;
  char *in_file_name = NULL;
  char *args[1024 + 5] = {NULL};
  char *tokens = NULL;
  int infile_descriptor;
  int outfile_descriptor;
  int redirection_result;
  int child_status;

  int is_home_dir_expanded = 0;
  int should_run_in_bg = 0;
  int is_infile = 0;
  int is_outfile = 0;

  signal(SIGTSTP, SIG_IGN);
  struct sigaction old_sigint = {0};
  old_sigint.sa_handler = SIGINT_default; 
  sigfillset(&old_sigint.sa_mask);
  old_sigint.sa_flags = 0;

  struct sigaction sigactIgnore = {0};
  sigactIgnore.sa_handler = SIG_IGN;
  sigfillset(&sigactIgnore.sa_mask);
  sigactIgnore.sa_flags = 0;

  sigaction(SIGINT, &sigactIgnore, &old_sigint);

  for (;;) {
    // Reset variables.
    reset_vars(&child_pid, &args_num, &infile_descriptor, &outfile_descriptor, &redirection_result);
    reset_flags(&should_run_in_bg, &is_infile, &is_outfile);
    reset_strs(tokens, in_file_name, out_file_name, pid);

    // Managing Background Processes
    bg_children = waitpid(0, &child_status, WUNTRACED | WNOHANG);
    while (bg_children > 0) {
      fflush(stdout);

      if (WIFEXITED(child_status)) {
        fprintf(stderr, "Child process %jd done.  Exit status %d.\n", (intmax_t)bg_children, WEXITSTATUS(child_status));
      }
      if (WIFSIGNALED(child_status)) {
        fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t)bg_children, WTERMSIG(child_status));
      }
      if (WIFSTOPPED(child_status)) {
        kill(bg_children, SIGCONT);
        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t)bg_children, WSTOPSIG(child_status));
      }
      fflush(stderr);
      bg_children = waitpid(0, &child_status, WUNTRACED | WNOHANG);
    }

    
    
    // Prompt
    char *prompt = "WallyShell> ";
    fflush(stdout);
    fprintf(stderr, "%s", prompt);
    fflush(stderr);

    // Register SIGTSTP to be ignored.
    ssize_t line_length = getline(&user_input, &n, stdin);
    // Remove "\n".
    user_input[strlen(user_input) - 1] = '\0';

    // Error from reading an input.
    if (line_length == -1) {
      clearerr(stderr);
      //clearerr(stdin);
      errno = 0;
      continue;
    }

    split_input(&args_num, IFS, tokens, user_input, args);
    if (is_invalid_input(&args_num, args)) {
      continue;
    }
    expand_input(&args_num, &is_home_dir_expanded, home_dir, pid, fg_status, bg_status, args);
    parse_input(&args_num, &should_run_in_bg, &is_infile, &is_outfile, &in_file_name, &out_file_name, args);
   
    if (strcmp(args[0], "exit") == 0) {
      int exit_status = execute_exit(&args_num, fg_status, args);
      free_memory(&args_num, args);
      exit(exit_status);
    }

    else if (strcmp(args[0], "cd") == 0) {
      execute_cd(&args_num, home_dir, args);
      free_memory(&args_num, args);
      continue;
    }

    // Executing built-in commands.
    else {

      child_pid = fork();
      //printf("\nHere is child_pid from fork(): %d\n", child_pid);
      
      switch(child_pid) {
        case -1:
          fflush(stdout);
          fprintf(stderr, "fork() error. Handle this.\n");
          fflush(stderr);
          exit(1);
          break;

        case 0:
          // All signals should be reset to their original dispositions when the shell was invoked.
          sigaction(SIGINT, &old_sigint, NULL);
          if (is_infile) {
            infile_descriptor = open(in_file_name, O_RDONLY);
            if (infile_descriptor == -1) {
              fflush(stdout);
              fprintf(stderr, "open() error for input redirection.");
              fflush(stderr);
              exit(1);
            }
            // stdin file descriptor is 0.
            redirection_result = dup2(infile_descriptor, 0);
            if (redirection_result == -1) {
              fflush(stdout);
              fprintf(stderr, "dup2() error from input redirection.");
              fflush(stderr);
              exit(1);
            }
            // Is this it?  More things to do?
          }

          if (is_outfile) {
            outfile_descriptor = open(out_file_name, O_RDWR | O_CREAT, 0777);
            if (outfile_descriptor == -1) {
              fflush(stdout);
              fprintf(stderr, "open() error for output redirection.");
              fflush(stderr);
            }
            // stdout file decriptor is 1.
            redirection_result = dup2(outfile_descriptor, 1);
            if (redirection_result == -1) {
              fflush(stdout);
              fprintf(stderr, "dup2() error from output redirection.");
              fflush(stderr);
              exit(1);
            }
          }

          //printf("Message from the child process.\n");
          // Error check for execvp.
          execvp(args[0], args);

          if (is_infile) {
            close(infile_descriptor);
          }

          if (is_outfile) {
            close(outfile_descriptor);
          }

          fflush(stdout);
          fprintf(stderr, "execvp() error. Handle this.\n");
          fflush(stderr);
          exit(1);
          break;

        default:
          //printf("Message from the shell.\n");
          //printf("Here is child_pid from default: %d\n", child_pid);
          if (!should_run_in_bg) {
            //printf("\nPerform a blocking wait on the foreground child process.\n");
            child_pid = waitpid(child_pid, &child_status, 0);
            fg_status = calloc(1024, sizeof(char));
            //printf("Here is child_pid after waitpid: %d", child_pid);
            if (WIFSIGNALED(child_status)) {
              sprintf(fg_status, "%d", 128 + WTERMSIG(child_status));
            } 

            else if (WIFSTOPPED(child_status)) {
              kill(child_pid, SIGCONT);
              fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t)child_pid);
              if (should_run_in_bg) {
                sprintf(bg_status, "%d", child_pid);
              }
            }

            else {
              sprintf(fg_status, "%d", WEXITSTATUS(child_status));
            }
            //printf("Here is fg_status: %s", fg_status);
          } else {
            bg_status = calloc(1024, sizeof(char));
            sprintf(bg_status, "%d", child_pid);
          }
          goto free;
      
      }
      //continue;
      //printf("BUILT-IN");
    }

free:
    /*
    // Free realloc() from str_gsub();
    for (int i = 0; i < args_num; i++) {
      free(args[i]);
      args[i] = NULL;
    }
    */
    free_memory(&args_num, args);
    continue;
  }
  return 0;
}

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

void split_input(int *args_num, char *IFS, char *tokens, char *user_input, char *args[1029]) {
  IFS = getenv("IFS");
  if (IFS == NULL) {
    IFS = " \t\n";
  }

  tokens = strtok(user_input, IFS);
  while (tokens != NULL) {
    if (strdup(tokens) != NULL) {
      args[*args_num] = strdup(tokens);
      *args_num += 1;
    } else {
      fflush(stdout);
      fprintf(stderr, "\nString split error\n");
      fflush(stderr);
      exit(1);
    }
    tokens = strtok(NULL, IFS);
  }
}

void expand_input(int *args_num, int *is_home_dir_expanded, char *home_dir, char pid[1024], char *fg_status, char *bg_status, char *args[1029]) {
  int i = 0;
  while (i < *args_num && args[i] != NULL) {
    // "~/" expansion with the HOME environment variable.
    *is_home_dir_expanded = 0;
    if (!(home_dir = getenv("HOME"))) {
      home_dir = "";
    }
    if (strncmp(args[i], "~/", 2) == 0) {
      *is_home_dir_expanded = 1;
      str_gsub(&args[i], "~", home_dir, is_home_dir_expanded);
    }

    // "$$" expansion with the shell's PID.
    str_gsub(&args[i], "$$", pid, is_home_dir_expanded);
    str_gsub(&args[i], "$?", fg_status, is_home_dir_expanded);
    str_gsub(&args[i], "$!", bg_status, is_home_dir_expanded);

    i++;
  }
}

int is_invalid_input(int *args_num, char *args[1029]) {
  if (*args_num == 0 || (strcmp(args[0], "#") == 0) || (strncmp(args[0], "#", 1) == 0)) {
    return 1;
  }
  return 0;
}

void parse_input(int *args_num, int *should_run_in_bg, int *is_infile, int *is_outfile, char **in_file_name, char **out_file_name, char *args[1029]) {
  // Parsing
  int i = 0;
  // Find the index number to either # or end of the input.
  while (i < *args_num) {
    if ((strcmp(args[i], "#") == 0) || i == *args_num -1) {
      break;
    }
    i++;
  }

  // cat myfile > output.txt & #
  if (i >= 3 && i == *args_num - 1 && strcmp(args[i], "#") == 0 && strcmp(args[i - 1], "&") == 0) {
    args[i] = NULL;
    *should_run_in_bg = 1;
    if(strcmp(args[i - 3], ">") == 0) {
      *out_file_name = args[i - 2];
      args[i - 3] = NULL;
      *is_outfile = 1;
    } else if (strcmp(args[i - 3], "<") == 0) {
      *in_file_name = args[i - 2];
      args[i - 3] = NULL;
      *is_infile = 1;
    }
  }
  // cat myfile > output.txt #
  else if (i >= 3 && i == *args_num - 1 && strcmp(args[i], "#") == 0) {
    args[i] = NULL;
    if(strcmp(args[i - 2], ">") == 0) {
      *out_file_name = args[i - 1];
      args[i - 2] = NULL;
      *is_outfile = 1;
    } else if (strcmp(args[i - 2], "<") == 0) {
      *in_file_name = args[i - 1];
      args[i - 2] = NULL;
      *is_infile = 1;
    }
  }
  // cat myfilie > output.txt &
  else if (i >= 3 && strcmp(args[i], "&") == 0) {
    args[i] = NULL;
    *should_run_in_bg = 1;
    if (strcmp(args[i - 2], ">") == 0) {
      *out_file_name = args[i - 1];
      args[i - 2] = NULL;
      *is_outfile = 1;
    } else if (strcmp(args[i - 2], "<") == 0) {
      *in_file_name = args[i - 1];
      args[i - 2] = NULL;
      *is_infile = 1;
    }
  }
  // cat myfile > output.txt
  else if (i >= 3) {
    if (strcmp(args[i - 1], ">") == 0) {
      *out_file_name = args[i];
      *is_outfile = 1;
      args[i - 1] = NULL;
      if (strcmp(args[i - 3], "<") == 0) {
        *in_file_name = args[i - 2];
        args[i - 2] = NULL;
        *is_infile = 1;
        args[i - 3] = NULL;
      }
    } else if (strcmp(args[i - 1], "<") == 0) {
      *in_file_name = args[i];
      args[i - 1] = NULL;
      *is_infile = 1;
      if (strcmp(args[i - 3], ">") == 0) {
        *out_file_name = args[i - 2];
        args[i - 2] = NULL;
        *is_outfile = 1;
        args[i - 3] = NULL;
      }
    }
  }
   
  else if (i >= 2 && strcmp(args[i], "#") == 0) {
    args[i] = NULL;
  }

  else if (i >= 2 && strcmp(args[i], "&") == 0) {
    *should_run_in_bg = 1;
    args[i] = NULL;
  }

  else if (i >= 1 && strcmp(args[i], "#") == 0) {
    args[i] = NULL;
  }

  else if (i >= 1 && strcmp(args[i], "&") == 0) {
    *should_run_in_bg = 1;
    args[i] = NULL;
  }
}


void reset_vars(pid_t *child_pid, int *args_num, int *infile_descriptor, int *outfile_descriptor, int *redirection_result) {
  *child_pid = -5;
  *args_num = 0;
  *infile_descriptor = -5;
  *outfile_descriptor = -5;
  *redirection_result = -5;
}


void reset_flags(int *should_run_in_bg, int *is_infile, int *is_outfile) {
  *should_run_in_bg = 0;
  *is_infile = 0;
  *is_outfile = 0;
}


void reset_strs(char *tokens, char *in_file_name, char *out_file_name, char pid[1024]) {
  tokens = NULL;
  in_file_name = NULL;
  out_file_name = NULL;
  sprintf(pid, "%d", getpid());
}

void free_memory(int *args_num, char *args[1029]) {
  for (int i = 0; i < *args_num; i++) {
    free(args[i]);
    args[i] = NULL;
  }
}

int execute_exit(int *args_num, char *fg_status, char *args[1029]) {
  if (*args_num > 2) {
    // If too many arguments are given.
    fflush(stdout);
    fprintf(stderr, "'exit' command takes only one argument.\n");
    fflush(stderr);
    return 1;
  }

  if (*args_num == 2) {
    if (isdigit(*args[1]) == 0) {
      // Correct number of argument is given but it is not an integer.
      fflush(stdout);
      fprintf(stderr, "'exit' command only takes an integer argument.\n");
      fflush(stderr);
      return 1;
    } else {
      fprintf(stderr, "\nexit\n");
      kill(getpid(), SIGINT);
      // Correct number of argument is given and it IS an integer.
      //printf("Print to stderr, sent SIGINT to all child processes, and exit immediately with the given integer value.\n");
      return atoi(args[1]);
    }
  } else {
    // No argument is given so it exits.
    //printf("Expand $? and exit.\n");
    //printf("%d", atoi(fg_status));
    fflush(stdout);
    fprintf(stderr, "\nexit\n");
    fflush(stderr);
    return atoi(fg_status);
  }
  return 0;
}

int execute_cd(int *args_num, char *home_dir, char *args[1029]) {
  // Executing "cd".
  int cd_result;
  if (*args_num > 2) {
    fflush(stdout);
    fprintf(stderr, "Incorrect number of arguments.\n");
    fflush(stderr);
  }

  else if (*args_num == 2) {
    cd_result = chdir(args[1]);
    if (cd_result != 0) {
      fflush(stdout);
      fprintf(stderr, "Cannot change directory.\n");
      fflush(stderr);
    }
  }

  else {
    home_dir = getenv("HOME");
    if (home_dir) {
      cd_result = chdir(home_dir);
      if (cd_result != 0) {
        fflush(stdout);
        fprintf(stderr, "Cannot change directory to home.\n");
        fflush(stderr);
      }
    } else {
      fflush(stdout);
      fprintf(stderr, "getenv('HOME') failed.");
      fflush(stderr);
    }
  }
  return 0;
}
