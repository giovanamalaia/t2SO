#ifndef GERENCIADOR_MEMORIA_H
#define GERENCIADOR_MEMORIA_H

#include <limits.h>

#define NUM_FRAMES 16
#define NUM_PAGINAS 32
#define NUM_PROCESSOS 4
#define TAM_MEMORIA 128

typedef struct {
    int presente;
    int modificado;
    int referencia;
    int frame;
} EntradaTabelaPagina;

typedef struct {
    EntradaTabelaPagina tabela[NUM_PAGINAS];
} TabelaPagina;

typedef struct {
    int ocupado;
    int processo;
    int pagina;
    int modificado;
    int referenciado;
    int ultimo_acesso; // Timestamp da última referência
} Quadro;


extern Quadro memoria_fisica[NUM_FRAMES];
extern TabelaPagina processos[NUM_PROCESSOS];

void inicializar_memoria();
void tratar_page_fault(int processo, int pagina, char tipo_acesso);
int verificar_page_fault(int processo, int pagina);
void configurar_working_set(int k);
int gerenciar_memoria_virtual(int segmento, int sem_id, int rodadas);
void atualizar_referencias();
void acessar_pagina(int quadro);

// Algoritmos de substituição
int substituir_nru(int processo, int pagina, char tipo_acesso);
int substituir_segunda_chance(int processo, int pagina, char tipo_acesso);
int substituir_lru(int processo, int pagina, char tipo_acesso);
int substituir_working_set(int processo, int pagina, char tipo_acesso);
void escrever_swap(Quadro quadro);  // Usando 'Quadro' no lugar de 'Pagina'


// Ponteiro para o algoritmo de substituição
extern int (*substituir_pagina)(int, int, char);

#endif