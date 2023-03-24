# Wally Shell
This is a shell built in C.  This shell implements a command line interface similar to well-known shells such as bash.
## Technologies
* C
## GIF Walkthrough
<p>
<image src = "shellGIF.gif" title = "Shell GIF">
</p>

## How to Run
* Clone this repository.
* Open the directory in terminal.
* Run `make` to compile the project.
* Run `./WallyShell` to run the shell.

## Project Details
* This shell prints interactive input prompt and receives user input.
    * Before printing a prompt message, this shell checks for any unwaited-for background processes in the same process group ID as the shell itself and prints informative messages to `stderr`.
    * After printing the command prompt, this shell reads a line of input from `stdin`.  If reading is interrupted by a signal, a newline is printed followed by a new command prompt.
* It then splits command line input into semantic tokens, performs expansion, and parses the words.
    * Input Splitting
        * The line of input is split into words, delimited by the characters in the `IFS` environment variable.
        * If the `IFS` environment variable is unset, `" \t\n"` will be the delimiter.
    * Expansion
        * Any occurrence of `~/` at the beginning of a word will be replaced with the value of the `HOME` environment variable.
        * Any occurrence of `$$` within a word will be replaced with the process ID of the shell process.
        * Any occurrence of `$?` within a word will be replaced with the exit status of the last foreground process.
        * Any occurrence of `$!` within a word will be replaced with the process ID of the most recent background process.
        * If no information is available, `$?` defaults to `"0"` and `$!` defaults to `"\0"`.
    * Parsing
        * The first occurrence of the word `#` and any additional words following it will be ignored as a comment.
        * If the last word is `&`, it indicates that the command is to be run in the background.
        * If the last word is immediately preceded by the word `<`, it is interpreted as the filename operand of the input redirection operator.
        * If the last word is immediately preceded by the word `>`, it is interpreted as the filename operand of the output redirection operator.
* There are two built-in commands, `exit` and `cd` that this shell will execute.
    * `exit` Command
        * The `exit` command takes one integer argument.  If not provided, the argument is implied to be the expansion of `$?`, the exit status of the last foreground process.
        * The shell prints `"\nexit\n"` to `stderr` and exits with the specified (or implied) value.
        * All child processes will be sent a `SIGINT` signal before exiting.
    * `cd` Command
        * The `cd` command takes one argument.  If not provided, the argument is implied to be the expansion of `~/`, the value of the `HOME` environment variable.
        * The shell changes its own current working directory to the specified (or implied) path.
* Other commands are executed using appropriate `exec(3)` function.
    * The command with its arguments is executed in the new child process via `fork()`.
    * In the child process:
        * If a filename was specified as the operand to the input or output redirection operator (`<` or `>`), the specified file will be opened for reading/writing on `stdin`/`stdout`. 
        * If a file name was specified as the operand to the output redirection operator but the file does not exist, then the child process will create the file with permissions `0777`.
        * If an error occurs in the child process, it will print an error message to `stderr` and exit with a non-zero exit status.
## Who is Wally?
Wally, whom this shell is named after, is my companion animal and is a connoisseur of socks.
<p>
<image src = "Wally.JPEG" title="Wally" width = 300 height = 400>
</p>

## Contact
Elliott Larsen
* Email elliottlrsn@gmail.com
* GitHub [@elliottlarsen](https://github.com/elliottlarsen)
* LinkedIn [@elliottlarsen](https://www.linkedin.com/in/elliottlarsen)