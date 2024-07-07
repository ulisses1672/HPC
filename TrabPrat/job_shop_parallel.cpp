#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <map>

struct Operation {
    int machine_id;
    int duration;
};

std::mutex mtx; // Mutex para garantir a exclusão mútua

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

void schedule_jobs_parallel(int num_machines, const std::vector<std::vector<Operation>>& jobs, int num_threads, std::map<int, std::vector<int>>& schedule) {
    std::vector<int> machine_end_time(num_machines, 0);
    std::vector<std::thread> threads;

    int jobs_per_thread = jobs.size() / num_threads;

    for (int i = 0; i < num_threads; ++i) {
        int start = i * jobs_per_thread;
        int end = (i == num_threads - 1) ? jobs.size() : (i + 1) * jobs_per_thread;
        threads.emplace_back(schedule_job_range, start, end, std::cref(jobs), std::ref(machine_end_time), std::ref(schedule));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

void write_output(const std::string& file_path, const std::map<int, std::vector<int>>& schedule) {
    std::ofstream output_file(file_path);
    if (!output_file) {
        std::cerr << "Error: Cannot open output file!" << std::endl;
        exit(1);
    }

    for (const auto& [job_index, times] : schedule) {
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
    schedule_jobs_parallel(num_machines, jobs, num_threads, schedule);

    write_output(output_file, schedule);

    return 0;
}
