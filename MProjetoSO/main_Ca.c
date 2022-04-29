#include <sys/wait.h>
#include "main.h"

int main(int argc, char *argv[]) {

    //Alinea A
    if(argc <= 3) {
        printf("Erro: tem que dar o numero de filhos, nome do ficheiro de input e de output\n");
        return -1;
    }
    int nfilhos = atoi(argv[1]);
    char *input = argv[2];
    char *output = argv[3];

    TIMESTAMPS_LIST *tl = (TIMESTAMPS_LIST *) calloc(1, sizeof(TIMESTAMPS_LIST));
    read_timestamps(tl, input);
    //print_timestamps(tl);

    //Alinea B
    long tamanho_filho = tl->n_timestamps / nfilhos;

    //Alinea C
    int fds[2];
    if(pipe(fds) == -1) {
        perror("Pipe falhou!");
        return -1;
    }

    for(int i = 0; i < nfilhos; i++) {
        //Alinea B
        //char filename[100];
        //sprintf(filename, "child-%d.txt", i);
        pid_t pid = fork();
        if(pid == 0) {
            close(fds[0]);
            //Alinea B
            //int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0600);
            //if (fd == -1) { perror("File open"); exit(1); }

            long inicio = tamanho_filho * i;
            long fim = tamanho_filho * (i+1);
            if(i == nfilhos - 1)
                //o ultimo filho fica com o resto
                fim = tl->n_timestamps;

            TIMESTAMPS *t = tl->phead;
            long j = 0;    //paciente
            while (t != NULL) {
                if(j >= inicio && j < fim) {
                    for(int sala = 0; sala < N_SALAS; sala++) {
                        // calcular quantas pessoas estao na sala ao mesmo tempo que o paciente "t"
                        int ocupacao = 0;
                        TIMESTAMPS *t2 = tl->phead;
                        while(t2 != t) {
                            if(!(t2->timestamps[sala] > t->timestamps[sala+1] || t2->timestamps[sala+1] < t->timestamps[sala])) {
                                ocupacao++;
                            }
                            t2 = t2->pnext;
                        }
                        // pid$id,timestamp,sala#ocupação (separador = ‘\n’)
                        char linha[128];
                        char *nome_salas[] = {"Espera Triagem", "Triagem", "Sala de Espera", "Consulta"};
                        sprintf(linha, "%d$%ld,%ld,%s#%d", getpid(), j, t->timestamps[sala], nome_salas[sala], ocupacao);
                        write(fds[1], linha, 128);
                    }
                }
                t = t->pnext;
                j++;
            }
            //close(fd);
            close(fds[1]);
            exit(0);
        }
    }
    // Pai
    close(fds[1]);
    int fd = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd == -1) { perror("File open"); exit(1); }
    int filhos_a_correr = nfilhos;
    while(filhos_a_correr > 0) {
        char linha[128];
        if(read(fds[0], linha, 128) > 0) {
            write(fd, linha, strlen(linha));
            write(fd, "\n", 1);
            //printf("linha: %s\n", linha);
        }
        if(waitpid(-1, NULL, WNOHANG) > 0) {
            filhos_a_correr--;
        }
    }
    close(fd);
    close(fds[0]);
    return 0;
}


