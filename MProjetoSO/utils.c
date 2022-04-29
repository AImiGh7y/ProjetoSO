//
// Created by so on 4/28/22.
//

#include "main.h"

void read_timestamps(TIMESTAMPS_LIST *tl, char *path) {
    long bytes, total=0, size;
    char *cds = NULL;

    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("File open"); exit(1); }

    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    cds = (char *) malloc(sizeof(char) * (size+1));
    while ((bytes = read(fd, cds+total, BUF_SIZE)))
        total += bytes;
    close(fd);

    parse_timestamps(tl, cds);
    free(cds);
}

void parse_timestamps(TIMESTAMPS_LIST *tl, char *cds) {

    // ler linha-a-linha (\n)
    char *linha_state;
    char *linha = strtok_r(cds, "\n", &linha_state);

    // primeira linha é o cabeçalho, devemos ignorar
    linha = strtok_r(NULL, "\n", &linha_state);

    while(linha != NULL) {
        // nova linha
        TIMESTAMPS *t = malloc(sizeof(TIMESTAMPS));
        t->pnext = t->pprev = NULL;

        // ler campo-a-campo (;)
        char *campo_state;
        char *campo = strtok_r(linha, ";", &campo_state);
        int n = 0;
        while(campo != NULL) {
            t->timestamps[n] = atol(campo);
            // proximo campo
            campo = strtok_r(NULL, ";", &campo_state);
            n++;
        }

        if(tl->ptail == NULL) {
            // lista vazia, primeiro elemento
            tl->phead = tl->ptail = t;
        }
        else {
            t->pprev = tl->ptail;
            tl->ptail->pnext = t;
            tl->ptail = t;
        }
        tl->n_timestamps++;

        // proxima linha
        linha = strtok_r(NULL, "\n", &linha_state);
    }
}

void print_timestamps(TIMESTAMPS_LIST *tl)
{
    printf("Tamanho: %ld timestamps\n", tl->n_timestamps);
    int i = 0;
    TIMESTAMPS *it = tl->phead;
    while(it != NULL) {
        printf("%d - %ld = %ld = %ld = %ld = %ld\n", i,
               it->timestamps[0], it->timestamps[1], it->timestamps[2], it->timestamps[3],
               it->timestamps[4]);
        it = it->pnext;
        i++;
    }
}