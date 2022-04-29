//
// Created by so on 4/9/22.
//

#ifndef MPROJETOSO_MAIN_H
#define MPROJETOSO_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <math.h>
#include <signal.h>

#define BUF_SIZE  4096
#define MOMENTOS_REGISTO 5
#define N_SALAS 4

typedef struct timestamps{
    time_t timestamps[MOMENTOS_REGISTO];

    struct timestamps *pnext;
    struct timestamps *pprev;
} TIMESTAMPS;

typedef struct timestamps_list {
    TIMESTAMPS *phead;
    TIMESTAMPS *ptail;
    long n_timestamps;
} TIMESTAMPS_LIST;

void read_timestamps(TIMESTAMPS_LIST * tl, char * path);
void parse_timestamps(TIMESTAMPS_LIST * tl, char * cds);
void print_timestamps(TIMESTAMPS_LIST *tl);

#endif //MPROJETOSO_MAIN_H