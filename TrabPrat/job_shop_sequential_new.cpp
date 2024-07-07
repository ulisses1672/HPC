#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>

struct Operation {
    int machine_id;
    int duration;
};

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
    schedule_jobs(num_machines, jobs, schedule);

    write_output(output_file, schedule);

    return 0;
}
