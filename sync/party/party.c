/*************************************************************
 *  7.3 - Problema da festa no quarto (The Room Party Problem)
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ansi-colors.h"

#ifndef N // Número de alunos
    #define N 200
#endif

#ifndef M // Número de quartos
    #define M 10
#endif

#ifndef LIMIT // Limite mínimo para que o decano entre no quarto
    #define LIMIT 5
#endif

#ifndef SLEEP_P // 
    #define SLEEP_P 0.2
#endif

double uniform(double a, double b) {
    /** Gera um número aleatório no interval [a, b]
     *  Nota: é um gerador enviesado. No entanto é
     *  bom o suficiente para esta aplicação.
     */
    double x = ((double)rand()) / ((double) RAND_MAX);
    return a + x * (b - a);
}

/* Coisas de thread */
pthread_mutex_t pmtx = PTHREAD_MUTEX_INITIALIZER; // print mutex
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // condicional

// As primeiras `M` threads são para os alunos e a última para o decano.
pthread_t student_threads[N];
pthread_t dean_thread;

static int students_awake;
static int dean_room = -1;
static int rooms[M];

void show_rooms() {
    printf("|");
    for (int i = 0; i < M; i++) {
        if (rooms[i] < LIMIT) {
            printf(GRN"%3d"RESET"|", rooms[i]);
        } else if (rooms[i] == LIMIT) {
            printf(YEL"%3d"RESET"|", rooms[i]);
        } else {
            printf(RED"%3d"RESET"|", rooms[i]);
        }
    }
    printf("\n");
}

void *student(void *arg) {
    /**
     * 
     */
    int room;
    pthread_mutex_lock(&mtx);
        do { // Enquanto acordado
            // Aluno quer festa
            for (int i = 0; i < M; i++) {
                if (i != dean_room) {
                    room = i;
                    break;
                }
            }
            rooms[room]++;
            show_rooms();
            pthread_cond_signal(&cond);
            // Festa!
            while (dean_room != room) {
                pthread_cond_wait(&cond, &mtx);
            }
            // Fim de Festa
            rooms[room]--;
            room = -1;
            pthread_cond_signal(&cond);
        } while (uniform(0.0, 1.0) < SLEEP_P);
        students_awake--;
    pthread_mutex_unlock(&mtx);
    pthread_cond_signal(&cond);
    return NULL;
}

void *dean(void *arg) {
    /**
     * 
     **/
    pthread_mutex_lock(&mtx);
    while (students_awake > 0) {
        for (int i = 0; i < M; i++) {
            if ((rooms[i] > LIMIT) || (rooms[i] == 0)) {
                dean_room = i;
                pthread_cond_signal(&cond);
            }
        }
        show_rooms();
        pthread_cond_wait(&cond, &mtx);
        pthread_cond_signal(&cond); 
    }
    pthread_mutex_unlock(&mtx);

    return NULL;    
}

void common_night() {
    // Alunos estão todos acordados
    students_awake = N;

    // Cria a thread do decano
    pthread_create(&dean_thread, NULL, dean, NULL);

    // Cria as threads dos alunos
    for (int i = 0; i < N; i++) {
        pthread_create(&(student_threads[i]), NULL, student, NULL);
    }

    // Coleta as threads
    for (int i = 0; i < N; i++) {
        pthread_join(student_threads[i], NULL);
    }

    pthread_join(dean_thread, NULL);
}



int main(int argc, char *argv[]) {

    common_night();

    return 0;
}