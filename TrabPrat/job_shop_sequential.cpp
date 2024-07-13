
/**
 * How to use this script:
 * 
 * 1. Compile the script using the following command:
 *    g++ job_shop_sequential.cpp -o job_shop_sequential -std=c++17
 * 
 
 * 2. Use the script generate_input_data.cpp to generate a input dataset
 *    
 *    Or Instead you could create an input file with the job and machine data 
 *    in the following format:
 *    
 *    <number_of_machines> <number_of_jobs>
 *    <machine_id> <duration> <machine_id> <duration> ...
 *    (repeat for each job)
 *    
 *    Example input file (input3.txt):
 *    3 3
 *    0 3 1 2 2 2
 *    0 2 2 1 1 4
 *    1 4 2 3 0 1
 * 
 * 3. Run the compiled script with the following command:
 *    job_shop_sequential input.txt output.txt
 * 
 * 4. The output file (output.txt) will contain the start times of the operations for each job.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <chrono>

struct Operation {
    int machine_id;
    int duration;
};

/**
 * Reads input data from a file.
 * 
 * @param file_path The path to the input file.
 * @param num_machines The number of machines (output parameter).
 * @param num_jobs The number of jobs (output parameter).
 * @param jobs The list of jobs, each job is a list of operations (output parameter).
 */
void read_input(const std::string& file_path, int& num_machines, int& num_jobs, std::vector<std::vector<Operation>>& jobs) {
    std::ifstream input_file(file_path);
    if (!input_file) {
        std::cerr << "Error: Cannot open input file!" << std::endl;
        exit(1);
    }

    input_file >> num_machines >> num_jobs;
    jobs.resize(num_jobs);

    for (int i = 0; i < num_jobs; ++i) {
        for (int j = 0; j < num_machines; ++j) {
            int machine_id, duration;
            input_file >> machine_id >> duration;
            jobs[i].emplace_back(Operation{machine_id, duration});
        }
    }
}

/**
 * Schedules jobs by assigning start times for their operations.
 * 
 * @param num_machines The number of machines.
 * @param jobs The list of jobs, each job is a list of operations.
 * @param schedule A vector recording the start times of operations for each job.
 */
void schedule_jobs(int num_machines, const std::vector<std::vector<Operation>>& jobs, std::vector<std::vector<int>>& schedule) {
    std::vector<int> machine_end_time(num_machines, 0);
    schedule.resize(jobs.size());

    for (size_t job_index = 0; job_index < jobs.size(); ++job_index) {
        int current_time = 0;
        for (const auto& operation : jobs[job_index]) {
            int start_time = std::max(current_time, machine_end_time[operation.machine_id]);
            schedule[job_index].push_back(start_time);
            current_time = start_time + operation.duration;
            machine_end_time[operation.machine_id] = current_time;
        }
    }
}

/**
 * Writes the scheduled job start times to an output file.
 * 
 * @param file_path The path to the output file.
 * @param schedule A vector recording the start times of operations for each job.
 */
void write_output(const std::string& file_path, const std::vector<std::vector<int>>& schedule) {
    std::ofstream output_file(file_path);
    if (!output_file) {
        std::cerr << "Error: Cannot open output file!" << std::endl;
        exit(1);
    }

    for (size_t job_index = 0; job_index < schedule.size(); ++job_index) {
        for (size_t i = 0; i < schedule[job_index].size(); ++i) {
            output_file << schedule[job_index][i];
            if (i < schedule[job_index].size() - 1) {
                output_file << " ";
            }
        }
        output_file << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: job_shop_sequential <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    int num_machines, num_jobs;
    std::vector<std::vector<Operation>> jobs;
    read_input(input_file, num_machines, num_jobs, jobs);

    std::vector<std::vector<int>> schedule;

    // Measure the execution time for the sequential version
    auto start_time = std::chrono::high_resolution_clock::now();
    schedule_jobs(num_machines, jobs, schedule);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> sequential_execution_time = end_time - start_time;

    write_output(output_file, schedule);

    std::cout << "Sequential execution time: " << sequential_execution_time.count() << " seconds" << std::endl;

    return 0;
}
