import sys
import threading
import time
import json
from collections import defaultdict

def read_input(file_path):
    """
    Lê os dados de entrada a partir de um arquivo JSON.

    Parâmetros:
    file_path (str): Caminho do arquivo JSON de entrada.

    Retorna:
    num_machines (int): Número de máquinas.
    num_jobs (int): Número de trabalhos.
    jobs (list): Lista de trabalhos, onde cada trabalho é uma lista de operações (máquina, duração).
    """
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
        
        num_machines = data['num_machines']
        num_jobs = data['num_jobs']
        jobs = [tuple(map(tuple, job)) for job in data['jobs']]
        
        return num_machines, num_jobs, jobs
    except FileNotFoundError:
        print(f"Error: File '{file_path}' not found.")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error reading JSON file '{file_path}': {e}")
        sys.exit(1)

def schedule_jobs_parallel(num_machines, jobs, num_threads):
    """
    Agenda os trabalhos de forma paralela utilizando múltiplas threads.

    Parâmetros:
    num_machines (int): Número de máquinas.
    jobs (list): Lista de trabalhos.
    num_threads (int): Número de threads a serem usadas para o processamento.

    Retorna:
    schedule (defaultdict): Dicionário contendo o cronograma das operações para cada trabalho.
    """
    machine_end_time = [0] * num_machines  # Mantém o tempo de término para cada máquina
    schedule = defaultdict(list)  # Armazena o cronograma de cada trabalho
    lock = threading.Lock()  # Lock para garantir exclusão mútua ao acessar machine_end_time
    
    # Particionamento (Partitioning): Dividindo os trabalhos entre as threads
    def schedule_job_range(start, end):
        """
        Agenda um intervalo de trabalhos.

        Parâmetros:
        start (int): Índice inicial dos trabalhos a serem agendados.
        end (int): Índice final (exclusivo) dos trabalhos a serem agendados.
        """
        nonlocal machine_end_time
        for job_index in range(start, end):
            job = jobs[job_index]
            current_time = 0
            for machine_id, duration in job:
                # Comunicação (Communication): Utilização de locks para garantir acesso seguro aos tempos das máquinas
                with lock:
                    start_time = max(current_time, machine_end_time[machine_id])
                    schedule[job_index].append((machine_id, start_time))
                    current_time = start_time + duration
                    machine_end_time[machine_id] = current_time

    # Aglutinação (Agglomeration): Refinando a divisão para melhorar a eficiência
    jobs_per_thread = len(jobs) // num_threads
    threads = []
    for i in range(num_threads):
        start = i * jobs_per_thread
        end = (i + 1) * jobs_per_thread if i != num_threads - 1 else len(jobs)
        thread = threading.Thread(target=schedule_job_range, args=(start, end))
        threads.append(thread)
        # Mapeamento (Mapping): Atribuindo as tarefas às threads e iniciando-as
        thread.start()
    
    for thread in threads:
        thread.join()  # Espera todas as threads terminarem
    
    return schedule

def write_output(file_path, schedule):
    """
    Escreve o cronograma das operações em um arquivo de saída.

    Parâmetros:
    file_path (str): Caminho do arquivo de saída.
    schedule (defaultdict): Dicionário contendo o cronograma das operações para cada trabalho.
    """
    with open(file_path, 'w') as f:
        f.write("Legend:\n")
        f.write("Each line represents the start times of the operations for a job, along with the machine number.\n")
        f.write("Format: <job_index>: <(machine_id, operation_start_time)> <(machine_id, operation_start_time)> ... <(machine_id, operation_start_time)>\n\n")
        
        for job_index in sorted(schedule.keys()):
            start_times = ' '.join([f"({machine_id}, {start_time})" for machine_id, start_time in schedule[job_index]])
            f.write(f"job{job_index}: {start_times}\n")

def main():
    """
    Função principal que lê os argumentos da linha de comando, executa o agendamento dos trabalhos e escreve o resultado.
    """
    if len(sys.argv) != 4:
        print("Usage: python job_shop_parallel.py <input_file> <output_file> <num_threads>")
        return
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    num_threads = int(sys.argv[3])
    
    num_machines, num_jobs, jobs = read_input(input_file)
    
    start_time = time.time()
    schedule = schedule_jobs_parallel(num_machines, jobs, num_threads)
    end_time = time.time()
    
    write_output(output_file, schedule)
    
    print(f"Execution Time with {num_threads} threads: {end_time - start_time} seconds")

if __name__ == "__main__":
    main()
