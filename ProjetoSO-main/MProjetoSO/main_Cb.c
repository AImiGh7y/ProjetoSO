#include <sys/wait.h>
#include <time.h>
#include "main.h"

void numero_anos(TIMESTAMPS_LIST *tl, int *ano_min, int *ano_max);
void parsing_ano(int signal);
int fd_pipe;

int main(int argc, char *argv[]) {
    //Alinea A
    if(argc <= 3) {
        printf("Erro: tem que dar o numero de filhos, nome do ficheiro de input e de output\n");
        return -1;
    }
    int nfilhos = atoi(argv[1]);
    char *input = argv[2];
    char *output = argv[3];

    //print_timestamps(tl);
    TIMESTAMPS_LIST *tl = (TIMESTAMPS_LIST *) calloc(1, sizeof(TIMESTAMPS_LIST));
    read_timestamps(tl, input);
    int ano_min, ano_max;
    numero_anos(tl, &ano_min, &ano_max);
    int anos = ano_max - ano_min + 1;

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
                        char linha[1024];
                        char *nome_salas[] = {"Espera Triagem", "Triagem", "Sala de Espera", "Consulta"};
                        sprintf(linha, "%d$%ld,%ld,%s#%d\n", getpid(), j, t->timestamps[sala], nome_salas[sala], ocupacao);
                        write(fds[1], linha, strlen(linha));
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
    // criar M filhos
    int Mpids[anos];
    int Mfds[anos][2];
    for(int i = 0; i < anos; i++) {
        pipe(Mfds[i]);
        Mpids[i] = fork();
        if(Mpids[i] == 0) {
            fd_pipe = Mfds[i][0];
            close(Mfds[i][1]);
            signal(SIGUSR1, parsing_ano);
            pause();
            exit(0);
        }
        close(Mfds[i][0]);
    }

    // Pai
    close(fds[1]);
    int fd = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd == -1) { perror("File open"); exit(1); }
    int filhos_a_correr = nfilhos;
    while(filhos_a_correr >= 0) {
        char linha[1024];
        if(read(fds[0], linha, 1024) > 0) {
            int ano;
            char str[128];
            sscanf(linha, "%s,%d,", str, &ano);
            write(Mfds[ano-ano_min][1], linha, 1024);
            kill(Mpids[ano-ano_min], SIGUSR1);
            //write(fd, linha, strlen(linha));
            //printf("linha: %s\n", linha);
        }
        if(waitpid(-1, NULL, WNOHANG) > 0) {
            printf("filhou parou de correr!\n");
            filhos_a_correr--;
        }
    }
    close(fd);
    close(fds[0]);
    return 0;
}

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

void numero_anos(TIMESTAMPS_LIST *tl, int *ano_min, int *ano_max) {
    *ano_min = 9999;
    *ano_max = 0;
    TIMESTAMPS *it = tl->phead;
    while(it != NULL) {
        struct tm* s = gmtime(&it->timestamps[0]);
        int ano = s->tm_year + 1900;
        if(ano < *ano_min)
            *ano_min = ano;
        if(ano > *ano_max)
            *ano_max = ano;
        it = it->pnext;
    }
}

void parsing_ano(int signal) {
    printf("Parsing_ano\n");
    //TODO: ler da pipe e executar o ficheiro python
}
