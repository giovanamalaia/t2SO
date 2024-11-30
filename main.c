#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gerenciador_memoria.h"
#include "acessos.h"


// Ponteiro para a função de substituição de páginas
int (*substituir_pagina)(int, int, char);

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
    srand(time(NULL));
    gerar_todos_acessos();
    inicializar_memoria();
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
            substituir_pagina = substituir_working_set;
            printf("Algoritmo de Substituição: Working Set (k = 3)\n");
            break;
        default:
            printf("Algoritmo inválido!\n");
            exit(1);
    }

    int total_page_faults = 0;

    for (int i = 0; i < rodadas; i++) {
        int processo = rand() % NUM_PROCESSOS;
        int pagina = rand() % NUM_PAGINAS;
        char tipo_acesso = (rand() % 2 == 0) ? 'R' : 'W';

        printf("Rodada %d: Processo P%d acessa página %d (%c)\n", i + 1, processo + 1, pagina, tipo_acesso);

        // Antes de acessar, verificamos se ocorre page fault
        int frame_anterior = processos[processo].tabela[pagina].frame;
        int presente_anterior = processos[processo].tabela[pagina].presente;
        int modificado_anterior = processos[processo].tabela[pagina].modificado;

        tratar_page_fault(processo, pagina, tipo_acesso);

        if (!presente_anterior) {
            total_page_faults++;
            printf("  Page Fault! Px = Processo P%d\n", processo + 1);

            if (frame_anterior != -1 && modificado_anterior) {
                printf("  Página suja escrita na área de swap.\n");
            }
        }
    }

    printf("\nResumo da Simulação:\n");
    printf("Rodadas executadas: %d\n", rodadas);
    printf("Total de Page Faults: %d\n", total_page_faults);

    imprimir_tabela_processos();

    return 0;
}
