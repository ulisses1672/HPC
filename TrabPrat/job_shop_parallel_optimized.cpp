/**
 * How to use this script:
 * 
 * 1. Compile the script using the following command:
 *    g++ job_shop_parallel_optimized.cpp -o job_shop_parallel_optimized -std=c++17 -pthread
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
 *    job_shop_parallel_optimized <dataset_name> <output_name> <number_of_threads>
 * 
 *    Example:
 *    job_shop_parallel_bb_optimized input3.txt output.txt 4
 * 
 * 4. The output file (output.txt) will contain the start times of the operations for each job.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <pthread.h>
#include <map>
#include <queue>
#include <limits>
#include <chrono>
#include <atomic>
#include <mutex>

struct Operation {
    int machine_id; // The ID of the machine required for this operation
    int duration; // The time it takes to complete this operation
};

/**
 * Node structure to represent a state in the branch-and-bound search.
 * 
 * This struct captures the current scheduling state of all jobs, including:
 * - The end time of the last operation on each machine.
 * - The end time of the last operation for each job.
 * - The schedule of jobs, which is a list of start times for operations.
 * - The current cost (makespan), representing the total time taken to complete all jobs so far.
 * - The current job index being scheduled.
 * - The current operation index being scheduled within that job.
 * 
 * The comparison operator is defined to allow priority queue to prioritize nodes with lower costs.
 */
struct Node {
    std::vector<int> machine_end_time; // End time of the last operation on each machine
    std::vector<int> job_end_time; // End time of the last operation for each job
    std::vector<std::vector<int>> schedule; // Schedule of jobs (start times of operations)
    int cost; // Current cost (makespan), the total time to complete all jobs
    int job_index; // Current job index being scheduled
    int op_index; // Current operation index being scheduled

    Node(int num_machines, int num_jobs)
        : machine_end_time(num_machines, 0), job_end_time(num_jobs, 0), schedule(num_jobs), cost(0), job_index(0), op_index(0) {}

    bool operator>(const Node& other) const {
        return cost > other.cost; // Used for priority queue to prioritize nodes with lower costs
    }
};

/**
 * Shared variables and synchronization mechanisms.
 * 
 * These variables are used to coordinate the parallel execution of the branch-and-bound algorithm:
 */
std::mutex pq_mutex; // Mutex to ensure mutual exclusion for priority queue
std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq; // Priority queue for Branch and Bound
std::atomic<int> best_cost(std::numeric_limits<int>::max()); // Best cost found
std::mutex best_mutex; // Mutex for best_cost and best_schedule
std::vector<std::vector<int>> best_schedule; // Best schedule found
std::atomic<bool> done(false); // Flag to indicate completion

/**
 * Reads the job and machine data from an input file.
 * 
 * This function reads the number of machines and jobs from the input file and
 * then reads the operations for each job. Each operation specifies the machine
 * ID and the duration required for that operation.
 * 
 * - file_path: The path to the input file.
 * - num_machines: A reference to an integer where the number of machines will be stored.
 * - num_jobs: A reference to an integer where the number of jobs will be stored.
 * - jobs: A reference to a vector of vectors that will store the operations for each job.
 */
void read_input(const std::string& file_path, int& num_machines, int& num_jobs, std::vector<std::vector<Operation>>& jobs) {
    std::ifstream input_file(file_path);
    if (!input_file) {
        std::cerr << "Error: Cannot open input file!" << std::endl;
        exit(1);
    }

    input_file >> num_machines >> num_jobs; // Read the number of machines and jobs
    jobs.resize(num_jobs); // Resize the jobs vector to hold operations for each job
    
    // Read each job's operations
    for (int i = 0; i < num_jobs; ++i) {
        for (int j = 0; j < num_machines; ++j) {
            int machine_id, duration;
            input_file >> machine_id >> duration;
            jobs[i].emplace_back(Operation{machine_id, duration});
        }
    }
}

/**
 * Calculates the lower bound (minimum cost estimate) for the given node.
 * 
 * The lower bound is an estimate of the minimum time required to complete all jobs 
 * from the current state represented by the node. It is used to prune the search space 
 * in the branch-and-bound algorithm.
 * 
 * - node: The current state of the job scheduling.
 * - jobs: A reference to a vector of vectors that contains the operations for each job.
 * 
 * The function calculates the remaining time needed for each job, adds it to the 
 * end time of the last operation for that job, and takes the maximum of these values 
 * to determine the lower bound.
 * 
 * - lower_bound: The current cost (makespan) from the node.
 * - job_remaining_time: The remaining time to complete the current job.
 * 
 * Returns the lower bound estimate.
 */
int calculate_lower_bound(const Node& node, const std::vector<std::vector<Operation>>& jobs) {
    int lower_bound = node.cost; // Start with the current cost
    for (int i = 0; i < jobs.size(); ++i) {
        int job_remaining_time = 0;
        // Calculate the remaining time for each job
        for (int j = (i == node.job_index ? node.op_index : 0); j < jobs[i].size(); ++j) {
            job_remaining_time += jobs[i][j].duration;
        }
        // Update the lower bound to be the maximum of the current lower bound and 
        // the total time required for the job (end time + remaining time)
        lower_bound = std::max(lower_bound, node.job_end_time[i] + job_remaining_time);
    }
    return lower_bound; // Return the lower bound estimate
}

/**
 * Generates child nodes from the current node by scheduling the next operation.
 * 
 * This function creates new nodes representing the state of the job scheduling after 
 * scheduling the next operation for the current job. Each child node corresponds to a 
 * possible next step in the scheduling process.
 * 
 * - node: The current state of the job scheduling.
 * - jobs: A reference to a vector of vectors that contains the operations for each job.
 * 
 * The function checks if there are more operations to schedule for the current job. If so, 
 * it schedules the next operation by updating the machine and job end times, and creates a 
 * new child node with this updated state. The new node is then added to the list of children.
 * 
 * - children: A vector to store the generated child nodes.
 * 
 * Returns a vector of child nodes.
 */
std::vector<Node> generate_children(const Node& node, const std::vector<std::vector<Operation>>& jobs) {
    std::vector<Node> children;
    // Check if there are more operations to schedule for the current job
    if (node.job_index < jobs.size() && node.op_index < jobs[node.job_index].size()) {
        Node child = node; // Create a copy of the current node
        int machine_id = jobs[node.job_index][node.op_index].machine_id;
        int duration = jobs[node.job_index][node.op_index].duration;

        // Calculate the start time of the next operation
        int start_time = std::max(node.machine_end_time[machine_id], node.job_end_time[node.job_index]);
        child.schedule[node.job_index].push_back(start_time); // Schedule the operation start time
        child.machine_end_time[machine_id] = start_time + duration; // Update machine end time
        child.job_end_time[node.job_index] = start_time + duration; // Update job end time
        child.cost = *std::max_element(child.machine_end_time.begin(), child.machine_end_time.end()); // Update cost

        // Move to the next operation or next job
        if (node.op_index + 1 < jobs[node.job_index].size()) {
            child.op_index++;
        } else {
            child.op_index = 0;
            child.job_index++;
        }
        children.push_back(child); // Add the new child node to the list
    }
    return children; // Return the list of generated child nodes
}
/**
 * Worker thread function to process nodes in the priority queue and generate child nodes.
 * 
 * This function is executed by multiple threads to perform the branch-and-bound search in parallel.
 * Each thread repeatedly extracts the best node (lowest cost) from the priority queue, calculates its 
 * lower bound, generates child nodes, and updates the best cost and schedule if a better solution is found.
 * 
 * - arg: A pointer to the vector of jobs, passed as an argument to the thread.
 * 
 * The function performs the following steps:
 * 1. Extracts the best node from the priority queue while ensuring mutual exclusion using pq_mutex.
 * 2. Calculates the lower bound for the extracted node.
 * 3. Prunes the node if its lower bound is greater than or equal to the best cost found so far.
 * 4. Generates child nodes by scheduling the next operation.
 * 5. Updates the best cost and schedule if a complete and better solution is found, ensuring mutual exclusion using best_mutex.
 * 6. Adds the child nodes to the priority queue for further processing.
 * 
 * The function terminates when the priority queue is empty, indicated by setting the done flag.
 */
void* worker_thread(void* arg) {
    const std::vector<std::vector<Operation>>& jobs = *(const std::vector<std::vector<Operation>>*)arg;

    while (!done.load()) {
        Node node(0, 0);
        {
            std::lock_guard<std::mutex> lock(pq_mutex); // Lock the priority queue
            if (pq.empty()) {
                done.store(true); // Mark as done if no more nodes to process
                return nullptr;
            }
            node = pq.top(); // Get the node with the lowest cost
            pq.pop(); // Remove it from the queue
        }

        int lower_bound = calculate_lower_bound(node, jobs);
        if (lower_bound >= best_cost.load()) continue; // Prune nodes with a higher lower bound

        auto children = generate_children(node, jobs);
        for (const auto& child : children) {
            if (child.job_index == jobs.size()) {
                std::lock_guard<std::mutex> lock(best_mutex); // Lock the best cost and schedule
                if (child.cost < best_cost.load()) {
                    best_cost = child.cost; // Update best cost
                    best_schedule = child.schedule; // Update best schedule
                }
            } else {
                std::lock_guard<std::mutex> lock(pq_mutex); // Lock the priority queue
                pq.push(child); // Add child node to the queue
            }
        }
    }
    return nullptr;
}
/**
 * Writes the best schedule to the output file.
 * 
 * This function takes the best schedule found by the algorithm and writes it to the specified output file.
 * Each job's schedule (start times of operations) is written on a separate line.
 * 
 * - file_path: The path to the output file where the schedule will be written.
 * - schedule: A vector of vectors containing the start times of operations for each job.
 * 
 * The function performs the following steps:
 * 1. Opens the output file for writing.
 * 2. Checks if the file was successfully opened; if not, it prints an error message and exits.
 * 3. Iterates over the schedule for each job and writes the start times of its operations to the file.
 * 4. Each job's schedule is written on a separate line, with start times separated by spaces.
 */
void write_output(const std::string& file_path, const std::vector<std::vector<int>>& schedule) {
    std::ofstream output_file(file_path);
    if (!output_file) {
        std::cerr << "Error: Cannot open output file!" << std::endl;
        exit(1);
    }

    // Iterate over each job's schedule
    for (size_t job_index = 0; job_index < schedule.size(); ++job_index) {
        for (size_t i = 0; i < schedule[job_index].size(); ++i) {
            output_file << schedule[job_index][i]; // Write the start time of the operation
            if (i < schedule[job_index].size() - 1) {
                output_file << " "; // Add a space between start times
            }
        }
        output_file << std::endl; // Move to the next line for the next job
    }
}

/**
 * Main function to execute the parallel branch-and-bound job shop scheduling algorithm.
 * 
 * This function performs the following steps:
 * 1. Validates the command-line arguments to ensure the correct usage.
 * 2. Reads the input file to obtain the number of machines, jobs, and their operations.
 * 3. Initializes the root node and pushes it onto the priority queue.
 * 4. Creates and starts the specified number of worker threads to process the nodes in parallel.
 * 5. Waits for all worker threads to complete.
 * 6. Writes the best schedule found to the output file.
 * 7. Outputs the best cost (makespan) found to the console.
 */
int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: job_shop_parallel_bb_optimized <input_file> <output_file> <num_threads>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1]; // Input file path
    std::string output_file = argv[2]; // Output file path
    int num_threads = std::stoi(argv[3]); // Number of threads to use

    int num_machines, num_jobs;
    std::vector<std::vector<Operation>> jobs;
    read_input(input_file, num_machines, num_jobs, jobs); // Read input data

    Node root(num_machines, jobs.size()); // Initialize the root node
    pq.push(root); // Push the root node onto the priority queue

    // Create and start the worker threads
    std::vector<pthread_t> threads(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, worker_thread, &jobs);
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        pthread_join(thread, nullptr);
    }

    write_output(output_file, best_schedule); // Write the best schedule to the output file

    std::cout << "Best cost: " << best_cost.load() << std::endl; // Output the best cost found

    return 0;
}
