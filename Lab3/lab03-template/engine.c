#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include "parser.h"

int read_line(int infile, char *buffer, int maxlen)
{
    int readlen = 0;
    char ch;

    // TODO: Read a single line from file; retains final '\n'
    while (readlen < maxlen - 1)
    {
        int res = read(infile, &ch, 1);

        if (res == 0)
        {
            break; // End of File, return 0
        }
        if (res < 0)
        { // Read Error
            perror("Error reading file");
            return -1;
        }
        buffer[readlen++] = ch;
        if (ch == '\n')
        {
            break; // End of line
        }
    }

    buffer[readlen] = '\0'; // Null terminate the string
    return readlen;
}

// Function to execute a command
void execute_command(token_t **tokens, int numtokens)
{
    char *argv[64];
    int arg_count = 0;

    // Extract the command and its arguments
    for (int i = 0; i < numtokens; i++)
    {
        argv[arg_count] = tokens[i]->value; // Store token's value as an argument
        arg_count++;
    }

    // Terminate the argument list for execve
    argv[arg_count] = '\0';

    if (arg_count == 0)
    {
        // No command to execute
        fprintf(stderr, "No command given.\n");
        return;
    }

    // Fork the process to run the command
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process: execute the command
        if (execvp(argv[0], argv) == -1)
        {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid > 0)
    {
        // Parent process: wait for the child to complete
        wait(NULL);
    }
    else
    {
        // Fork failed
        perror("fork failed");
    }
}

void update_variable(char *name, char *value)
{
    // Update or create a variable
}

char *lookup_variable(char *name)
{
    // Lookup a variable
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <input file>\n", argv[0]);
        return -1;
    }

    int infile = open(argv[1], O_RDONLY);
    if (infile < 0)
    {
        perror("Error opening input file");
        return -2;
    }

    char buffer[1024];
    int readlen;

    // Read file line by line
    while (1)
    {

        // Load the next line
        readlen = read_line(infile, buffer, 1024);
        if (readlen < 0)
        {
            perror("Error reading input file");
            return -3;
        }
        if (readlen == 0)
        {
            break;
        }

        // Tokenize the line
        int numtokens = 0;
        token_t **tokens = tokenize(buffer, readlen, &numtokens);
        assert(numtokens > 0);

        // Parse token list
        // * Organize tokens into command parameters
        // * Check if command is a variable assignment
        // * Check if command has a redirection
        // * Expand variables if any
        // * Normalize executables
        // * Check if pipes are present

        // * Check if pipes are present
        // TODO

        // Run commands
        // * Fork and execute commands
        // * Handle pipes
        // * Handle redirections
        // * Handle pipes
        // * Handle variable assignments
        // TODO

        // Execute the command
        execute_command(tokens, numtokens);

        // Free tokens vector
        for (int ii = 0; ii < numtokens; ii++)
        {
            free(tokens[ii]->value);
            free(tokens[ii]);
        }
        free(tokens);
    }

    close(infile);

    // Remember to deallocate anything left which was allocated dynamically
    // (i.e., using malloc, realloc, strdup, etc.)

    return 0;
}
