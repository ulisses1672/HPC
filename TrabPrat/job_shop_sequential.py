import sys
from collections import defaultdict

def read_input(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()
    
    num_machines, num_jobs = map(int, lines[0].strip().split())
    jobs = []
    
    for line in lines[1:]:
        operations = list(map(int, line.strip().split()))
        job = [(operations[i], operations[i + 1]) for i in range(0, len(operations), 2)]
        jobs.append(job)
    
    return num_machines, num_jobs, jobs

def schedule_jobs(num_machines, jobs):
    machine_end_time = [0] * num_machines
    schedule = defaultdict(list)
    
    for job_index, job in enumerate(jobs):
        current_time = 0
        for machine_id, duration in job:
            start_time = max(current_time, machine_end_time[machine_id])
            schedule[job_index].append((machine_id, start_time))
            current_time = start_time + duration
            machine_end_time[machine_id] = current_time
    
    return schedule

def write_output(file_path, schedule):
    with open(file_path, 'w') as f:
        f.write("Legend:\n")
        f.write("Each line represents the start times of the operations for a job, along with the machine number.\n")
        f.write("Format: <job_index>: <(machine_id, operation_start_time)> <(machine_id, operation_start_time)> ... <(machine_id, operation_start_time)>\n\n")
        
        for job_index in sorted(schedule.keys()):
            start_times = ' '.join([f"({machine_id}, {start_time})" for machine_id, start_time in schedule[job_index]])
            f.write(f"job{job_index}: {start_times}\n")

def main():
    if len(sys.argv) != 3:
        print("Usage: python job_shop_sequential.py <input_file> <output_file>")
        return
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    num_machines, num_jobs, jobs = read_input(input_file)
    schedule = schedule_jobs(num_machines, jobs)
    write_output(output_file, schedule)

if __name__ == "__main__":
    main()
