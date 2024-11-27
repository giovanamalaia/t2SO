#include <stdio.h>
#include <stdlib.h>
#include "gerenciador_memoria.h"

Quadro memoria_fisica[NUM_FRAMES];
TabelaPagina processos[NUM_PROCESSOS];

void inicializar_memoria() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        memoria_fisica[i].ocupado = 0;
        memoria_fisica[i].processo = -1;
        memoria_fisica[i].pagina = -1;
        memoria_fisica[i].modificado = 0;
        memoria_fisica[i].referenciado = 0;
    }
    for (int p = 0; p < NUM_PROCESSOS; p++) {
        for (int i = 0; i < NUM_PAGINAS; i++) {
            processos[p].tabela[i].presente = 0;
            processos[p].tabela[i].modificado = 0;
            processos[p].tabela[i].referencia = 0;
            processos[p].tabela[i].frame = -1;
        }
    }
}

void tratar_page_fault(int processo, int pagina, char tipo_acesso) {
    int quadro_vazio = -1;

    // Verifica se há quadros vazios
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (!memoria_fisica[i].ocupado) {
            quadro_vazio = i;
            break;
        }
    }

    if (quadro_vazio != -1) {
        // Alocar página em um quadro vazio
        memoria_fisica[quadro_vazio].ocupado = 1;
        memoria_fisica[quadro_vazio].processo = processo;
        memoria_fisica[quadro_vazio].pagina = pagina;
        memoria_fisica[quadro_vazio].modificado = (tipo_acesso == 'W') ? 1 : 0; // Marca como modificado se for 'W'
        memoria_fisica[quadro_vazio].referenciado = 1;

        processos[processo].tabela[pagina].presente = 1;
        processos[processo].tabela[pagina].frame = quadro_vazio;
        processos[processo].tabela[pagina].modificado = (tipo_acesso == 'W') ? 1 : 0; // Ajusta na tabela de páginas

        printf("Página %d do processo %d alocada no quadro %d\n", pagina, processo, quadro_vazio);
    } else {
        // Substituição de página
        int quadro_a_substituir = substituir_pagina(processo, pagina, tipo_acesso);

        int proc_antigo = memoria_fisica[quadro_a_substituir].processo;
        int pag_antiga = memoria_fisica[quadro_a_substituir].pagina;

        processos[proc_antigo].tabela[pag_antiga].presente = 0;

        if (memoria_fisica[quadro_a_substituir].modificado) {
            printf("  Página %d do processo %d escrita na área de swap.\n", pag_antiga, proc_antigo);
        }

        memoria_fisica[quadro_a_substituir].processo = processo;
        memoria_fisica[quadro_a_substituir].pagina = pagina;
        memoria_fisica[quadro_a_substituir].modificado = (tipo_acesso == 'W') ? 1 : 0; // Ajusta o bit de modificado
        memoria_fisica[quadro_a_substituir].referenciado = 1;

        processos[processo].tabela[pagina].presente = 1;
        processos[processo].tabela[pagina].frame = quadro_a_substituir;
        processos[processo].tabela[pagina].modificado = (tipo_acesso == 'W') ? 1 : 0;

        printf("Página %d do processo %d substituiu página %d do processo %d\n",
               pagina, processo, pag_antiga, proc_antigo);
    }
}

int substituir_nru(int processo, int pagina, char tipo_acesso) {
    int candidato = -1;

    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memoria_fisica[i].referenciado == 0 && memoria_fisica[i].modificado == 0) {
            candidato = i;
            break;
        }
    }

    if (candidato == -1) {
        for (int i = 0; i < NUM_FRAMES; i++) {
            if (memoria_fisica[i].referenciado == 0) {
                candidato = i;
                break;
            }
        }
    }

    printf("NRU escolheu quadro %d para substituir\n", candidato != -1 ? candidato : 0);

    return (candidato != -1) ? candidato : 0;
}


int substituir_segunda_chance(int processo, int pagina, char tipo_acesso) {
    static int ponteiro = 0;

    while (1) {
        if (memoria_fisica[ponteiro].referenciado == 0) {
            int quadro = ponteiro;
            ponteiro = (ponteiro + 1) % NUM_FRAMES;
            return quadro;
        }

        memoria_fisica[ponteiro].referenciado = 0;
        ponteiro = (ponteiro + 1) % NUM_FRAMES;
    }
}

int substituir_lru(int processo, int pagina, char tipo_acesso) {
    int menor_contador = INT_MAX;
    int quadro_lru = -1;

    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memoria_fisica[i].referenciado < menor_contador) {
            menor_contador = memoria_fisica[i].referenciado;
            quadro_lru = i;
        }
    }

    return quadro_lru;
}

int substituir_working_set(int processo, int pagina, char tipo_acesso) {
    int quadro_a_substituir = -1;

    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memoria_fisica[i].referenciado < 3) {
            quadro_a_substituir = i;
            break;
        }
    }

    return quadro_a_substituir != -1 ? quadro_a_substituir : 0;
}
