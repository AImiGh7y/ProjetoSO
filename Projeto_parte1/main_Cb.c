#include <sys/wait.h>
#include <time.h>
#include "main.h"

void numero_anos(TIMESTAMPS_LIST *tl, int *ano_min, int *ano_max);
void parsing_ano(int signal);
static int ano;

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
    int ano_min, ano_max;
    numero_anos(tl, &ano_min, &ano_max);
    int anos = ano_max - ano_min + 1;
    printf("anos: %d, ano_min: %d, ano_max: %d\n", anos, ano_min, ano_max);

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
                        // ignorar timestamps invalidos
                        if(t->timestamps[sala] <= 9999)
                            continue;
                    
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
    // criar M filhos
    int Mpids[anos];
    int Mfds[anos][2];
    for(int i = 0; i < anos; i++) {
        pipe(Mfds[i]);
        Mpids[i] = fork();
        if(Mpids[i] == 0) {
            ano = ano_min + i;
            close(Mfds[i][1]);

            // Mfds[i][0] => stdin
            dup2(Mfds[i][0], 0);

            signal(SIGUSR1, parsing_ano);
            pause();
            exit(0);
        }
        close(Mfds[i][0]);
    }

    // para cada Mi temos que escrever cabeçalho
    for(int i = 0; i < anos; i++) {
        char *cabecalho = "time,count\n";
        write(Mfds[i][1], cabecalho, strlen(cabecalho));
    }

    // Pai
    close(fds[1]);
    int fd = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd == -1) { perror("File open"); exit(1); }
    int filhos_a_correr = nfilhos;
    printf("a processar o ficheiro...\n");
    while(filhos_a_correr > 0) {
        char linha[128];
        if(read(fds[0], linha, 128) > 0) {
            // escrever para output
            write(fd, linha, strlen(linha));
            write(fd, "\n", 1);

            // parsing de pid$id,timestamp,sala#ocupação
            // precisamos do timestamp e da ocupacao
            time_t timestamp;
            int ocupacao;
            sscanf(linha, "%*[^,],%ld,%*[^#]#%d", &timestamp, &ocupacao);
            
            // timestamp -> ano
            struct tm* s = gmtime(&timestamp);
            int ano = s->tm_year + 1900;
            int i = ano - ano_min;
            
            // escrever no formato CSV para o Python
            // timestamp,ocupacao
            char str[128];
            sprintf(str, "%ld,%d\n", timestamp, ocupacao);
            write(Mfds[i][1], str, strlen(str));
            //write(fd, linha, strlen(linha));
            //printf("linha: %s\n", linha);
        }
        if(waitpid(-1, NULL, WNOHANG) > 0) {
            filhos_a_correr--;
        }
    }

    // executar cada M
    printf("a gravar os graficos...\n");
    for(int i = 0; i < anos; i++) {
        close(Mfds[i][1]);
        kill(Mpids[i], SIGUSR1);
    }
    // esperar por cada M
    for(int i = 0; i < anos; i++) {
        printf("esperar por %d\n", Mpids[i]);
        waitpid(Mpids[i], NULL, 0);
    }

    close(fd);
    close(fds[0]);
    return 0;
}

void numero_anos(TIMESTAMPS_LIST *tl, int *ano_min, int *ano_max) {
    *ano_min = 10000;
    *ano_max = 0;
    TIMESTAMPS *it = tl->phead;
    while(it != NULL) {
        // ignorar timestamps invalidos
        if(it->timestamps[0] <= 9999) {
            it = it->pnext;
            continue;
        }

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

    char fname[128];
    sprintf(fname, "output-%d", ano);
    execlp("python3", "python3", "plot.py", fname, NULL);

    perror("exec falhou");
    exit(1);
}
