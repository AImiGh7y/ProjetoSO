#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "main.h"

#define BUF_SIZE 4096                                                       // block transfer size
#define LISTENQ 5                                                           // Size of the listen Queue


char *socket_path = "/tmp/socket";                                          // Unix domain socket name


int main(int argc, char *argv[])
{
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

    int listenfd,connfd;     // buffer for outgoing file
    struct sockaddr_un channel_srv;
    
    if ( (listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {              // Creating the server socket
        perror("socket error");
        exit(-1);
    }
    
    unlink(socket_path);
    
    memset(&channel_srv, 0, sizeof(channel_srv));
    channel_srv.sun_family = AF_UNIX;
    strncpy(channel_srv.sun_path, socket_path, sizeof(channel_srv.sun_path)-1);
    
    if (bind(listenfd, (struct sockaddr*)&channel_srv, sizeof(channel_srv)) == -1) {      // Binding the server socket to its name
        perror("bind error");
        exit(-1);
    }
    if (listen(listenfd, LISTENQ) == -1) {                                  // Configuring the listen queue
        perror("listen error");
        exit(-1);
    }

    for(int i = 0; i < nfilhos; i++) {
        //Alinea B
        //char filename[100];
        //sprintf(filename, "child-%d.txt", i);
        pid_t pid = fork();
        if(pid == 0) {
            //Alinea B
            //int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0600);
            //if (fd == -1) { perror("File open"); exit(1); }

            long inicio = tamanho_filho * i;
            long fim = tamanho_filho * (i+1);
            if(i == nfilhos - 1)
                //o ultimo filho fica com o resto
                fim = tl->n_timestamps;

            //o execl so recebe strings e temos de fazer a conversão
            char inicio_str[32];
            char fim_str[32];
            sprintf(inicio_str, "%ld", inicio);
            sprintf(fim_str, "%ld", fim);
            execl("./uds-client", "./uds-client", input, inicio_str, fim_str, NULL);
            perror("nao executou o cliente");
            exit(0);
        }
    }
    // Socket is now set up and bound. Waiting for connections
    int f = open(output, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (f == -1) { perror("File open"); exit(1); }
    for(int i = 0; i < nfilhos; i++) {
        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            perror("accept error");
            continue;
        }

        char linha[128];
        while(read(connfd, linha, 128) > 0) {
            write(f, linha, strlen(linha));
            write(f, "\n", 1);
            //printf("linha: %s\n", linha);
        }
        close(connfd);                                                      // close connection
    }
    close(f);
    return 0;
}
