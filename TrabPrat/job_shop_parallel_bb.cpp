#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <limits>
#include <chrono>

struct Operation {
    int machine_id;
    int duration;
};

struct Node {
    std::vector<int> machine_end_time; // End time of last operation on each machine
    std::vector<int> job_end_time; // End time of last operation for each job
    std::vector<std::vector<int>> schedule; // Schedule of jobs
    int cost; // Current cost (makespan)
    int job_index; // Current job index being scheduled
    int op_index; // Current operation index being scheduled

    Node(int num_machines, int num_jobs) 
        : machine_end_time(num_machines, 0), job_end_time(num_jobs, 0), schedule(num_jobs), cost(0), job_index(0), op_index(0) {}

    Node(const Node& other) = default;
    Node(Node&& other) = default;
    Node& operator=(const Node& other) = default; // Adding the copy assignment operator

    bool operator>(const Node& other) const {
        return cost > other.cost; // Used for priority queue
    }
};

// Mutex to ensure mutual exclusion
std::mutex mtx;

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

int calculate_lower_bound(const Node& node, const std::vector<std::vector<Operation>>& jobs) {
    // Calculate the lower bound based on the remaining operations
    int lower_bound = node.cost;
    for (int i = node.job_index; i < jobs.size(); ++i) {
        int job_remaining_time = 0;
        for (int j = node.op_index; j < jobs[i].size(); ++j) {
            job_remaining_time += jobs[i][j].duration;
        }
        lower_bound += job_remaining_time;
    }
    return lower_bound;
}

std::vector<Node> generate_children(const Node& node, const std::vector<std::vector<Operation>>& jobs) {
    std::vector<Node> children;
    if (node.job_index < jobs.size() && node.op_index < jobs[node.job_index].size()) {
        Node child = node;
        int machine_id = jobs[node.job_index][node.op_index].machine_id;
        int duration = jobs[node.job_index][node.op_index].duration;

        int start_time = std::max(node.machine_end_time[machine_id], node.job_end_time[node.job_index]);
        child.schedule[node.job_index].push_back(start_time);
        child.machine_end_time[machine_id] = start_time + duration;
        child.job_end_time[node.job_index] = start_time + duration;
        child.cost = *std::max_element(child.machine_end_time.begin(), child.machine_end_time.end());

        if (node.op_index + 1 < jobs[node.job_index].size()) {
            child.op_index++;
        } else {
            child.op_index = 0;
            child.job_index++;
        }
        children.push_back(child);
    }
    return children;
}

void branch_and_bound_parallel(int num_machines, const std::vector<std::vector<Operation>>& jobs, int num_threads, int& best_cost, std::vector<std::vector<int>>& best_schedule) {
    auto cmp = [](const Node& a, const Node& b) { return a > b; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
    Node root(num_machines, jobs.size());
    pq.push(root);

    std::vector<std::thread> threads;

    auto worker = [&]() {
        while (true) {
            Node node(num_machines, jobs.size());
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (pq.empty()) return;
                node = pq.top();
                pq.pop();
            }

            int lower_bound = calculate_lower_bound(node, jobs);
            if (lower_bound >= best_cost) continue;

            auto children = generate_children(node, jobs);
            for (const auto& child : children) {
                if (child.job_index == jobs.size()) {
                    std::lock_guard<std::mutex> lock(mtx);
                    if (child.cost < best_cost) {
                        best_cost = child.cost;
                        best_schedule = child.schedule;
                    }
                } else {
                    std::lock_guard<std::mutex> lock(mtx);
                    pq.push(child);
                }
            }
        }
    };

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& thread : threads) {
        thread.join();
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

    int best_cost = std::numeric_limits<int>::max();
    std::vector<std::vector<int>> best_schedule;

    // Measure the execution time for the parallel version
    auto start_time = std::chrono::high_resolution_clock::now();
    branch_and_bound_parallel(num_machines, jobs, num_threads, best_cost, best_schedule);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> parallel_execution_time = end_time - start_time;

    write_output(output_file, best_schedule);

    std::cout << "Best cost: " << best_cost << std::endl;
    std::cout << "Parallel execution time: " << parallel_execution_time.count() << " seconds" << std::endl;

    return 0;
}
