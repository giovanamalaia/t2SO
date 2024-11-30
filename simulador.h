#ifndef SIMULADOR_H
#define SIMULADOR_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NUM_PROCESSOS 4
#define TAM_MEMORIA 128
#define SEM_KEY 1234

// Declaração das funções
void simular_processos(int segmento, int sem_id, int rodadas);
int inicializar_semaforo();
void destruir_semaforo(int sem_id);

#endif
