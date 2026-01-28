#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <pthread.h>
#include <map>
#include <chrono>

// Struct to store the operations of each job
struct Operation {
    int machine_id;
    int duration;
};

// Mutex for ensuring mutual exclusion
pthread_mutex_t mtx;

// Struct to pass arguments to thread functions
struct ThreadArgs {
    int start;
    int end;
    const std::vector<std::vector<Operation>>* jobs;
    std::vector<int>* machine_end_time;
    std::map<int, std::vector<int>>* schedule;
};

// Function to read input data from file
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

// Function to schedule a range of jobs
void* schedule_job_range(void* args) {
    ThreadArgs* thread_args = static_cast<ThreadArgs*>(args);
    int start = thread_args->start;
    int end = thread_args->end;
    const std::vector<std::vector<Operation>>& jobs = *thread_args->jobs;
    std::vector<int>& machine_end_time = *thread_args->machine_end_time;
    std::map<int, std::vector<int>>& schedule = *thread_args->schedule;

    for (int job_index = start; job_index < end; ++job_index) {
        int current_time = 0;
        for (const auto& operation : jobs[job_index]) {
            pthread_mutex_lock(&mtx); // Ensure mutual exclusion
            int start_time = std::max(current_time, machine_end_time[operation.machine_id]);
            schedule[job_index].push_back(start_time);
            current_time = start_time + operation.duration;
            machine_end_time[operation.machine_id] = current_time;
            pthread_mutex_unlock(&mtx); // Release the mutex
        }
    }

    return nullptr;
}

// Function to schedule jobs in parallel using multiple threads
void schedule_jobs_parallel(int num_machines, const std::vector<std::vector<Operation>>& jobs, int num_threads, std::map<int, std::vector<int>>& schedule) {
    std::vector<int> machine_end_time(num_machines, 0); // Tracks the end times of the last operation on each machine
    std::vector<pthread_t> threads(num_threads); // Vector to hold thread objects
    std::vector<ThreadArgs> thread_args(num_threads); // Vector to hold thread arguments

    int jobs_per_thread = jobs.size() / num_threads; // Determine the number of jobs each thread will handle

    for (int i = 0; i < num_threads; ++i) {
        int start = i * jobs_per_thread;
        int end = (i == num_threads - 1) ? jobs.size() : (i + 1) * jobs_per_thread;
        thread_args[i] = {start, end, &jobs, &machine_end_time, &schedule};
        pthread_create(&threads[i], nullptr, schedule_job_range, &thread_args[i]);
    }

    for (auto& thread : threads) {
        pthread_join(thread, nullptr); // Wait for all threads to finish
    }
}

// Function to write the scheduled job start times to an output file
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

    // Initialize mutex
    pthread_mutex_init(&mtx, nullptr);

    // Measure the execution time for the parallel version
    auto start_time = std::chrono::high_resolution_clock::now();
    schedule_jobs_parallel(num_machines, jobs, num_threads, schedule);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> parallel_execution_time = end_time - start_time;

    write_output(output_file, schedule);

    // Destroy mutex
    pthread_mutex_destroy(&mtx);

    std::cout << "Parallel execution time: " << parallel_execution_time.count() << " seconds" << std::endl;

    return 0;
}
