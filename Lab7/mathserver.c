#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define MAX_CONTEXTS 16
#define MAX_QUEUE_SIZE 1000
#define BATCH_SIZE 10
#define MAX_LOG_LINE 2048
#define MAX_PRIMES_BUFFER 100000  // Increased buffer size for more primes

// Structure to hold an operation
typedef struct {
    char op[4];      // Operation type (set, add, sub, mul, div, pri, pia, fib)
    int ctx;         // Context number
    double val;      // Value for operation
} Operation;    

// Structure to hold a queue of operations for a context
typedef struct {
    Operation ops[MAX_QUEUE_SIZE];
    int size;
    int current;
} OperationQueue;

// Structure to pass to thread
typedef struct {
    int ctx_id;
    double* ctx_value;
    OperationQueue* queue;
    FILE* output_file;
    pthread_mutex_t* log_mutex;
} ThreadArgs;

// Global variables
OperationQueue context_queues[MAX_CONTEXTS];
double context_values[MAX_CONTEXTS] = {0};
pthread_mutex_t log_mutex;

// Function to compute nth Fibonacci number recursively
int compute_fibonacci(int n) {
    if (n <= 1) return n;
    return compute_fibonacci(n - 1) + compute_fibonacci(n - 2);
}

// Function to check if a number is prime
int is_prime(int n) {
    if (n <= 1) return 0;
    for (int i = 2; i <= sqrt(n); i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}
int count_primes(int n) {
    // Allocate a boolean array to mark non-prime numbers
    char* is_prime = malloc((n + 1) * sizeof(char));
    if (is_prime == NULL) return 0;

    // Initialize all numbers as prime
    memset(is_prime, 1, (n + 1) * sizeof(char));
    is_prime[0] = is_prime[1] = 0;  // 0 and 1 are not prime

    // Use Sieve of Eratosthenes algorithm
    for (int i = 2; i * i <= n; i++) {
        if (is_prime[i]) {
            // Mark multiples of i as non-prime
            for (int j = i * i; j <= n; j += i) {
                is_prime[j] = 0;
            }
        }
    }

    // Count the number of primes
    int count = 0;
    for (int i = 2; i <= n; i++) {
        if (is_prime[i]) count++;
    }

    free(is_prime);
    return count;
} 

// Function to compute prime factors
void compute_primes(int n, char* result, size_t result_size) {
    result[0] = '\0';
    char temp[32];
    int first = 1;
    size_t current_len = 0;
    
    for (int i = 2; i <= n; i++) {
        if (is_prime(i)) {
            // Calculate space needed for this number plus separator
            size_t needed = snprintf(NULL, 0, "%s%d", first ? "" : ", ", i);
            
            // Make sure we have enough space (including null terminator)
            if (current_len + needed >= result_size - 1)
                break;
                
            // Add separator if not first number
            if (!first) {
                strcat(result, ", ");
                current_len += 2;
            }
            
            // Add the number
            sprintf(temp, "%d", i);
            strcat(result, temp);
            current_len += strlen(temp);
            first = 0;
        }
    }
}

// Function to approximate pi using Leibniz series
double approximate_pi(int iterations) {
    double pi = 0.0;
    for (int i = 0; i < iterations; i++) {
        double term = 1.0 / (2 * i + 1);
        if (i % 2 == 0)
            pi += term;
        else
            pi -= term;
    }
    return 4 * pi;
}

// Function to process operations in a context
void process_operation(Operation* op, double* ctx_value, char* log_line) {
    if (strcmp(op->op, "set") == 0) {
        *ctx_value = op->val;
        sprintf(log_line, "ctx %02d: set to value %d", op->ctx, (int)op->val);
    }
    else if (strcmp(op->op, "add") == 0) {
        *ctx_value += op->val;
        sprintf(log_line, "ctx %02d: add %d (result: %d)", op->ctx, (int)op->val, (int)*ctx_value);
    }
    else if (strcmp(op->op, "sub") == 0) {
        *ctx_value -= op->val;
        sprintf(log_line, "ctx %02d: sub %d (result: %d)", op->ctx, (int)op->val, (int)*ctx_value);
    }
    else if (strcmp(op->op, "mul") == 0) {
        *ctx_value *= op->val;
        sprintf(log_line, "ctx %02d: mul %d (result: %d)", op->ctx, (int)op->val, (int)*ctx_value);
    }
    else if (strcmp(op->op, "div") == 0) {
        *ctx_value /= op->val;
        sprintf(log_line, "ctx %02d: div %d (result: %d)", op->ctx, (int)op->val, (int)*ctx_value);
    }
    else if (strcmp(op->op, "pri") == 0) {
        char primes[MAX_PRIMES_BUFFER] = "";
        compute_primes((int)*ctx_value, primes, sizeof(primes));
        sprintf(log_line, "ctx %02d: primes (result: %s)", op->ctx, primes);
    }
    else if (strcmp(op->op, "pia") == 0) {
        double pi = approximate_pi((int)*ctx_value);
        sprintf(log_line, "ctx %02d: pia (result %.15f)", op->ctx, pi);
    }
    else if (strcmp(op->op, "fib") == 0) {
        int fib = compute_fibonacci((int)*ctx_value);
        sprintf(log_line, "ctx %02d: fib (result: %d)", op->ctx, fib);
    }
}

// Thread function to process operations in a context
void* context_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    char log_batch[BATCH_SIZE][MAX_LOG_LINE];
    int batch_count = 0;
    
    while (args->queue->current < args->queue->size) {
        Operation* op = &args->queue->ops[args->queue->current];
        char log_line[MAX_LOG_LINE];
        
        process_operation(op, args->ctx_value, log_line);
        strcpy(log_batch[batch_count], log_line);
        batch_count++;
        args->queue->current++;
        
        // Write batch to file if we have BATCH_SIZE operations or queue is complete
        if (batch_count == BATCH_SIZE || args->queue->current == args->queue->size) {
            pthread_mutex_lock(args->log_mutex);
            for (int i = 0; i < batch_count; i++) {
                fprintf(args->output_file, "%s\n", log_batch[i]);
            }
            fflush(args->output_file);
            pthread_mutex_unlock(args->log_mutex);
            batch_count = 0;
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    const char usage[] = "Usage: mathserver.out <input trace> <output trace>\n";
    char buffer[1024];
    pthread_t threads[MAX_CONTEXTS];
    ThreadArgs thread_args[MAX_CONTEXTS];
    int active_contexts = 0;

    // Parse command line arguments
    if (argc != 3) {
        printf("%s", usage);
        return 1;
    }

    // Open input and output files
    FILE* input_file = fopen(argv[1], "r");
    FILE* output_file = fopen(argv[2], "w");
    
    if (!input_file || !output_file) {
        printf("Error opening files\n");
        return 1;
    }

    // Initialize mutex
    pthread_mutex_init(&log_mutex, NULL);

    // Read and parse input file
    while (!feof(input_file)) {
        char* rez = fgets(buffer, sizeof(buffer), input_file);
        if (!rez)
            break;

        // Remove newline character
        buffer[strlen(buffer) - 1] = '\0';

        // Parse operation
        Operation op;
        char* token = strtok(buffer, " ");
        strcpy(op.op, token);
        
        token = strtok(NULL, " ");
        op.ctx = atoi(token);
        
        if (strcmp(op.op, "pri") != 0 && strcmp(op.op, "pia") != 0 && strcmp(op.op, "fib") != 0) {
            token = strtok(NULL, " ");
            op.val = atof(token);
        }

        // Add operation to appropriate queue
        OperationQueue* queue = &context_queues[op.ctx];
        queue->ops[queue->size++] = op;
        
        // Mark context as active
        if (queue->size == 1) {
            active_contexts++;
        }
    }

    // Create threads for each active context
    for (int i = 0; i < MAX_CONTEXTS; i++) {
        if (context_queues[i].size > 0) {
            thread_args[i].ctx_id = i;
            thread_args[i].ctx_value = &context_values[i];
            thread_args[i].queue = &context_queues[i];
            thread_args[i].output_file = output_file;
            thread_args[i].log_mutex = &log_mutex;
            
            pthread_create(&threads[i], NULL, context_thread, &thread_args[i]);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < MAX_CONTEXTS; i++) {
        if (context_queues[i].size > 0) {
            pthread_join(threads[i], NULL);
        }
    }

    // Cleanup
    pthread_mutex_destroy(&log_mutex);
    fclose(input_file);
    fclose(output_file);

    return 0;
}