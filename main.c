#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gerenciador_memoria.h"
#include "simulador.h"

#include "acessos.h"
#include <sys/_types/_key_t.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>


// Função para imprimir as tabelas de processos
void imprimir_tabela_processos() {
    printf("\nTabela de Processos:\n");
    for (int p = 0; p < NUM_PROCESSOS; p++) {
        printf("Processo P%d:\n", p + 1);
        for (int i = 0; i < NUM_PAGINAS; i++) {
            if (processos[p].tabela[i].presente) {
                printf("  Página %02d -> Quadro %d [Modificado: %d]\n",
                       i, processos[p].tabela[i].frame, processos[p].tabela[i].modificado);
            }
        }
        printf("\n");
    }
}

int main() {
    // Inicializa semente para geração aleatória
    srand(time(NULL));

    // Gera arquivos de acesso simulados
    //gerar_todos_acessos();

    // Inicializa memória e tabelas de página
    inicializar_memoria();

    int algoritmo, rodadas;

    printf("Escolha o algoritmo de substituição:\n");
    printf("1 - NRU\n2 - Segunda Chance\n3 - LRU\n4 - Working Set\n");
    scanf("%d", &algoritmo);

    printf("Número de rodadas: ");
    scanf("%d", &rodadas);

    // Configurar o ponteiro para a função de substituição
    switch (algoritmo) {
        case 1:
            substituir_pagina = substituir_nru;
            printf("Algoritmo de Substituição: NRU\n");
            break;
        case 2:
            substituir_pagina = substituir_segunda_chance;
            printf("Algoritmo de Substituição: Segunda Chance\n");
            break;
        case 3:
            substituir_pagina = substituir_lru;
            printf("Algoritmo de Substituição: LRU\n");
            break;
        case 4:
            printf("Informe o valor de k para Working Set: ");
            int k;
            scanf("%d", &k);
            configurar_working_set(k);
            substituir_pagina = substituir_working_set;
            printf("Algoritmo de Substituição: Working Set (k = %d)\n", k);
            break;
        default:
            printf("Algoritmo inválido!\n");
            exit(1);
    }

    // Configuração da memória compartilhada e semáforo
    key_t chave_memoria = ftok("/tmp", 'M');
    int segmento = shmget(chave_memoria, TAM_MEMORIA, IPC_CREAT | 0666);
    if (segmento == -1) {
        perror("Erro ao criar memória compartilhada");
        exit(EXIT_FAILURE);
    }

    int sem_id = inicializar_semaforo();

    if (fork() == 0) {
        // Processo filho: Simulador
        simular_processos(segmento, sem_id, rodadas);
        exit(0);
    } else {
        // Processo pai: Gerenciador de Memória
        int total_page_faults = gerenciar_memoria_virtual(segmento, sem_id, rodadas);

        // Limpeza de recursos
        shmctl(segmento, IPC_RMID, NULL); // Remove memória compartilhada
        destruir_semaforo(sem_id);       // Remove semáforo

        // Exibição do resumo
        printf("\nResumo da Simulação:\n");
        printf("Rodadas executadas: %d\n", rodadas);
        printf("Total de Page Faults: %d\n", total_page_faults);
        imprimir_tabela_processos();
    }

    return 0;
}
