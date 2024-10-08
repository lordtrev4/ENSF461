#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include "parser.h"
#include <string.h>
#include <stdlib.h>

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
            break; // end of file
        }
        if (res < 0)
        { // Read Error
            perror("Error reading file");
            return -1;
        }
        buffer[readlen++] = ch;
        if (ch == '\n')
        {
            break; // end of line
        }
    }

    buffer[readlen] = '\0'; 
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

            for (int j = i; j < numtokens - 2; j++)
            {
                tokens[j] = tokens[j + 2];
            }
            numtokens -= 2;
            break;
        }
    }
}

void pipe_command(token_t **left_tokens, int left_numtokens, token_t **right_tokens, int right_numtokens)
{
    int pipe_fd[2];

    if (pipe(pipe_fd) == -1)
    {
        perror("Error: pipe failed");
        exit(EXIT_FAILURE);
    }
    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        char *argv1[64];
        int arg_count1 = 0;
        for (int i = 0; i < left_numtokens; i++)
        {
            argv1[arg_count1] = left_tokens[i]->value;
            arg_count1++;
        }
        argv1[left_numtokens] = NULL;

        if (execvp(argv1[0], argv1) == -1)
        {
            perror("Error: execvp failed for left command");
            exit(EXIT_FAILURE);
        }
    }
    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        char *argv2[64];
        int arg_count2 = 0;
        for (int i = 0; i < right_numtokens; i++)
        {
            argv2[arg_count2] = right_tokens[i]->value;
            arg_count2++;
        }

        argv2[right_numtokens] = NULL;

        redirect(right_tokens, right_numtokens);
        if (execvp(argv2[0], argv2) == -1)
        {
            perror("Error: execvp failed for right command");
            exit(EXIT_FAILURE);
        }
    }
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// Function to execute a command
void execute_command(token_t **tokens, int numtokens)
{
    int pipe_index = -1;
    for (int i = 0; i < numtokens; i++)
    {
        if (tokens[i]->type == TOKEN_PIPE)
        {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index != -1)
    {
        token_t **left_tokens = tokens;
        int left_numtokens = pipe_index;

        token_t **right_tokens = &tokens[pipe_index + 1];
        int right_numtokens = numtokens - pipe_index - 1;

        pipe_command(left_tokens, left_numtokens, right_tokens, right_numtokens);
    }
    else
    {
        char *argv[64];
        int arg_count = 0;

        // Extract the command and its arguments
        for (int i = 0; i < numtokens; i++)
        {
            argv[arg_count] = tokens[i]->value; // token value as arguement
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
}


void update_variable(char *name, char *value)
// update or create a variable
{
    // check is variable exists and updates
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            free(variables[i].value);
            variables[i].value = strdup(value);
            return;
        }
    }
    
    // adding new variable
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

        // load next line
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

        if (strchr(buffer, '=') != NULL) {
            // Handle variable assignment
            char *name = strtok(buffer, " =");
            char *value = strtok(NULL, "\n"); 
            if (value != NULL) {
                update_variable(name, value);
            }
            continue; 
        }

        for (int i = 0; i < numtokens; i++) {
            if (tokens[i]->type == TOKEN_VAR && tokens[i]->value[0] == '$') {
                char *var_name = tokens[i]->value + 1; 
                char *var_value = lookup_variable(var_name);
                if (var_value != NULL) {
                    free(tokens[i]->value);
                    tokens[i]->value = strdup(var_value); 
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

