#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <chrono>

/**
 * How to use this script:
 * 
 * 1. Compile the script using the following command:
 *    g++ job_shop_parallel.cpp -o job_shop_parallel -std=c++17 -pthread
 * 
 * 2. Create an input file with the job and machine data in the following format:
 *    <number_of_machines> <number_of_jobs>
 *    <machine_id> <duration> <machine_id> <duration> ...
 *    (repeat for each job)
 * 
 *    Example input file (input.txt):
 *    3 3
 *    0 3 1 2 2 2
 *    0 2 2 1 1 4
 *    1 4 2 3 0 1
 * 
 * 3. Run the compiled script with the following command:
 *    ./job_shop_parallel input.txt output.txt <number_of_threads>
 * 
 *    Example:
 *    ./job_shop_parallel input.txt output.txt 4
 * 
 * 4. The output file (output.txt) will contain the start times of the operations for each job.
 */

struct Operation {
    int machine_id;
    int duration;
};

// Mutex to ensure mutual exclusion during shared resource access
std::mutex mtx;

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
 * Schedules a range of jobs by assigning start times for their operations.
 * 
 * @param start The starting index of the jobs to schedule.
 * @param end The ending index (exclusive) of the jobs to schedule.
 * @param jobs The list of jobs, each job is a list of operations.
 * @param machine_end_time A vector tracking the end times of the last operation on each machine.
 * @param schedule A map recording the start times of operations for each job.
 */
void schedule_job_range(int start, int end, const std::vector<std::vector<Operation>>& jobs, std::vector<int>& machine_end_time, std::map<int, std::vector<int>>& schedule) {
    for (int job_index = start; job_index < end; ++job_index) {
        int current_time = 0;
        for (const auto& operation : jobs[job_index]) {
            std::lock_guard<std::mutex> lock(mtx); // Exclusão mútua
            int start_time = std::max(current_time, machine_end_time[operation.machine_id]);
            schedule[job_index].push_back(start_time);
            current_time = start_time + operation.duration;
            machine_end_time[operation.machine_id] = current_time;
        }
    }
}

/**
 * Schedules jobs in parallel using multiple threads.
 * 
 * @param num_machines The number of machines.
 * @param jobs The list of jobs, each job is a list of operations.
 * @param num_threads The number of threads to use for parallel processing.
 * @param schedule A map recording the start times of operations for each job.
 */
void schedule_jobs_parallel(int num_machines, const std::vector<std::vector<Operation>>& jobs, int num_threads, std::map<int, std::vector<int>>& schedule) {
    std::vector<int> machine_end_time(num_machines, 0); // Tracks the end times of the last operation on each machine
    std::vector<std::thread> threads; // Vector to hold thread objects

    int jobs_per_thread = jobs.size() / num_threads; // Determine the number of jobs each thread will handle

    for (int i = 0; i < num_threads; ++i) {
        int start = i * jobs_per_thread;
        int end = (i == num_threads - 1) ? jobs.size() : (i + 1) * jobs_per_thread;
        threads.emplace_back(schedule_job_range, start, end, std::cref(jobs), std::ref(machine_end_time), std::ref(schedule));
    }

    for (auto& thread : threads) {
        thread.join(); // Wait for all threads to finish
    }
}

/**
 * Writes the scheduled job start times to an output file.
 * 
 * @param file_path The path to the output file.
 * @param schedule A map recording the start times of operations for each job.
 */
void write_output(const std::string& file_path, const std::map<int, std::vector<int>>& schedule) {
    std::ofstream output_file(file_path);
    if (!output_file) {
        std::cerr << "Error: Cannot open output file!" << std::endl;
        exit(1);
    }

    for (auto it = schedule.begin(); it != schedule.end(); ++it) {
        int job_index = it->first;
        const std::vector<int>& times = it->second;
        for (size_t i = 0; i < times.size(); ++i) {
            output_file << times[i];
            if (i < times.size() - 1) {
                output_file << " ";
            }
        }
        output_file << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: job_shop_parallel <input_file> <output_file> <num_threads>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];
    int num_threads = std::stoi(argv[3]);

    int num_machines, num_jobs;
    std::vector<std::vector<Operation>> jobs;
    read_input(input_file, num_machines, num_jobs, jobs);

    std::map<int, std::vector<int>> schedule;

    // Measure the execution time for the parallel version
    auto start_time = std::chrono::high_resolution_clock::now();
    schedule_jobs_parallel(num_machines, jobs, num_threads, schedule);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> parallel_execution_time = end_time - start_time;

    write_output(output_file, schedule);

    std::cout << "Parallel execution time: " << parallel_execution_time.count() << " seconds" << std::endl;

    return 0;
}