#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>



#include "gerenciador_memoria.h"

Quadro memoria_fisica[NUM_FRAMES];
TabelaPagina processos[NUM_PROCESSOS];

int (*substituir_pagina)(int, int, char);


// Valor dinâmico para a janela do Working Set
int tamanho_working_set = 3;

void configurar_working_set(int k) {
    tamanho_working_set = k;
}

void inicializar_memoria() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        memoria_fisica[i].ocupado = 0;
        memoria_fisica[i].processo = -1;
        memoria_fisica[i].pagina = -1;
        memoria_fisica[i].modificado = 0;
        memoria_fisica[i].referenciado = 0;
         memoria_fisica[i].ultimo_acesso = 0;
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

int gerenciar_memoria_virtual(int segmento, int sem_id, int rodadas) {
    char* memoria = (char*)shmat(segmento, NULL, 0);
    if (memoria == (void*)-1) {
        perror("Erro ao anexar memória compartilhada");
        exit(EXIT_FAILURE);
    }

    struct sembuf operacao;
    int total_page_faults = 0;

    for (int r = 0; r < rodadas; r++) {
        printf("\nRodada %d:\n", r + 1);

        for (int processo_atual = 0; processo_atual < NUM_PROCESSOS; processo_atual++) {
            // Espera por um dado do simulador
            operacao.sem_num = 0;
            operacao.sem_op = -1; // Decrementa o semáforo (espera)
            operacao.sem_flg = 0;
            semop(sem_id, &operacao, 1);

            // Lê o acesso da memória compartilhada
            printf("GMV recebeu: %s\n", memoria);

            int pagina;
            char tipo_acesso;
            sscanf(memoria, "%d %c", &pagina, &tipo_acesso);

            // Verifica e trata o page fault
            if (verificar_page_fault(processo_atual, pagina)) {
                tratar_page_fault(processo_atual, pagina, tipo_acesso);
                total_page_faults++;
            }
        }
        atualizar_referencias();
    }

    // Libera a memória compartilhada
    shmdt(memoria);
    return total_page_faults;
}

int verificar_page_fault(int processo, int pagina) {
    return !processos[processo].tabela[pagina].presente;
}

void tratar_page_fault(int processo, int pagina, char tipo_acesso) {
    if (!verificar_page_fault(processo, pagina)) {
        printf("  Página %d do processo %d já está na memória.\n", pagina, processo+1);
        return;
    }

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
        memoria_fisica[quadro_vazio].ultimo_acesso = clock();

        processos[processo].tabela[pagina].presente = 1;
        processos[processo].tabela[pagina].frame = quadro_vazio;
        processos[processo].tabela[pagina].modificado = (tipo_acesso == 'W') ? 1 : 0; // Ajusta na tabela de páginas
        
        acessar_pagina(quadro_vazio);
        printf("Página %d do processo %d alocada no quadro %d\n", pagina, processo+1, quadro_vazio);
    } else {
        // Substituição de página
        int quadro_a_substituir = substituir_pagina(processo, pagina, tipo_acesso);

        int proc_antigo = memoria_fisica[quadro_a_substituir].processo;
        int pag_antiga = memoria_fisica[quadro_a_substituir].pagina;

        processos[proc_antigo].tabela[pag_antiga].presente = 0;

        if (memoria_fisica[quadro_a_substituir].modificado) {
            printf("  Página %d do processo %d escrita na área de swap.\n", pag_antiga, proc_antigo+1);
        }

        memoria_fisica[quadro_a_substituir].processo = processo;
        memoria_fisica[quadro_a_substituir].pagina = pagina;
        memoria_fisica[quadro_a_substituir].modificado = (tipo_acesso == 'W') ? 1 : 0; // Ajusta o bit de modificado
        memoria_fisica[quadro_a_substituir].referenciado = 1;
        memoria_fisica[quadro_a_substituir].ultimo_acesso = clock();

        processos[processo].tabela[pagina].presente = 1;
        processos[processo].tabela[pagina].frame = quadro_a_substituir;
        processos[processo].tabela[pagina].modificado = (tipo_acesso == 'W') ? 1 : 0;

        acessar_pagina(quadro_a_substituir);
        printf("Página %d do processo %d substituiu página %d do processo %d\n",
               pagina, processo+1, pag_antiga, proc_antigo+1);
    }
}

void atualizar_referencias() {
    if (substituir_pagina == substituir_nru || substituir_pagina == substituir_segunda_chance) {
        for (int i = 0; i < NUM_FRAMES; i++) {
            memoria_fisica[i].referenciado = 0; // Reseta apenas os bits de referência
            // Os bits de modificação não são alterados aqui
        }
        printf("Bits de referência resetados após a rodada.\n");
    }
}

// int substituir_lru(int processo, int pagina, char tipo_acesso) {
//     int menor_contador = INT_MAX;
//     int quadro_lru = -1;

//     for (int i = 0; i < NUM_FRAMES; i++) {
//         if (memoria_fisica[i].referenciado < menor_contador) {
//             menor_contador = memoria_fisica[i].referenciado;
//             quadro_lru = i;
//         }
//     }

//     return quadro_lru;
// }

// int substituir_working_set(int processo, int pagina, char tipo_acesso) {
//     int quadro_a_substituir = -1;
//     int paginas_no_ws = 0;

//     printf("\n[DEBUG] Working Set Substituição - Processo %d, Página %d\n", processo+1, pagina);

//     // Contar páginas no Working Set do processo
//     for (int i = 0; i < NUM_FRAMES; i++) {
//         if (memoria_fisica[i].processo == processo) {
//             paginas_no_ws++;
//         }
//     }

//     // Se o número de páginas no Working Set for menor que k, nenhuma substituição é necessária
//     if (paginas_no_ws < tamanho_working_set) {
//         printf("[DEBUG] Espaço disponível no Working Set (tamanho atual: %d, limite: %d)\n", paginas_no_ws, tamanho_working_set);
//         return -1; // Nenhuma substituição necessária
//     }

//     // Se exceder o limite k, substitua a página mais antiga (FIFO)
//     int tempo_mais_antigo = INT_MAX;

//     for (int i = 0; i < NUM_FRAMES; i++) {
//         if (memoria_fisica[i].processo == processo) {
//             if (memoria_fisica[i].ultimo_acesso < tempo_mais_antigo) {
//                 tempo_mais_antigo = memoria_fisica[i].ultimo_acesso;
//                 quadro_a_substituir = i;
//             }
//         }
//     }

//     printf("[DEBUG] Página fora do Working Set (quadro %d) será substituída\n", quadro_a_substituir);
//     return quadro_a_substituir;
// }



void acessar_pagina(int quadro) {
    memoria_fisica[quadro].referenciado = 1;
    memoria_fisica[quadro].ultimo_acesso = clock(); // Atualiza o tempo atual
}

int substituir_nru(int processo, int pagina, char tipo_acesso) {
    int menor_classe = 4; // Maior classe possível + 1
    int candidatos[NUM_FRAMES];
    int num_candidatos = 0;

    for (int i = 0; i < NUM_FRAMES; i++) {
        // Verifica se o quadro pertence ao processo que causou o page-fault
        if (memoria_fisica[i].processo != processo) {
            continue; // Ignora quadros de outros processos
        }

        // Calcula a classe com base nos bits de referência e modificação
        int classe = (memoria_fisica[i].referenciado << 1) | memoria_fisica[i].modificado;

        // Atualiza a menor classe e reseta os candidatos, caso encontrado uma nova classe menor
        if (classe < menor_classe) {
            menor_classe = classe;
            num_candidatos = 0; // Redefine os candidatos para a nova menor classe
        }

        // Adiciona o quadro ao array de candidatos se tiver a mesma classe
        if (classe == menor_classe) {
            candidatos[num_candidatos++] = i;
        }
    }

    // Verifica se há candidatos para substituição
    if (num_candidatos == 0) {
        printf("Erro: Nenhuma página do processo %d encontrada para substituição.\n", processo+1);
        return -1; // Indica erro
    }

    // Escolhe um quadro aleatório entre os candidatos da menor classe
    int escolhido = candidatos[rand() % num_candidatos];

    // Se a página escolhida foi modificada, deve ser escrita no swap antes da substituição
    if (memoria_fisica[escolhido].modificado) {
        // Aqui você chamaria a função que lida com o swap, como escrever_swap()
        printf("Página modificada no quadro %d, escrevendo no swap...\n", escolhido);
        escrever_swap(memoria_fisica[escolhido]);
    }

    // Log para depuração
    printf("NRU escolheu quadro %d para substituir (Classe %d, Processo %d)\n",
           escolhido, menor_classe, processo+1);

    return escolhido;
}

void escrever_swap(Quadro quadro) {
    // A função agora recebe um 'Quadro' que contém a página
    // Implemente a lógica para escrever no swap
    printf("Escrevendo a página do processo %d e da página %d no swap...\n", 
            quadro.processo+1, quadro.pagina);
}

int substituir_segunda_chance(int processo, int pagina, char tipo_acesso) {
    static int ponteiro = 0;

    while (1) {
        // Verifica se o quadro de memória pertence ao processo que causou o page-fault
        if (memoria_fisica[ponteiro].referenciado == 0 && memoria_fisica[ponteiro].processo == processo) {
            // Realiza a substituição da página dentro do próprio processo
            int quadro = ponteiro;
            ponteiro = (ponteiro + 1) % NUM_FRAMES;
            return quadro;  // Retorna o quadro onde a substituição ocorreu
        }

        // Se o quadro de memória está sendo referenciado, resetamos o bit e passamos para o próximo quadro
        memoria_fisica[ponteiro].referenciado = 0;
        ponteiro = (ponteiro + 1) % NUM_FRAMES;
    }
}

int substituir_lru(int processo, int pagina, char tipo_acesso) {
    int menor_tempo = INT_MAX; // Inicializa com o maior valor possível
    int quadro_lru = -1;       // Nenhum quadro escolhido inicialmente

    // Itera sobre todos os quadros para encontrar o quadro LRU do processo atual
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memoria_fisica[i].processo == processo) { // Verifica apenas os quadros do processo atual
            if (memoria_fisica[i].ultimo_acesso < menor_tempo) {
                menor_tempo = memoria_fisica[i].ultimo_acesso;
                quadro_lru = i;
            }
        }
    }

    // Remove a página substituída da tabela de páginas do processo
    if (quadro_lru != -1) {
        int pagina_antiga = memoria_fisica[quadro_lru].pagina;

        // Atualiza a tabela de páginas do processo para remover a página substituída
        processos[processo].tabela[pagina_antiga].presente = 0;

        // Atualiza o quadro com a nova página
        memoria_fisica[quadro_lru].pagina = pagina;
        memoria_fisica[quadro_lru].modificado = (tipo_acesso == 'W') ? 1 : 0;
        memoria_fisica[quadro_lru].ultimo_acesso = clock(); // Marca o tempo atual
    }

    return quadro_lru; // Retorna o quadro substituído
}

#include <stdio.h>
#include <stdlib.h>
#include "gerenciador_memoria.h"

// int substituir_working_set(int processo, int pagina, char tipo_acesso) {
//     WorkingSet* ws = &working_sets[processo]; // Acessa o Working Set do processo atual
//     int quadro_a_substituir = -1;

//     // Verifica se o número de páginas no Working Set é menor que k
//     if (ws->tamanho < tamanho_working_set) {
//         printf("[DEBUG] Working Set não está cheio (tamanho atual: %d, limite: %d)\n", ws->tamanho, tamanho_working_set);
//         return -1; // Nenhuma substituição necessária
//     }

//     // Substituir a página mais antiga (FIFO dentro do Working Set)
//     int pagina_a_remover = ws->paginas[0];

//     // Procura o quadro correspondente à página mais antiga
//     for (int i = 0; i < NUM_FRAMES; i++) {
//         if (memoria_fisica[i].processo == processo && memoria_fisica[i].pagina == pagina_a_remover) {
//             quadro_a_substituir = i;
//             break;
//         }
//     }

//     if (quadro_a_substituir == -1) {
//         printf("[ERRO] Não foi possível encontrar a página para substituição no Working Set\n");
//         return -1;
//     }

//     // Atualiza o Working Set removendo a página mais antiga
//     for (int i = 1; i < ws->tamanho; i++) {
//         ws->paginas[i - 1] = ws->paginas[i];
//     }
//     ws->tamanho--;

//     // Atualiza a tabela de páginas do processo
//     processos[processo].tabela[pagina_a_remover].presente = 0;

//     printf("[DEBUG] Página %d do Processo %d removida do quadro %d\n",
//            pagina_a_remover, processo + 1, quadro_a_substituir);

//     return quadro_a_substituir;
// }





