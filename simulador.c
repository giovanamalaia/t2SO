#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "simulador.h"


void simular_processos(int segmento) {
    char* memoria = (char*) shmat(segmento, NULL, 0);
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

    int processo_atual = 0;
    while (1) {
        char linha[20];
        if (fgets(linha, sizeof(linha), arquivos[processo_atual])) {
            // Envia o acesso para o GMV
            strncpy(memoria, linha, TAM_MEMORIA);
            printf("P%d enviou: %s", processo_atual + 1, linha);
            sleep(1); // Simula atraso
        } else {
            break; // Fim do arquivo
        }
        processo_atual = (processo_atual + 1) % NUM_PROCESSOS;
    }

    for (int i = 0; i < NUM_PROCESSOS; i++) fclose(arquivos[i]);
    shmdt(memoria);
}
