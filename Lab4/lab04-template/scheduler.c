#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

// total jobs
int numofjobs = 0;

struct job
{
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets;    // number of tickets for lottery scheduling
    int start;      // start time
    int completion; // completion time
    int wait;       // wait time
    struct job *next;
};

// the workload list
struct job *head = NULL;

void append_to(struct job **head_pointer, int arrival, int length, int tickets)
{
    struct job *new_job = (struct job *)malloc(sizeof(struct job));
    new_job->id = numofjobs++;
    new_job->arrival = arrival;
    new_job->length = length;
    new_job->tickets = tickets;
    new_job->next = NULL;

    if (*head_pointer == NULL)
    {
        *head_pointer = new_job; // if the list is empty then the new job is the head
    }
    else
    {
        struct job *current = *head_pointer;
        while (current->next != NULL)
        {
            current = current->next; // traverse to the end of the list
        }
        current->next = new_job; // append the new job to the end of the list
    }
    return;
}

void read_job_config(const char *filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int tickets = 0;

    char *delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    // TODO, error checking
    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    // TODO: if the file is empty, we should just exit with error
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (line[read - 1] == '\n')
            line[read - 1] = 0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;

        append_to(&head, atoi(arrival), atoi(length), tickets);
    }

    fclose(fp);
    if (line)
        free(line);
}

void policy_SJF()
{
    printf("Execution trace with SJF:\n");
    
    struct job *current = head;
    int time = 0;

    while (1) {
        // Find the job that is ready to run
        struct job *shortest_job = NULL;
        struct job *prev_shortest = NULL;
        struct job *iter = head;

        while (iter != NULL) {
            if (iter->arrival <= time && (shortest_job == NULL || iter->length < shortest_job->length || 
               (iter->length == shortest_job->length && iter->id < shortest_job->id))) {
                shortest_job = iter;
                prev_shortest = NULL;  // Track the previous node for removal if necessary
            }
            iter = iter->next;
        }

        if (shortest_job == NULL) {
            // No job is ready to run; break if all jobs have completed
            if (current == NULL) {
                break; // All jobs completed, exit the loop
            } else {
                // CPU is idle; move time forward to the next job's arrival
                time = (current == NULL) ? INT_MAX : current->arrival;
                continue;
            }
        }

        // If we found a job to run, print its execution details
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, shortest_job->id, shortest_job->arrival, shortest_job->length);
        shortest_job->start = time; // start time
        time += shortest_job->length; // advance time
        shortest_job->completion = time; // completion time
        shortest_job->wait = shortest_job->start - shortest_job->arrival; // wait time

        // Remove the job from the list (optional based on your structure)
        // You can also just set current to NULL if you're maintaining a list of completed jobs.
        if (head == shortest_job) {
            head = shortest_job->next; // Remove from the head
        } else {
            iter = head;
            while (iter != NULL && iter->next != shortest_job) {
                iter = iter->next; // Traverse to the job before shortest_job
            }
            if (iter != NULL) {
                iter->next = shortest_job->next; // Unlink shortest_job from the list
            }
        }
    }


    printf("End of execution with SJF.\n");
}

void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    // TODO: implement STCF policy

    printf("End of execution with STCF.\n");
}

void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    // TODO: implement RR policy

    printf("End of execution with RR.\n");
}

void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    // Leave this here, it will ensure the scheduling behavior remains deterministic
    srand(42);

    // In the following, you'll need to:
    // Figure out which active job to run first
    // Pick the job with the shortest remaining time
    // Considers jobs in order of arrival, so implicitly breaks ties by choosing the job with the lowest ID

    // To achieve consistency with the tests, you are encouraged to choose the winning ticket as follows:
    // int winning_ticket = rand() % total_tickets;
    // And pick the winning job using the linked list approach discussed in class, or equivalent

    printf("End of execution with LT.\n");
}

void policy_FIFO()
{
    printf("Execution trace with FIFO:\n");

    struct job *current = head;
    int time = 0;

    while (current != NULL)
    {
        if (time < current->arrival)
        {
            time = current->arrival;
        }
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, current->id, current->arrival, current->length);
        current->start = time; // start time
        time += current->length;
        current->completion = time;                        // completion time
        current->wait = current->start - current->arrival; // wait time

        current = current->next;
    }
    printf("End of execution with FIFO.\n");
}

void policy_analysis(char *pname)
{
    int total_response_time = 0;
    int total_turnaround_time = 0;
    int total_wait_time = 0;
    struct job *current = head;

    printf("Begin analyzing %s:\n", pname);
    while (current != NULL)
    {
        total_response_time += current->start - current->arrival;
        total_turnaround_time += current->completion - current->arrival;
        total_wait_time += current->wait;
        printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", current->id, current->start - current->arrival, current->completion - current->arrival, current->wait);

        current = current->next;
    }
    printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n", (float)total_response_time / numofjobs, (float)total_turnaround_time / numofjobs, (float)total_wait_time / numofjobs);
    printf("End analyzing %s.\n", pname);
}

int main(int argc, char **argv)
{

    static char usage[] = "usage: %s analysis policy slice trace\n";

    int analysis;
    char *pname;
    char *tname;
    int slice;

    if (argc < 5)
    {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    // if 0, we don't analysis the performance
    analysis = atoi(argv[1]);

    // policy name
    pname = argv[2];

    // time slice, only valid for RR
    slice = atoi(argv[3]);

    // workload trace
    tname = argv[4];

    read_job_config(tname);

    if (strcmp(pname, "FIFO") == 0)
    {
        policy_FIFO();
        if (analysis == 1)
        {
            policy_analysis(pname);
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        policy_SJF();
        if (analysis == 1)
        {
            policy_analysis(pname);
        }    
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "RR") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "LT") == 0)
    {
        // TODO
    }

    exit(0);
}