#include <stdio.h>
#include <string.h>
#include "record_list.h"
#include "util.h"

// Function to write the list to the output file
void write_list_to_file(record_t* head, const char* filename) {
    FILE* fout = fopen(filename, "w");
    if (fout == NULL) {
        fprintf(stderr, "Error: unable to open/create file %s\n", filename);
        return;
    }
    record_t* current = head;
    while (current != NULL) {
        fprintf(fout, "%.5f,%.5f\n", current->avg, current->sdv);
        current = current->next;
    }
    fclose(fout);
}

// Function to free the memory allocated for the list
void free_list(record_t* head) {
    record_t* current = head;
    while (current != NULL) {
        record_t* next = current->next;
        free(current);
        current = next;
    }
}

int main(int argc, char** argv) {

    char usage[] = "Usage: parsecsv.out <input CSV file> <output CSV file>\n\n";
    char foerr[] = "Error: unable to open/create file\n\n";

    if ( argc != 3 ) {
        fprintf(stderr, "Usage: parsecsv.out <input CSV file> <output CSV file>\n\n");
        return -1;
    }

    FILE* fin = fopen(argv[1], "r");
    if ( fin == NULL ) {
        fprintf(stderr, "Error: unable to open file %s\n\n", argv[1]);
        return -2;
    }

    int* newline = read_next_line(fin);
    record_t* head = NULL;
    record_t* curr = NULL;
    
    while ( newline != NULL ) {
        float avg = compute_average(newline);
        float sdv = compute_stdev(newline);
        curr = append(curr, avg, sdv);
        if ( head == NULL )
            head = curr;
        free(newline);
        newline = read_next_line(fin);
    }
    fclose(fin);

    // TODO: write the list to the output file
    // Each line of the output file should contain the average and the standard deviation
    // as a comma-separated pair (e.g., "1.23,4.56")
    // Write the list to the output file
    write_list_to_file(head, "output.csv");

    // Free all the memory allocated for the list

    // TODO: free all the memory allocated for the list
    free_list(head);

    return 0;
}