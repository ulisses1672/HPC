import json
import random
import sys

# Definição dos parâmetros para a geração de dados
num_machines = 10  # Número de máquinas disponíveis
num_jobs = 1000  # Número de trabalhos (jobs) a serem gerados
max_operations_per_job = 5  # Número máximo de operações por trabalho
max_duration = 10  # Duração máxima de uma operação

def generate_input(file_name, num_machines, num_jobs, max_operations_per_job, max_duration):
    """
    Gera um arquivo JSON com dados de entrada para o problema de Job-Shop Scheduling.

    Parâmetros:
    file_name (str): Nome do arquivo de saída para salvar os dados gerados.
    num_machines (int): Número de máquinas disponíveis.
    num_jobs (int): Número de trabalhos (jobs) a serem gerados.
    max_operations_per_job (int): Número máximo de operações por trabalho.
    max_duration (int): Duração máxima de uma operação.
    """
    jobs = []

    # Gera dados para cada trabalho
    for _ in range(num_jobs):
        job = []
        for _ in range(max_operations_per_job):
            machine_id = random.randint(0, num_machines - 1)
            duration = random.randint(1, max_duration)
            job.append([machine_id, duration])
        jobs.append(job)

    # Estrutura os dados em um dicionário
    data = {
        "num_machines": num_machines,
        "num_jobs": num_jobs,
        "jobs": jobs
    }

    # Salva os dados gerados em um arquivo JSON
    with open(file_name, 'w') as f:
        json.dump(data, f, indent=4)

def main():
    """
    Função principal que lê o nome do arquivo de saída a partir da linha de comando 
    e chama a função de geração de dados.
    """
    if len(sys.argv) != 2:
        print("Usage: python generate_large_input.py <output_file>")
        return
    
    output_file = sys.argv[1]
    generate_input(output_file, num_machines, num_jobs, max_operations_per_job, max_duration)

if __name__ == "__main__":
    main()
