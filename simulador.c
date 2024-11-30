#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "simulador.h"

#define SEM_KEY 1234 // Chave para o semáforo

// void simular_processos(int segmento, int sem_id, int rodadas) {
//     char* memoria = (char*)shmat(segmento, NULL, 0);
//     if (memoria == (void*)-1) {
//         perror("Erro ao anexar memória compartilhada");
//         exit(EXIT_FAILURE);
//     }

//     FILE* arquivos[NUM_PROCESSOS] = {
//         fopen("acessos_P1", "r"),
//         fopen("acessos_P2", "r"),
//         fopen("acessos_P3", "r"),
//         fopen("acessos_P4", "r")
//     };

//     for (int i = 0; i < NUM_PROCESSOS; i++) {
//         if (!arquivos[i]) {
//             perror("Erro ao abrir arquivo de acessos");
//             exit(EXIT_FAILURE);
//         }
//     }

//     struct sembuf operacao;
//     int rodada_atual = 0;

//     for (int r = 0; r < rodadas; r++) {
//         printf("\nIniciando rodada %d...\n", ++rodada_atual);

//         for (int processo_atual = 0; processo_atual < NUM_PROCESSOS; processo_atual++) {
//             char linha[20];
//             if (fgets(linha, sizeof(linha), arquivos[processo_atual])) {
//                 // Envia o acesso para a memória compartilhada
//                 strncpy(memoria, linha, TAM_MEMORIA);
//                 printf("P%d enviou: %s", processo_atual + 1, linha);

//                 // Sinaliza o GMV que um dado está disponível
//                 operacao.sem_num = 0;
//                 operacao.sem_op = 1; // Incrementa o semáforo
//                 operacao.sem_flg = 0;
//                 semop(sem_id, &operacao, 1);

//                 sleep(1); // Simula atraso entre processos
//             } else {
//                 printf("Processo P%d terminou seus acessos.\n", processo_atual + 1);
//             }
//         }
//     }

//     printf("Todas as rodadas concluídas.\n");

//     for (int i = 0; i < NUM_PROCESSOS; i++) fclose(arquivos[i]);
//     shmdt(memoria);
// }


void simular_processos(int segmento, int sem_id, int rodadas) {
    char* memoria = (char*)shmat(segmento, NULL, 0);
    if (memoria == (void*)-1) {
        perror("Erro ao anexar memória compartilhada");
        exit(EXIT_FAILURE);
    }

    FILE* arquivos[NUM_PROCESSOS] = {
        fopen("acessos_P1", "r"),
        fopen("acessos_P2", "r"),
        fopen("acessos_P3", "r"),
        fopen("acessos_P4", "r")
    };

    for (int i = 0; i < NUM_PROCESSOS; i++) {
        if (!arquivos[i]) {
            perror("Erro ao abrir arquivo de acessos");
            exit(EXIT_FAILURE);
        }
    }

    struct sembuf operacao;
    int rodada_atual = 0;

    for (int r = 0; r < rodadas; r++) {
        printf("\nIniciando rodada %d...\n", ++rodada_atual);

        for (int processo_atual = 0; processo_atual < NUM_PROCESSOS; processo_atual++) {
            char linha[20];
            if (fgets(linha, sizeof(linha), arquivos[processo_atual])) {
                // Envia o acesso para a memória compartilhada
                strncpy(memoria, linha, TAM_MEMORIA);
                printf("P%d enviou: %s", processo_atual + 1, linha);

                // Sinaliza o GMV que um dado está disponível
                operacao.sem_num = 0;
                operacao.sem_op = 1; // Incrementa o semáforo
                operacao.sem_flg = 0;
                semop(sem_id, &operacao, 1);

                sleep(1); // Simula atraso entre processos
            } else {
                printf("Processo P%d terminou seus acessos.\n", processo_atual + 1);
            }
        }
    }

    printf("Todas as rodadas concluídas.\n");

    for (int i = 0; i < NUM_PROCESSOS; i++) fclose(arquivos[i]);
    shmdt(memoria);
}

int inicializar_semaforo() {
    int sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Erro ao criar semáforo");
        exit(EXIT_FAILURE);
    }
    semctl(sem_id, 0, SETVAL, 0); // Inicializa o semáforo com valor 0
    return sem_id;
}

void destruir_semaforo(int sem_id) {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Erro ao remover semáforo");
    }
}

