#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



// Functions
int wish_command(char **args);
int wish_help_command(char **args);
int wish_exit_command(char **args);


// List of Commands
char *builtin_str[] = {
        "cd",
        "help",
        "exit"
};

int (*builtin_func[]) (char **) = {
        &wish_command,
        &wish_help_command,
        &wish_exit_command
};

int wish_num_bull() {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/




/**
   @happens Builtin command: change directory.
   @pre args List of args.  args[0] is "cd".  args[1] is the directory.
   @post Always returns 1, to continue executing.
 */
int wish_command(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "wish: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("wish");
        }
    }
    return 1;
}

/**
   @happens Builtin command: print help.
   @pre args List of args.  Not examined.
   @post Always returns 1, to continue executing.
 */
int wish_help_command(char **args)
{
    int i;
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < wish_num_bull(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

/**
   @happens Builtin command: print help.
   @pre args List of args.  Not examined.
   @post Always returns 1, to continue executing.
 */
int wish_exit_command(char **args)
{
    return 0;
}

/**
  @happens Launch a program and wait for it to terminate.
  @pre args Null terminated list of arguments (including program).
  @post Always returns 1, to continue execution.
 */
int wish_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("wish");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("wish");
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/**
   @happens Builtin command: print help.
   @pre args List of args.  Not examined.
   @post Always returns 1, to continue executing.
 */
int wish_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < wish_num_bull(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return wish_launch(args);
}

/**
   @happens Read a line of input from stdin.
   @post The line from stdin.
 */
char *wish_readline(void)
{
#ifdef WISH_USE_STD_GETLINE
    char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("wish: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define WISH_RL_BUFSIZE 1024
    int bufsize = WISH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "wish: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Read a character
        c = getchar();

        if (c == EOF) {
            exit(EXIT_SUCCESS);
        } else if (c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // If we have exceeded the buffer, reallocate.
        if (position >= bufsize) {
            bufsize += WISH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "wish: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
#endif
}

#define WISH_TOK_BUFSIZE 64
#define WISH_TOK_DELIM " \t\r\n\a"
/**
   @happens Split a line into tokens (very naively).
   @pre line The line.
   @post Null-terminated array of tokens.
 */
char **wish_split_line(char *line)
{
    int bufsize = WISH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "wish: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, WISH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += WISH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "wish: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, WISH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/**
   @happens Loop getting input and executing it.
 */
void wish_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("wish> ");
        line = wish_readline();
        args = wish_split_line(line);
        status = wish_execute(args);

        free(line);
        free(args);
    } while (status);
}

/**
   @happens Main entry point.
   @pre argc Argument count. argv Argument vector.
   @post status code
 */
int main(int argc, char **argv)
{
    // Load config files, if any.

    // Run command loop.
    wish_loop();

    // Perform any shutdown/cleanup.

    return EXIT_SUCCESS;
}
