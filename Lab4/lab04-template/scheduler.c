#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

// total jobs
int numofjobs = 0;

struct job
{
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int initial_length; // initial length of the job
    int tickets;        // number of tickets for lottery scheduling
    int start;          // start time
    int completion;     // completion time
    int wait;           // wait time
    struct job *next;
    bool jobDone;
};

// the workload list
struct job *head = NULL;

void append_to(struct job **head_pointer, int arrival, int length, int tickets)
{
    struct job *new_job = (struct job *)malloc(sizeof(struct job));
    new_job->completion = 0;
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

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

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

    int time = 0;
    struct job *remaining = head;
    int jobs_remaining = numofjobs;

    while (jobs_remaining > 0)
    {
        struct job *shortest_job = NULL;
        struct job *current = remaining;

        while (current != NULL)
        {
            if (current->arrival <= time && !current->jobDone &&
                (shortest_job == NULL || current->length < shortest_job->length ||
                 (current->length == shortest_job->length && current->id < shortest_job->id)))
            {
                shortest_job = current;
            }
            current = current->next;
        }

        if (shortest_job == NULL)
        {
            int next_arrival = INT_MAX;
            current = remaining;
            while (current != NULL)
            {
                if (!current->jobDone && current->arrival < next_arrival && current->arrival > time)
                {
                    next_arrival = current->arrival;
                }
                current = current->next;
            }
            time = next_arrival;
            continue;
        }

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
               time, shortest_job->id, shortest_job->arrival, shortest_job->length);

        shortest_job->start = time;
        time += shortest_job->length;
        shortest_job->completion = time;
        shortest_job->wait = shortest_job->start - shortest_job->arrival;
        shortest_job->jobDone = true;
        jobs_remaining--;
    }

    printf("End of execution with SJF.\n");
}

void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    int time = 0;
    struct job *remaining = head;
    int jobs_remaining = numofjobs;

    struct job *current = head;
    while (current != NULL)
    {
        current->jobDone = false;
        current->start = -1;
        current->initial_length = current->length;
        current = current->next;
    }

    while (jobs_remaining > 0)
    {
        struct job *shortest_job = NULL;
        current = remaining;

        while (current != NULL)
        {
            if (current->arrival <= time && !current->jobDone &&
                (shortest_job == NULL || current->length < shortest_job->length ||
                 (current->length == shortest_job->length && current->id < shortest_job->id)))
            {
                shortest_job = current;
            }
            current = current->next;
        }

        if (shortest_job == NULL)
        {
            int next_arrival = INT_MAX;
            current = remaining;
            while (current != NULL)
            {
                if (!current->jobDone && current->arrival < next_arrival && current->arrival > time)
                {
                    next_arrival = current->arrival;
                }
                current = current->next;
            }
            time = next_arrival;
            continue;
        }

        int run_time = shortest_job->length;
        current = remaining;
        while (current != NULL)
        {
            if (!current->jobDone && current->arrival > time && current->arrival < time + run_time)
            {
                run_time = current->arrival - time;
            }
            current = current->next;
        }

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
               time, shortest_job->id, shortest_job->arrival, run_time);

        if (shortest_job->start == -1)
        {
            shortest_job->start = time;
        }

        time += run_time;
        shortest_job->length -= run_time;

        if (shortest_job->length == 0)
        {
            shortest_job->completion = time;
            shortest_job->jobDone = true;
            jobs_remaining--;
        }
    }

    printf("End of execution with STCF.\n");

    current = head;
    while (current != NULL)
    {
        current->wait = current->completion - current->arrival - current->initial_length;
        current = current->next;
    }
}

void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    int time = 0;
    struct job *ready_queue[100];
    int front = 0;
    int rear = 0;
    int active_jobs = 0;

    struct job *current = head;
    while (current != NULL)
    {
        current->jobDone = false;
        current->start = -1;
        current->initial_length = current->length;
        current = current->next;
    }

    current = head;
    while (current != NULL)
    {
        if (current->arrival == 0)
        {
            ready_queue[rear] = current;
            rear = (rear + 1) % 100;
            active_jobs++;
        }
        current = current->next;
    }

    while (active_jobs > 0)
    {
        struct job *current_job = ready_queue[front];
        front = (front + 1) % 100;
        active_jobs--;

        if (current_job->start == -1)
        {
            current_job->start = time;
        }

        int run_time = min(slice, current_job->length);

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
               time, current_job->id, current_job->arrival, run_time);

        time += run_time;
        current_job->length -= run_time;

        current = head;
        while (current != NULL)
        {
            if (!current->jobDone && current->arrival <= time && current->arrival > (time - run_time))
            {
                ready_queue[rear] = current;
                rear = (rear + 1) % 100;
                active_jobs++;
            }
            current = current->next;
        }

        if (current_job->length > 0)
        {
            ready_queue[rear] = current_job;
            rear = (rear + 1) % 100;
            active_jobs++;
        }
        else
        {
            current_job->completion = time;
            current_job->jobDone = true;
        }

        if (active_jobs == 0)
        {
            bool continue_execution = false;
            int next_arrival = INT_MAX;
            current = head;
            while (current != NULL)
            {
                if (!current->jobDone && current->arrival > time && current->arrival < next_arrival)
                {
                    continue_execution = true;
                    next_arrival = current->arrival;
                }
                current = current->next;
            }
            if (continue_execution)
            {
                time = next_arrival;
                current = head;
                while (current != NULL)
                {
                    if (!current->jobDone && current->arrival == next_arrival)
                    {
                        ready_queue[rear] = current;
                        rear = (rear + 1) % 100;
                        active_jobs++;
                    }
                    current = current->next;
                }
            }
        }
    }

    printf("End of execution with RR.\n");

    current = head;
    while (current != NULL)
    {
        current->wait = current->completion - current->arrival - current->initial_length;
        current = current->next;
    }
}

void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    srand(42);

    struct job *current;
    int time = 0;
    int total_tickets = 0;
    int active_jobs = 0;

    current = head;
    while (current != NULL)
    {
        current->jobDone = false;
        current->start = -1;
        current->initial_length = current->length;
        current = current->next;
    }

    while (1)
    {
        active_jobs = 0;
        total_tickets = 0;
        current = head;
        while (current != NULL)
        {
            if (!current->jobDone && current->arrival <= time)
            {
                active_jobs++;
                total_tickets += current->tickets;
            }
            current = current->next;
        }

        if (active_jobs == 0)
        {
            int next_arrival = INT_MAX;
            current = head;
            while (current != NULL)
            {
                if (!current->jobDone && current->arrival > time && current->arrival < next_arrival)
                {
                    next_arrival = current->arrival;
                }
                current = current->next;
            }
            if (next_arrival == INT_MAX)
                break;
            time = next_arrival;
            continue;
        }

        int winning_ticket = rand() % total_tickets;
        int ticket_counter = 0;
        struct job *winner = NULL;
        current = head;

        while (current != NULL)
        {
            if (!current->jobDone && current->arrival <= time)
            {
                ticket_counter += current->tickets;
                if (ticket_counter > winning_ticket)
                {
                    winner = current;
                    break;
                }
            }
            current = current->next;
        }

        if (winner == NULL)
            continue;

        int run_ticks = slice;
        if (winner->length < run_ticks)
        {
            run_ticks = winner->length;
        }

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", time, winner->id, winner->arrival, run_ticks);

        if (winner->start == -1)
        {
            winner->start = time;
        }

        time += run_ticks;
        winner->length -= run_ticks;

        if (winner->length == 0)
        {
            winner->completion = time;
            winner->jobDone = true;
        }
    }

    printf("End of execution with LT.\n");

    current = head;
    while (current != NULL)
    {
        current->wait = current->completion - current->arrival - current->initial_length;
        current = current->next;
    }
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
        policy_STCF();
        if (analysis == 1)
        {
            policy_analysis(pname);
        }
    }
    else if (strcmp(pname, "RR") == 0)
    {
        policy_RR(slice);
        if (analysis == 1)
        {
            policy_analysis(pname);
        }
    }
    else if (strcmp(pname, "LT") == 0)
    {
        policy_LT(slice);
        if (analysis == 1)
        {
            policy_analysis(pname);
        }
    }

    exit(0);
}