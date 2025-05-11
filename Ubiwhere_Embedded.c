#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// Estrutura para armazenar dados do sensor
typedef struct {
    float temperatura;
    unsigned long timestamp;
} sensor_data_t;

// Buffer para comunicação entre as threads
sensor_data_t sensor_buffer;
int dados_disponiveis = 0;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dados_prontos = PTHREAD_COND_INITIALIZER;

// Simula a leitura de um sensor de temperatura
float ler_temperatura() {
    // Simular ler temperatura de NTC Vishay NTCLE100E3103JB0
    // com 10K ohm a 25ºC, B=3977K
    // Usando o método do ponto de inflexão com resistencia fixa de 10k ohm
    // Lendo com uma ADC de 10bits temos valores entre 200 e 800 (especulados)
    
    // Para simular valores aleatórios:
    int adc = 200 + rand() % (800-200+1); // valor da adc simulado entre 200 e 800
    
    // COnverter valores de ADC para temperatura:
    // Mapeamento linear: ADC 200-800 para temperatura 0-50°C
    float m = 50.0 / (800.0 - 200.0);  // Inclinação da reta 
    float b = -m * 200.0;              // Valor da ordenada na origem das absissas
    
    float temp = m * adc + b;
    
    return temp;
}

// Simula a leitura de um sensor de temperatura que o utilkizador pode escolher
float ler_temperatura_manual(int leitura_manual) {
    // Simular ler temperatura de NTC Vishay NTCLE100E3103JB0
    // com 10K ohm a 25ºC, B=3977K
    // Usando o método do ponto de inflexão com resistencia fixa de 10k ohm
    // Lendo com uma ADC de 10bits temos valores entre 200 e 800 (especulados)
    
    // Para simular valores aleatórios:
    int adc = leitura_manual; // valor da adc simulado entre 200 e 800
    
    // COnverter valores de ADC para temperatura:
    // Mapeamento linear: ADC 200-800 para temperatura 0-50°C
    float m = 50.0 / (800.0 - 200.0);  // Inclinação da reta 
    float b = -m * 200.0;              // Valor da ordenada na origem das absissas
    
    float temp = m * adc + b;
    
    return temp;
}

// Thread para leitura do sensor (Task 0)
void* task_sensor(void *arg) {

    printf("[Task 0] Tarefa de leitura do sensor iniciada\n");
    
    while (1) {
        // Adquirir lock
        pthread_mutex_lock(&buffer_mutex);
        
        // Ler o sensor
        sensor_buffer.temperatura = ler_temperatura();
        sensor_buffer.timestamp = time(0);
        dados_disponiveis = 1;

        
        // Sinalizar que novos dados estão disponíveis
        pthread_cond_signal(&dados_prontos);
        
        // Liberar lock
        pthread_mutex_unlock(&buffer_mutex);
        
        // Aguardar antes da próxima leitura
        sleep(1);
    }
}

// Thread para envio UART (Task 1)
void* task_uart(void *arg) {
    
    
    printf("[Task 1] Tarefa de envio UART iniciada\n");
    
    while (1) {
        // Adquirir lock
        pthread_mutex_lock(&buffer_mutex);
        
        // Esperar por novos dados
        while (!dados_disponiveis) {
            pthread_cond_wait(&dados_prontos, &buffer_mutex);
        }
        
        // Converter tempo lido em epoch para formato legível
        time_t raw_time = sensor_buffer.timestamp;
        struct tm *time_info = localtime(&raw_time);
        
        
        
        // Mudar formato da hora em epoch para data e hora
        char formatted_time[20];
        strftime(formatted_time, sizeof(formatted_time), "%Y-%m-%d %H:%M:%S", time_info);
        
        
        // Enviar dados via UART (simulado com printf)
        printf("[Task 1] DADOS ENVIADOS VIA UART\n");
        printf("Data/Hora: %-30s \n", formatted_time);
        printf("Temperatura: %.2f °C\n", sensor_buffer.temperatura);

       
        // Abrir arquivo CSV para escrita
        FILE *file = fopen("dados_sensor.csv", "a");
        if (file == 0) {
            perror("Erro ao abrir arquivo CSV");
        } else {
            // Escrever dados no arquivo CSV
            fprintf(file, "%s,%.2f\n", formatted_time, sensor_buffer.temperatura);
            fclose(file);
        }
        // Marcar dados como lidos
        dados_disponiveis = 0;
        
        // Liberar lock
        pthread_mutex_unlock(&buffer_mutex);
    }
}

int main() {
    // Inicializar gerador de números aleatórios
    srand(time(0));
    

    printf("SIMULAÇÃO DE LEITURA DE SENSOR DE TEMPERATURA\n\n");
    
     // Apagar dados do arquivo CSV antes de escrever
    FILE *file_clear = fopen("dados_sensor.csv", "w");
    if (file_clear == 0) {
        perror("Erro ao limpar arquivo CSV");
    } else {
        fclose(file_clear);
    }
    
    // Criar as threads
    pthread_t thread_sensor, thread_uart;
    pthread_create(&thread_sensor, 0, task_sensor, 0);
    pthread_create(&thread_uart, 0, task_uart, 0);
    
    // Aguardar as threads
    pthread_join(thread_sensor, 0);
    pthread_join(thread_uart, 0);
    

    return 0;
}