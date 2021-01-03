/******************************************************
 * 5.7 - Travessia do Rio (River Crossing Problem)
 * Aluno: Pedro Maciel Xavier @ ECI/UFRJ
 * 
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ansi-colors.h"

#ifndef EMPLOYEES // Número de funcionários de cada empresa
    #define EMPLOYEES 100
#endif

#ifndef BOAT_SLOTS // Assentos no barco
    #define BOAT_SLOTS 4
#endif

#ifndef HALF_BOAT_SLOTS // Metade dos assentos no barco
    #define HALF_BOAT_SLOTS 2
#endif

#ifndef HACKER 
    #define HACKER 0
#endif

#ifndef SERF 
    #define SERF 1
#endif


/* Coisas de Thread */
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t hacker_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t serf_cond = PTHREAD_COND_INITIALIZER;

pthread_cond_t boat_cond = PTHREAD_COND_INITIALIZER;

pthread_t threads[2][EMPLOYEES];

int hackers = 0, serfs = 0;
int tickets[2] = {0, 0}, next_ride[2] = {0, 0};
int on_board = 0, boarding = 0, shared = 0;

void rowBoat() {
    // Finish boarding
    printf(MAG"O BOTE ZARPOU!"RESET"\n\n");
}

void board() {
    on_board++;
    printf(MAG"O Bote está com %d tripulantes."RESET"\n", on_board);
}

void *hacker(void *arg) {
    /** Hacker do Linux
     */
    int sudo = 0;

    pthread_mutex_lock(&mtx);
        while (boarding) {
            // Não entra na fila se alguém estiver embarcando
            // evita 2 sudoers
            pthread_cond_wait(&boat_cond, &mtx);
        }

        // Incrementa o número de hackers na espera
        hackers++;

        // Tira uma senha e incrementa o contador de bilhetes hacker
        int ticket = tickets[HACKER]++;

        if (hackers == BOAT_SLOTS) { // Somente os hackers no bote
            // ~$ sudo boat
            sudo = 1;

            // Incrementa a lista de quem pode ir no próximo barco
            next_ride[HACKER] += BOAT_SLOTS;

            // Inicia o embarque
            boarding = 1; shared = 0;

            printf(GRN"SUDO HACKER ["RESET"%02d"GRN"]"RESET" iniciou o embarque.\n", ticket);
        } else if ((hackers == HALF_BOAT_SLOTS) && (serfs >= HALF_BOAT_SLOTS)) { // Bote compartilhado
            // ~$ sudo boat
            sudo = 1;

            // Incrementa a lista de quem pode ir no próximo barco
            next_ride[HACKER] += HALF_BOAT_SLOTS;
            next_ride[SERF] += HALF_BOAT_SLOTS;

            // Reinicia a contagem de hackers e desconta os servos
            hackers = 0;
            serfs -= HALF_BOAT_SLOTS;

            // Inicia o embarque
            boarding = 1; shared = 1;

            printf(GRN"SUDO HACKER ["RESET"%02d"GRN"]"RESET" iniciou o embarque.\n", ticket);
        } else {
            while (next_ride[HACKER] <= ticket) {
                printf(GRN"HACKER ["RESET"%02d"GRN"]"RESET" está aguardando.\n", ticket);
                // Aguarda até que seu ticket permita o embarque
                pthread_cond_wait(&hacker_cond, &mtx);
            }
        }

        printf(GRN"HACKER ["RESET"%02d"GRN"]"RESET" embarcou.\n", ticket);
        board();

        

        // Avisa para os outros que embarquem
        pthread_cond_signal(&hacker_cond);
        if (shared) pthread_cond_signal(&serf_cond);

        if (sudo) {
            // Aguarda que todos embarquem
            while (on_board < BOAT_SLOTS) {
                pthread_cond_wait(&boat_cond, &mtx);
            }
            rowBoat();
            on_board = 0;
            boarding = 0;
        } 

        pthread_cond_signal(&boat_cond);

        hackers--;
    pthread_mutex_unlock(&mtx);
    
    return NULL;
}

void *serf(void *arg) {
    /** Mané da Microsoft
     */
    int admin = 0;

    pthread_mutex_lock(&mtx);
        while (boarding) {
            pthread_cond_wait(&boat_cond, &mtx);
        }

        // Tira uma senha e incrementa o contador servil
        int ticket = tickets[SERF]++;

        // Incrementa o número de servos na espera
        serfs++;

        if (serfs == BOAT_SLOTS) { // Somente servos no bote
            // C:\Windows\system32> boat
            admin = 1;

            // Incrementa a lista de quem pode ir no próximo barco
            next_ride[SERF] += BOAT_SLOTS;

            // Reinicia a contagem de servos
            serfs = 0;

            // Inicia o embarque
            boarding = 1; shared = 0;

            // Embarca
            printf("Um "RED"SERVO ADMIN"RESET" embarcando. bilhete no.%03d \n", ticket);
            
            board();

            // Manda mensagem para os servos
            pthread_cond_signal(&serf_cond);

        } else if ((serfs == HALF_BOAT_SLOTS) && (hackers >= HALF_BOAT_SLOTS)) { // Bote compartilhado
            // C:\Windows\system32> boat
            admin = 1;

            // Incrementa a lista de quem pode ir no próximo barco
            next_ride[SERF] += HALF_BOAT_SLOTS;
            next_ride[HACKER] += HALF_BOAT_SLOTS;

            // Reinicia a contagem de servos e desconta os hackers
            serfs = 0;
            hackers -= HALF_BOAT_SLOTS;

            // Inicia o embarque
            boarding = 1; shared = 1;

            // Embarca
            printf("Um "RED"SERVO ADMIN"RESET" embarcando. bilhete no.%03d \n", ticket);
            
            board();

            // Manda mensagem para todo mundo
            pthread_cond_signal(&serf_cond);
            pthread_cond_signal(&hacker_cond);

        } else {
            while (next_ride[SERF] <= ticket) {
                printf("Um "RED"SERVO"RESET" está esperando com seu bilhete no.%03d \n", ticket);
                
                // Aguarda até que seu ticket permita o embarque
                pthread_cond_wait(&serf_cond, &mtx);
            }
            printf("Um "RED"SERVO"RESET" embarcando. bilhete no.%03d \n", ticket);
            
            board();

            pthread_cond_signal(&serf_cond);

            if (shared) {
                pthread_cond_signal(&hacker_cond);
            }

            pthread_cond_signal(&boat_cond);
        }
        
        if (admin) {
            // Aguarda que todos embarquem
            while (on_board < BOAT_SLOTS) {
                pthread_cond_wait(&boat_cond, &mtx);
            }
            printf(MAG"O BOTE VAI ZARPAR!"RESET"\n");
            
            rowBoat();
        }
        printf(RED"SERVO"RESET" dentro do barco. bilhete no.%03d \n", ticket);
        
    pthread_mutex_unlock(&mtx);

    return NULL;
}

void river_crossing() {
    
    // Cria as threads
    for (int i = 0; i < EMPLOYEES; i++) {
        pthread_create(&(threads[HACKER][i]), NULL, hacker, NULL);
        // pthread_create(&(threads[SERF][i]), NULL, serf, NULL);
    }

    // Coleta as threads
    for (int i = 0; i < EMPLOYEES; i++) {
        pthread_join(threads[HACKER][i], NULL);
        // pthread_join(threads[SERF][i], NULL);
    }
}

int main(int argc, char* argv[]) {

    // Bota o pessoal para atravessar o rio
    river_crossing();

    return 0;
}