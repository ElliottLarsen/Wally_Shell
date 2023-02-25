# Wally Shell
This is a shell built in C.  This shell implements a command line interface similar to well-known shells such as bash.
## Technologies
* C
## Project Details
* This shell prints interactive input prompt.
* It then parses command line input into semantic tokens and performs parameter expansion.
* There are two built-in commands, `exit` and `cd`, which this shell will execute.
* Other commands are executed using appropriate `exec(3)` function.
* This shell also implements custom behavior for `SIGINT` and `SIGTSTP` signals.
## GIF Walkthrough