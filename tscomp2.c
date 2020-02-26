/* External definitions for time-shared computer model. */

#include "simlib.h"               /* Required for use of simlib.c. */
#include <time.h>

#define EVENT_ARRIVAL          1  /* Event type for arrival of job to CPU. */
#define EVENT_END_CPU_RUN      2  /* Event type for end of a CPU run. */
#define EVENT_END_SIMULATION   3  /* Event type for end of the simulation. */
#define LIST_QUEUE             1  /* List number for CPU queue. */
#define LIST_CPU               2  /* List number for CPU. */
#define SAMPST_RESPONSE_TIMES  1  /* sampst variable for response times. */

#define EVENT_ARRIVAL_0        0  /* Event type for arrival of job to two CPU. */
#define EVENT_END_CPU_RUN_1    1  /* Event type for end of a first CPU run. */
#define EVENT_END_CPU_RUN_2    2  /* Event type for end of a second CPU run. */
#define LIST_CPU_1             1  /* List number for CPU. */
#define LIST_CPU_2             2  /* List number for CPU. */
#define LIST_QUEUE_1           11  /* List number for CPU queue. */
#define LIST_QUEUE_2           12  /* List number for CPU queue. */
#define SAMPST_RESPONSE_TIMES_1  1  /* sampst variable for response times for first CPU. */
#define SAMPST_RESPONSE_TIMES_2  2  /* sampst variable for response times for second CPU. */

#define STREAM_THINK           1  /* Random-number stream for think times. */
#define STREAM_SERVICE         2  /* Random-number stream for service times. */

/* Declare non-simlib global variables. */

int   min_terms, max_terms, incr_terms, num_terms, num_responses,
      num_responses_required, term;
float mean_think, mean_service, quantum, swap;
double random_value;
FILE  *infile, *outfile;

/* Declare non-simlib functions. */

void arrive(void);
void start_CPU_run(int id);
void end_CPU_run(int id);
void report(void);


int main()  /* Main function. */
{
    /* Set random seed. */
    srand ( time ( NULL));
    
    /* Open input and output files. */

    infile  = fopen("tscomp.in",  "r");
    outfile = fopen("tscomp_2.out", "w");

    /* Read input parameters. */

    fscanf(infile, "%d %d %d %d %f %f %f %f",
           &min_terms, &max_terms, &incr_terms, &num_responses_required,
           &mean_think, &mean_service, &quantum, &swap);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Time-shared computer model\n\n");
    fprintf(outfile, "Number of terminals%9d to%4d by %4d\n\n",
            min_terms, max_terms, incr_terms);
    fprintf(outfile, "Mean think time  %11.3f seconds\n\n", mean_think);
    fprintf(outfile, "Mean service time%11.3f seconds\n\n", mean_service);
    fprintf(outfile, "Quantum          %11.3f seconds\n\n", quantum);
    fprintf(outfile, "Swap time        %11.3f seconds\n\n", swap);
    fprintf(outfile, "Number of jobs processed%12d\n\n\n",
            num_responses_required);
    // fprintf(outfile, "Number of      Average         Average");
    // fprintf(outfile, "       Utilization\n");
    // fprintf(outfile, "terminals   response time  number in queue     of CPU");

    fprintf(outfile, "+-------+----------------+----------------+----------------+----------------+----------------+----------------+\n");
    fprintf(outfile, "| numbe | Average respon | Average respon | Average number | Average number | Utilization of | Utilization of |\n");
    fprintf(outfile, "| r of  | se time CPU 1  | se time CPU 2  | in queue CPU 1 | in queue CPU 2 | CPU 1          | CPU 2          |\n");
    fprintf(outfile, "| termi |                |                |                |                |                |                |\n");
    fprintf(outfile, "| nals  |                |                |                |                |                |                |\n");
    fprintf(outfile, "+-------+----------------+----------------+----------------+----------------+----------------+----------------+\n");
    

    /* Run the simulation varying the number of terminals. */

    for (num_terms = min_terms; num_terms <= max_terms;
         num_terms += incr_terms) {

        /* Initialize simlib */

        init_simlib();

        /* Set maxatr = max(maximum number of attributes per record, 4) */

        maxatr = 4;  /* NEVER SET maxatr TO BE SMALLER THAN 4. */

        /* Initialize the non-simlib statistical counter. */

        num_responses = 0;

        /* Schedule the first arrival to the CPU from each terminal. */

        for (term = 1; term <= num_terms; ++term)
            event_schedule(expon(mean_think, STREAM_THINK), EVENT_ARRIVAL_0);

        /* Run the simulation until it terminates after an end-simulation event
           (type EVENT_END_SIMULATION) occurs. */

        do {

            /* Determine the next event. */

            timing();

            /* Invoke the appropriate event function. */

            switch (next_event_type) {
                case EVENT_ARRIVAL_0:
                    arrive();
                    break;
                case EVENT_END_CPU_RUN_1:
                    end_CPU_run(EVENT_END_CPU_RUN_1);
                    break;
                case EVENT_END_CPU_RUN_2:
                    end_CPU_run(EVENT_END_CPU_RUN_2);
                    break;
                case EVENT_END_SIMULATION:
                    report();
                    break;
            }

        /* If the event just executed was not the end-simulation event (type
           EVENT_END_SIMULATION), continue simulating.  Otherwise, end the
           simulation. */

        } while (next_event_type != EVENT_END_SIMULATION);
    }

    fclose(infile);
    fclose(outfile);

    return 0;
}


void arrive(void)  /* Event function for arrival of job at CPU after think
                      time. */
{

    /* Place the arriving job at the end of the CPU queue.
       Note that the following attributes are stored for each job record:
            1. Time of arrival to the computer.
            2. The (remaining) CPU service time required (here equal to the
               total service time since the job is just arriving). */

    transfer[1] = sim_time;
    transfer[2] = expon(mean_service, STREAM_SERVICE);
    // list_file(LAST, LIST_QUEUE);

    /* Allocate job randomly for two CPU */
    random_value = (double)rand()/RAND_MAX;
    if (random_value < 0.5)
      list_file(LAST, LIST_QUEUE_1);
    else
      list_file(LAST, LIST_QUEUE_2);

    /* If the CPU is idle, start a CPU run. */
    if (list_size[LIST_CPU_1] == 0 && list_size[LIST_QUEUE_1] > 0) {
        start_CPU_run(LIST_CPU_1);
    } else if (list_size[LIST_CPU_2] == 0 && list_size[LIST_QUEUE_2] > 0) {
        start_CPU_run(LIST_CPU_2);
    }


    // if (list_size[LIST_CPU] == 0)
    //     start_CPU_run(0);
}


void start_CPU_run(int id)  /* Non-event function to start a CPU run of a job. */
{
    float run_time;

    /* Remove the first job from the queue. */
    /*  id = {
          1 == EVENT_END_CPU_RUN_1 == LIST_CPU_1,
          2 == EVENT_END_CPU_RUN_2 == LIST_CPU_2
        }
        [11] = LIST_QUEUE_1 = Queue of CPU 1
        [12] = LIST_QUEUE_2 = Queue of CPU 2
    */

    list_remove(FIRST, id + 10);

    /* Determine the CPU time for this pass, including the swap time. */

    if (quantum < transfer[2])
        run_time = quantum + swap;
    else
        run_time = transfer[2] + swap;

    /* Decrement remaining CPU time by a full quantum.  (If less than a full
       quantum is needed, this attribute becomes negative.  This indicates that
       the job, after exiting the CPU for the current pass, will be done and is
       to be sent back to its terminal.) */

    transfer[2] -= quantum;

    /* Place the job into the CPU. */
    /*  id = {
          1 == EVENT_END_CPU_RUN_1 == LIST_CPU_1,
          2 == EVENT_END_CPU_RUN_2 == LIST_CPU_2
        }
        [11] = LIST_QUEUE_1 = Queue of CPU 1
        [12] = LIST_QUEUE_2 = Queue of CPU 2
    */

    list_file(FIRST, id);

    /* Schedule the end of the CPU run. */

    event_schedule(sim_time + run_time, id);
}


void end_CPU_run(int id)  /* Event function to end a CPU run of a job. */
{
    /* Remove the job from the CPU. */

    list_remove(FIRST, id);

    /* Check to see whether this job requires more CPU time. */

    if (transfer[2] > 0.0) {

        /* This job requires more CPU time, so place it at the end of the queue
           and start the first job in the queue. */

        list_file(LAST, id + 10);

        /* If the CPU is idle, start a CPU run. */
        if (list_size[LIST_CPU_1] == 0 && list_size[LIST_QUEUE_1] > 0) {
            start_CPU_run(LIST_CPU_1);
        } else if (list_size[LIST_CPU_2] == 0 && list_size[LIST_QUEUE_2] > 0) {
            start_CPU_run(LIST_CPU_2);
        }
    }

    else {

        /* This job is finished, so collect response-time statistics and send it
           back to its terminal, i.e., schedule another arrival from the same
           terminal. */

        /*
          id = {
            1 == EVENT_END_CPU_RUN_1 == LIST_CPU_1 == SAMPST_RESPONSE_TIMES_1,
            2 == EVENT_END_CPU_RUN_2 == LIST_CPU_2 == SAMPST_RESPONSE_TIMES_2
          }
        */

        sampst(sim_time - transfer[1], id);

        event_schedule(sim_time + expon(mean_think, STREAM_THINK),
                       EVENT_ARRIVAL_0);

        /* Increment the number of completed jobs. */

        ++num_responses;

        /* Check to see whether enough jobs are done. */

        if (num_responses >= num_responses_required)

            /* Enough jobs are done, so schedule the end of the simulation
               immediately (forcing it to the head of the event list). */

            event_schedule(sim_time, EVENT_END_SIMULATION);

        else

            /* Not enough jobs are done; if the queue is not empty, start
               another job. */

            if (list_size[LIST_QUEUE_1] > 0) {
                start_CPU_run(LIST_CPU_1);
            } else if (list_size[LIST_QUEUE_2] > 0) {
                start_CPU_run(LIST_CPU_2);
            }
    }
}


void report(void)  /* Report generator function. */
{

  /* Report example
        +-------+----------------+----------------+----------------+----------------+----------------+----------------+
        | nbTrm | AvrRspTimeCPU1 | AvrRspTimeCPU2 | AvrNbQueueCPU1 | AvrNbQueueCPU2 |   UtilOfCPU1   |   UtilOfCPU2   |
        +-------+----------------+----------------+----------------+----------------+----------------+----------------+
        | 12345 | 1234567890.123 | 1234567890.123 | 1234567890.123 | 1234567890.123 | 1234567890.123 | 1234567890.123 |
        +-------+----------------+----------------+----------------+----------------+----------------+----------------+
    */
    /* Get and write out estimates of desired measures of performance. */
    fprintf(outfile, "| %5d | %*.3f | %*.3f | %*.3f | %*.3f | %*.3f | %*.3f |\n", 
        num_terms, 14, sampst(0.0, -SAMPST_RESPONSE_TIMES_1), 14, sampst(0.0, -SAMPST_RESPONSE_TIMES_2),
        14, filest(LIST_QUEUE_1),14, filest(LIST_QUEUE_2),
        14, filest(LIST_CPU_1), 14, filest(LIST_CPU_2));
    fprintf(outfile, "+-------+----------------+----------------+----------------+----------------+----------------+----------------+\n");

}

