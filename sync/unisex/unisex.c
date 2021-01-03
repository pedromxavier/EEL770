/******************************************************
 * 6.2 - Banheiro Unissex (The Unisex Bathroom Problem)
 * Aluno: Pedro Maciel Xavier @ ECI/UFRJ
 * 
 * Para este problema, está sendo simulado um dia no 
 * cotidiano da empresa. Cada funcionário possui uma
 * thread específica, conforme a função `employee`.
 * 
 * O problema foi resolvido de maneira generalizada,
 * para uma situação onde cada um dentre M sexos dis-
 * tintos conta com N representantes.
 * 
 * Para simular este cenário de uma maneira razoável,
 * supus que cada pessoa vai diariamente ao banheiro
 * um certo número de vezes dado por uma distribuição
 * de Poisson. Além disso, o tempo que cada um passa
 * dentro do banheiro foi estimado a partir de uma
 * pesquisa inglesa, e supõe-se a distribuição normal
 * 
 * O horário de cada ação de cada funcionário aparece
 * na saída do programa, onde cada hora representa um
 * segundo que se passou.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>
#include "ansi-colors.h"

#ifndef MAX_INSIDE /* Máximo de funcionários dentro do banheiro ao mesmo tempo */
    #define MAX_INSIDE 3 
#endif

#ifndef M /* Número de sexos */
    #define M 2
#endif

#ifndef N /* Número de pessoas de cada sexo */
    #define N 10
#endif

/* Algumas estatísticas sobre ida ao banheiro na Inglaterra
** fonte: 
** https://palife.co.uk/news/taking-some-me-time-workers-take-ten-times-as-long-to-poop-at-work%EF%BB%BF/
** Dados ponderados pela população das cidades analisadas.
*/
#ifndef AVG_WORKING_HOURS /* Expediente médio */
    #define AVG_WORKING_HOURS 8.0 // Horas
#endif

#ifndef AVG_BATHROOM_TIME /* Tempo médio que alguém passa no banheiro ~23 min*/
    #define AVG_BATHROOM_TIME (23.3196 / 60.0)// Horas
#endif

#ifndef STD_BATHROOM_TIME /* Variância do tempo que alguém passa no banheiro ~9 min*/
    #define STD_BATHROOM_TIME (9.03795 / 60.0)// Horas
#endif

#ifndef AVG_BATHROOM_TRIPS /* Numero médio de idas ao banheiro diárias */
    #define AVG_BATHROOM_TRIPS 6.5
#endif

#ifndef AVG_BETWEEN_BATHROOM_TRIPS /* Tempo médio entre idas ao banheiro */
    #define AVG_BETWEEN_BATHROOM_TRIPS (AVG_WORKING_HOURS / 24.0) * AVG_BATHROOM_TRIPS
#endif

#ifndef D_EPSILON /* Precisão do (double)*/
    #define D_EPSILON 2.220446e-16 
#endif

#ifndef TWO_PI
    #define TWO_PI 2.0 * M_PI 
#endif

/** Geração de números aleatórios para fins de
 *  simulação.
 */

double uniform(double a, double b) {
    /** Gera um número aleatório no interval [a, b]
     *  Nota: é um gerador enviesado. No entanto é
     *  bom o suficiente para esta aplicação.
     */
    double x = ((double)rand()) / ((double) RAND_MAX);
    return a + x * (b - a);
}

double normal(double mu, double sigma) {
    /** Distribuição normal gerada a partir de uma
     *  uniforme, pela transformação de Box-Muller
     */
    double x, y;
    do {
        x = uniform(0.0, 1.0);
        y = uniform(0.0, 1.0);
    } while (x <= D_EPSILON);

    double r = sigma * sqrt(-2.0 * log(x));
    double u = r * cos(TWO_PI * y) + mu;
    // double v = r * sin(TWO_PI * y) + mu;
    return u;
}

int poisson(double lambda) {
    /** Gera números inteiros conforme um processo
     *  de Poisson.
     */ 
    double e_l = exp(-lambda);
    double r_u, p_u;
    int r_p;
 
    r_p = -1;
    p_u = 1.;

    do {
        r_u = uniform(0.0, 1.0);
        p_u = p_u * r_u;
        r_p++;
    } while (p_u > e_l);

    return r_p;
}

/* ------------------------------------------ */

// Locks & Variáveis de Condição
pthread_mutex_t pmtx = PTHREAD_MUTEX_INITIALIZER; // print mutex
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // condicional

// Uma thread para cada funcionário
pthread_t threads[M][N];

/** Estrutura para armazenar as informações
 *  de um determinado funcionário
 */
typedef struct {
    int gender, pindex;
    double turn_time;
} person, *pt_person;

/* Diz o momento de início do expediente */
static double work_start;

/* Diz quantas pessoas do sexo i estão no banheiro */
static int inside[M]; 

/** Funções para a contagem de tempo.
 */  
#ifndef WORK_OFFSET // Início do expediente
    #define WORK_OFFSET 9.0
#endif

double sec_clock() {
    /** Relógio que conta segundos
     */
    struct timeval t;

    if (gettimeofday(&t, NULL) == 0) {
        return (double) t.tv_sec + (double) t.tv_usec * 1e-6;
    } else { // Medida inválida
        return 0.0;
    }
}

void clock_printf (const char *format, ...) {
    /** Funciona da mesma forma que printf(const char *format, ...)
     *  mas exibe as "horas" daquilo que imprime.
     */ 
    // STRING BUFFER
    char *string;

    // VARIABLE ARGUMENTS POINTER
    va_list arg_ptr;

    // GET FORMATED STRING SIZE
    va_start(arg_ptr, format);
    size_t n = vsnprintf(NULL, 0, format, arg_ptr);
    va_end(arg_ptr);

    // ALLOCATE BUFFER FOR FORMATED STRING
    string = (char *) malloc(sizeof(char) * (n + 1));
    if (string == NULL) {
        return;   
    }

    // PRINT FORMATED STRING TO VARIABLE
    va_start(arg_ptr, format);
    vsnprintf(string, (n + 1), format, arg_ptr);
    va_end(arg_ptr);

    // GET `WALL` TIME (LITERALLY, FROM THE OFFICE WALL)
    int t_hrs, t_min, t_sec;
    double t = fmod(sec_clock() - work_start, 24.0) + WORK_OFFSET;

    // COMPUTE INTEGER HOURS, MINUTES, SECONDS FROM DECIMAL NUMBER OF HOURS
    t_hrs = (int) t;
    t_min = (int) (60.0 * (t - (double) t_hrs));
    t_sec = (int) (60.0 * ((60.0 * (t - (double) t_hrs)) - (double) t_min));

    // ASK FOR PRINT LOCK
    pthread_mutex_lock(&pmtx);
        printf(GRN"(%02d:%02d:%02d)"RESET"%s", t_hrs, t_min, t_sec, string);
    pthread_mutex_unlock(&pmtx);

    // FREE MEMORY
    free(string);
}

void spin(double t) {
    double t0 = sec_clock();

    // Aguarda `t` segundos enquanto desperdiça alguns ciclos de CPU
    while ((sec_clock() - t0) < t);
}
/* ----------------- */

int available(int gender) {
    /** Verifica se uma pessoa do sexo `gender`
     *  pode entrar no banheiro. Supõe-se que a
     *  thread que chama esta função detem o lock.
     */ 

    /** Verifica primeiro se já constam `MAX_INSIDE`
     *  pessoas do mesmo sexo
     */
    if (inside[gender] < MAX_INSIDE) {
        for (int i = 0; i < M; i++) {
            /** Em seguida, verifica se tem pessoas
             *  de algum outro sexo.
             */
            if ((i != gender) && (inside[i] > 0)) {
                return 0;
            } else {
                continue;
            }
        }
        return 1;
    } else {
        return 0;
    }
}

int thr_do_work(pt_person p) {
    // Marca o início deste turno
    double turn_start = sec_clock();

    clock_printf("[id: %02d, sexo: %d] começou a trabalhar.\n", p->pindex, p->gender);

    int in_turn, at_work;
    if (!isinf(p->turn_time)) {
        do {/* Spin: roda neste loop até que a pessoa queira ir ao
            ** banheiro ou o expediente acabe.
            */
            // Verifica se a pessoa ainda não deseja ir ao banheiro.
            in_turn = ((sec_clock() - turn_start) < p->turn_time);
            // Varifica se o expediente ainda não acabou.
            at_work = ((sec_clock() - work_start) < AVG_WORKING_HOURS);
        } while (in_turn && at_work);
    } else {
        at_work = ((sec_clock() - work_start) < AVG_WORKING_HOURS);
    }
    return at_work;
}

void thr_enter_toilet(pt_person p) {
    pthread_mutex_lock(&mtx);
        clock_printf("[id: %02d, sexo: %d] foi ao banheiro.\n", p->pindex, p->gender);

        /** Aguarda enquanto o banheiro estiver ocupado por uma
         *  ou mais pessoas de sexo distinto ou por mais do que
         *  `MAX_INSIDE` pessoas do mesmo sexo.
        */
        while (!available(p->gender)) { 
            pthread_cond_wait(&cond, &mtx);
        }
        /* Pronto para entrar no banheiro */
        inside[p->gender]++;
  
        clock_printf("[id: %02d, sexo: %d] entrou no banheiro.\n", p->pindex, p->gender);
    pthread_mutex_unlock(&mtx);
}

void thr_do_toilet(pt_person p) {
    /* Representa o tempo que a pessoa `p` passa
    ** 'ocupada' dentro do banheiro.
    */

    // Calcula o tempo gasto no banheiro por amostragem.
    double t = normal(AVG_BATHROOM_TIME, STD_BATHROOM_TIME);

    /* Spin: aguarda o tempo que a pessoa passa
    ** no banheiro.
    */
    if (t > 0.0) spin(t); 
}

void thr_exit_toilet(pt_person p) {
    // Solicitará o lock
    pthread_mutex_lock(&mtx);
        // Obteve o lock
    
        // Libera o recurso
        inside[p->gender]--;
        // Sinaliza a liberação de um recurso.
        pthread_cond_signal(&cond);

        clock_printf("[id: %02d, sexo: %d] saiu do banheiro.\n", p->pindex, p->gender);
      
        // Soltará o lock
    pthread_mutex_unlock(&mtx);
    // Lock liberado
}

void *employee(void *arg) {
    // Identificação da pessoa
    pt_person p = (pt_person) arg;
    
    clock_printf("[id: %02d, sexo: %d] chegou ao trabalho.\n", p->pindex, p->gender);

    for(;;) {
        // Repete enquanto ainda estiver no horário do expediente
        if (thr_do_work(p)) {
            // Funcionário vai ao banheiro
            thr_enter_toilet(p);
            // Faz o que tem que fazer
            thr_do_toilet(p);
            // Funcionário sai do banheiro
            thr_exit_toilet(p);
            continue;
        } else { // Funcionário vai para casa, 'apertado' ou não.   
            clock_printf("[id: %02d, sexo: %d] foi para casa.\n", p->pindex, p->gender);
            break;
        }
    }
    return NULL;
}

void work_day() {
    /** Representa um dia de trabalho numa determinada
     *  empresa, onde há um banheiro unissex.
     */

    // M * N funcionários, de M sexos distintos.
    person people[M][N];

    // Horário de início do expediente
    work_start = sec_clock();

    for (int j = 0; j < N; j ++) { // Para cada pessoa
        for (int i = 0; i < M; i ++) { // De um determinado sexo
            // Identificação da pessoa
            people[i][j].pindex = i * N + j;
            // Sexo da pessoa
            people[i][j].gender = i;

            /* Tempo entre cada ida ao banheiro no dia obtido
            ** dividindo o número de horas trabalhadas por dia
            ** pelo número de vezes que a pessoa vai ao banheiro.
            */ 
            people[i][j].turn_time = AVG_WORKING_HOURS / poisson(AVG_BATHROOM_TRIPS);

            // Cria a thread.
            pthread_create(&(threads[i][j]), NULL, employee, &(people[i][j]));
        }
    }

    // Coleta as threads
    for (int j = 0; j < N; j ++) { // Para cada pessoa
        for (int i = 0; i < M; i ++) { // De um determinado sexo
            pthread_join(threads[i][j], NULL);
        }
    }
}

// Função principal
int main(int argc, char* argv[]) {
    // Simula um dia tranquilo de trabalho.
    work_day();
    return 0;
}