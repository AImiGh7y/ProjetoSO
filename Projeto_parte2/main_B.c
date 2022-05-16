#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include "main.h"

void *worker(void *arg);
int N, P;
TIMESTAMPS_LIST *tl;

//char buf[N][128];
char** buf;
int prodptr = 0, consptr = 0;
pthread_mutex_t trinco_p, trinco_c;
//sem_t pode_prod = criar_semaforo(N), pode_cons = criar_semaforo(0);
sem_t pode_prod , pode_cons;

void* produtor(void* unused)
{
    while(1) {
        char str[128];
        produz(str);
        sem_wait(&pode_prod);
        pthread_mutex_lock(&trinco_p);
        buf[prodptr] = item;
        prodptr = (prodptr+1) % N;
        pthread_mutex_unlock(&trinco_p);
        sem_post(&pode_cons);
    }
    return NULL;
}

void* consumidor(void* unused) {
    while(1) {
        char* item;
        sem_wait(&pode_cons);
        pthread_mutex_lock(&trinco_c);
        item = buf[consptr];
        consptr = (consptr+1) % N;
        pthread_mutex_unlock(&trinco_c);
        sem_post(&pode_prod);
        consome(item);
    }
    return NULL;
}

int main(int argc, char *argv[]) {

    //Alinea A
    if(argc <= 4) {
        printf("Erro: tem que dar o numero de consumidores, numero de produtores, nome do ficheiro de input e de output\n");
        return -1;
    }
    N = atoi(argv[1]);
    P = atoi(argv[2]);
    char *input = argv[3];
    char *output = argv[4];

    tl = (TIMESTAMPS_LIST *) calloc(1, sizeof(TIMESTAMPS_LIST));
    read_timestamps(tl, input);
    //print_timestamps(tl);

    for(long i = 0; i < nfilhos; i++) {
        //pid_t pid = fork();
        pthread_t id;
        pthread_create(&id, NULL, worker, (void *)i);

    }
    // Pai
    pthread_exit(NULL);
    return 0;
}

void *worker(void *arg)
{
    long i = (long)arg;
    char filename[100];
    sprintf(filename, "worker-%ld.txt", i);
    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd == -1) { perror("File open"); exit(1); }

    TIMESTAMPS *t = tl->phead;
    long j = 0;    //paciente
    while (t != NULL) {
        if(j % nfilhos == i) {
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
                sprintf(linha, "%d$%ld,%ld,%s#%d\n", getpid(), j, t->timestamps[sala], nome_salas[sala], ocupacao);
                write(fd, linha, strlen(linha));
            }
        }
        t = t->pnext;
        j++;
    }
    return NULL;
}


