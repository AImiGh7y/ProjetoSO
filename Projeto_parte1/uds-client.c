#include <sys/socket.h>
#include <sys/un.h>
#include "main.h"

char *socket_path = "/tmp/socket";

#define BUF_SIZE 4096			/* block transfer size */

int main(int argc, char **argv)
{
    int uds;
    struct sockaddr_un channel;		/* Unix Domain socket */
    
    if (argc != 4) {
        printf("Usage: client file-name\n");
        exit(1);
    }

    char *input = argv[1];
    int inicio = atoi(argv[2]);
    int fim = atoi(argv[3]);

    if ( (uds = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }
    
    memset(&channel, 0, sizeof(channel));
    channel.sun_family= AF_UNIX;
    strncpy(channel.sun_path, socket_path, sizeof(channel.sun_path)-1);
    
    if (connect(uds, (struct sockaddr*)&channel, sizeof(channel)) == -1) {
        perror("connect error");
        exit(1);
    }

    TIMESTAMPS_LIST *tl = (TIMESTAMPS_LIST *) calloc(1, sizeof(TIMESTAMPS_LIST));
    read_timestamps(tl, input);
    //print_timestamps(tl);

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
                write(uds, linha, 128);
            }
        }
        t = t->pnext;
        j++;
    }
    close(uds);
    return 0;
}
