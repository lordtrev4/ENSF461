#include "util.h"

int* read_next_line(FILE* fin) {
    // TODO: This function reads the next line from the input file
    // The line is a comma-separated list of integers
    // Return the list of integers as an array where the first element
    // is the number of integers in the rest of the array
    // Return NULL if there are no more lines to read
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), fin) == NULL) {
        return NULL;
    }

    int count = 0;
    char* token = strtok(buffer, ",");
    while (token != NULL) {
        count++;
        token = strtok(NULL, ",");
    }

    int* result = (int*)malloc((count + 1) * sizeof(int));
    if (result == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    result[0] = count;
    rewind(fin);
    fgets(buffer, sizeof(buffer), fin);
    token = strtok(buffer, ",");
    for (int i = 1; i <= count; i++) {
        result[i] = atoi(token);
        token = strtok(NULL, ",");
    }

    return result;

}


float compute_average(int* line) {
    // TODO: Compute the average of the integers in the vector
    // Recall that the first element of the vector is the number of integers
     int count = line[0];
    int sum = 0;
    for (int i = 1; i <= count; i++) {
        sum += line[i];
    }
    return (float)sum / count;

}


float compute_stdev(int* line) {
    // TODO: Compute the standard deviation of the integers in the vector
    // Recall that the first element of the vector is the number of integers
        int count = line[0];
    float mean = compute_average(line);
    float sum = 0.0;
    for (int i = 1; i <= count; i++) {
        sum += (line[i] - mean) * (line[i] - mean);
    }
    return sqrt(sum / count);

}