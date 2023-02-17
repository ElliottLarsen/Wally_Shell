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

int main(void) {
  char *user_input = NULL;
  int args_num = 0;
  size_t n = 0;
  char *IFS;
  char *home_dir;
  char pid[1024];
  sprintf(pid, "%d", getpid());
  

  char *out_file_name = NULL;
  char *in_file_name = NULL;
  char *args[1024 + 5] = {NULL};
  char *tokens = NULL;

  int is_home_dir_expanded = 0;
  int should_run_in_bg = 0;

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
        printf("\nString split error\n");
        exit(1);
      }
      tokens = strtok(NULL, IFS);
    }


    // If no input is given.
    if (args_num == 0) {
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
        // "$$" expansion with the shell's PID.
        str_gsub(&args[i], "$$", pid, &is_home_dir_expanded);
        // "$?" expansion.
        // "$!" expansion.
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

    /*
    // cat myfile > output.txt #
    if (i >= 3 && i == args_num - 1 && strcmp(args[i], "#") == 0) {
      args[i] = NULL;
      if(strcmp(args[i - 2], ">") == 0) {
        out_file_name = args[i - 1];
        args[i - 2] = NULL;
      } else if (strcmp(args[i - 2], "<") == 0) {
        in_file_name = args[i - 1];
        args[i - 2] = NULL;
      }
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
    // cat myfilie > output.txt &
    if (i >= 3 && strcmp(args[i], "&") == 0) {
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
    if (i >= 3) {
      if (strcmp(args[i - 1], ">") == 0) {
        out_file_name = args[i];
        args[i - 1] = NULL;
      } else if (strcmp(args[i - 1], "<") == 0) {
        in_file_name = args[i];
        args[i - 1] = NULL;
      }
    }
    */


    // 
    /*
    i = 0;
    while (i < args_num) {
      if ((strcmp(args[i], "#") == 0) || i == args_num - 1) {
        if (strcmp(args[i], "#") == 0) {
          args[i] = NULL;
        }
        if (i >= 2) {
          if (strcmp(args[i - 1], "&") == 0) {
            should_run_in_bg = 1;
            args[i - 1] = NULL;
          }

          if (strcmp(args[i - 2], ">") == 0) {
            out_file_name = args[i - 2];
          }

          if (strcmp(args[i- 2], "<") == 0) {
            in_file_name = args[i - 1];
          }

        }
        i++;
      }
      i++;
    }
    */

    /*
    int back_pointer = args_num - 1;
    while (back_pointer >=0) {
      // The first occurrence of the word "#" and any additional words following it shall be ignored as a command.
      if (strncmp(args[back_pointer], "#", 1) == 0) {
        args[back_pointer] = NULL;
        back_pointer--;
        continue;
      }

      // If the last word is &, it shall indicate that the command is to be run in the background.
      if (strcmp(args[back_pointer], "&") == 0) {
        should_run_in_bg = 1;
        args[back_pointer] = NULL;
        back_pointer--;
      }

      if (back_pointer > 0 && strcmp(args[back_pointer - 1], ">") == 0) {
        out_file_name = args[back_pointer];
        args[back_pointer - 1] = NULL;
        back_pointer--;
      }

      if (back_pointer > 0 && strcmp(args[back_pointer - 1], "<") == 0) {
        in_file_name = args[back_pointer];
        args[back_pointer - 1] = NULL;
        back_pointer--;
      }
      
      back_pointer--;
    }
    */
    
    
    // Execution - Do I account for the comments here?
    // Executing "exit".
    if (strcmp(args[0], "exit") == 0) {
      printf("EXIT COMMAND RECEIVED");
    } 
    //Executing "cd".
    else if (strcmp(args[0], "cd") == 0) {
      if (args_num > 2) {
        fprintf(stderr, "Incorrect number of arguments\n");
        continue;
      } else if (args_num == 2) {
        // Check for the error.
        if (chdir(args[1]) != 0) {
          fprintf(stderr, "Cannot change directory\n");
          continue;
        } else {
          fprintf(stdout, "successful change of directory\n");
        }
      } else {
        home_dir = getenv("HOME");
        if (home_dir) {
          if (chdir(home_dir) != 0) {
            fprintf(stderr, "Cannot change directory\n");
            continue;
          } else {
            fprintf(stdout, "successful change of directory\n");
          }
        } else {
          // What to do if "HOME" environment variable doesn't exist?
          fprintf(stderr, "Cannot go to HOME\n");
          continue;
        }
      }
    } 
    // Executing built-in commands.
    else {
      printf("BUILT-IN");
    }

    
         
  }
  return 0;
}
