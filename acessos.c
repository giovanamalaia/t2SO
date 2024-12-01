#include <stdio.h>
#include <stdlib.h>
#include "acessos.h"

void gerar_acessos(const char* nome_arquivo) {
    FILE* arquivo = fopen(nome_arquivo, "w");
    if (!arquivo) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 100; i++) {
        int pagina = rand() % 32; 
        char tipo_acesso = (rand() % 2 == 0) ? 'R' : 'W'; 
        fprintf(arquivo, "%02d %c\n", pagina, tipo_acesso);
    }

    fclose(arquivo);
}

void gerar_todos_acessos() {
    gerar_acessos("acessos_P1");
    gerar_acessos("acessos_P2");
    gerar_acessos("acessos_P3");
    gerar_acessos("acessos_P4");
}