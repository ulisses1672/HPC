#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_MACHINES 10
#define NUM_JOBS 1000
#define MAX_OPERATIONS_PER_JOB 5
#define MAX_DURATION 10

/**
 * How to use this script:
 * 
 * 1. Compile the script using the following command:
 *    gcc generate_large_input.cpp -o generate_large_input
 * 
 * 2. Run the compiled script with the following command:
 *    ./generate_large_input output.txt
 * 
 *    This will generate a file named 'output.txt' with the input data for the Job-Shop Scheduling problem.
 * 
 *    Example:
 *    ./generate_large_input input.txt
 * 
 *    This will create an 'input.txt' file with randomly generated job-shop scheduling data.
 *
 * Generates input data for the Job-Shop Scheduling problem and writes it to a file.
 *
 * @param file_name The name of the output file.
 * @param num_machines The number of available machines.
 * @param num_jobs The number of jobs to be generated.
 * @param max_operations_per_job The maximum number of operations per job.
 * @param max_duration The maximum duration of an operation.
 */
void generate_input(const char *file_name, int num_machines, int num_jobs, int max_operations_per_job, int max_duration) {
    FILE *file = fopen(file_name, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", file_name);
        exit(1);
    }

    // Seed for random number generation
    srand(time(NULL));

    // Write the number of machines and jobs to the file
    fprintf(file, "%d %d\n", num_machines, num_jobs);

    // Generate data for each job
    for (int i = 0; i < num_jobs; i++) {
        for (int j = 0; j < max_operations_per_job; j++) {
            int machine_id = rand() % num_machines;
            int duration = (rand() % max_duration) + 1;
            fprintf(file, "%d %d", machine_id, duration);
            if (j < max_operations_per_job - 1) {
                fprintf(file, " ");
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

/**
 * Main function that reads the output file name from the command line
 * and calls the data generation function.
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return 1;
    }

    const char *output_file = argv[1];
    generate_input(output_file, NUM_MACHINES, NUM_JOBS, MAX_OPERATIONS_PER_JOB, MAX_DURATION);

    printf("Input generated and saved to %s\n", output_file);
    return 0;
}
