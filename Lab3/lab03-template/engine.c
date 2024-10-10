#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include "parser.h"
#include <string.h>

#define MAX_VARIABLES 1000  // Define a maximum number of variables

typedef struct {
    char *name;
    char *value;
} variable_t;

variable_t variables[MAX_VARIABLES];  // Array of variables
int variable_count = 0;  // Count of variables

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

void redirect(token_t **tokens, int numtokens)
{
    for (int i = 0; i < numtokens; i++)
    {
        if (tokens[i]->type == TOKEN_REDIR)
        {
            int fd = open(tokens[i + 1]->value, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0)
            {
                perror("Error: Cannot Open File");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);

            free(tokens[i]->value);
            free(tokens[i]);
            free(tokens[i + 1]->value);
            free(tokens[i + 1]);

            for (int j = 1; j < numtokens - 2; j++)
            {
                tokens[j] = tokens[j + 2];
            }
            numtokens -= 2;
            break;
        }
    }
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
    argv[arg_count] = NULL;

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
        redirect(tokens, numtokens);
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
// update or create a variable
{
    // Check if variable already exists and update it
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            free(variables[i].value);
            variables[i].value = strdup(value);
            return;
        }
    }
    
    // If not found, add new variable
    variables[variable_count].name = strdup(name);
    variables[variable_count].value = strdup(value);
    variable_count++;
}

char *lookup_variable(char *name)
{
    // Lookup a variable
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }   
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
        if (strchr(buffer, '=') != NULL) {
            // Handle variable assignment
            char *name = strtok(buffer, " =");
            char *value = strtok(NULL, "\n"); // Get the rest of the line as value
            update_variable(name, value);
            continue; // Skip executing this line
        }

        for (int i = 0; i < numtokens; i++) {
            if (tokens[i]->type == TOKEN_VAR && tokens[i]->value[0] == '$') {
                char *var_name = tokens[i]->value + 1; // Skip the '$'
                char *var_value = lookup_variable(var_name);
                if (var_value != NULL) {
                    free(tokens[i]->value);
                    tokens[i]->value = strdup(var_value); // Replace with the variable's value
                }
            }
        }

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
