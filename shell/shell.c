#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "colorf.h"

#ifndef SHELL_BANNER
	#define SHELL_BANNER \
"    _____ _    _ ______ _      _         \n"\
"   / ____| |  | |  ____| |    | |        \n"\
"  | (___ | |__| | |__  | |    | |        \n"\
"   \\___ \\|  __  |  __| | |    | |        \n"\
"   ____) | |  | | |____| |____| |____    \n"\
"  |_____/|_|  |_|______|______|______|   \n"\
"     Pedro Maciel Xavier - ECI/UFRJ      \n"
#endif

#ifndef PID_BANNER
	#define PID_BANNER \
"   :::::::::: pid = %4d :::::::::::     \n"
#endif

#ifndef RESET_BANNER
	#define RESET_BANNER \
"   ::::::::::::: RESET :::::::::::::     \n"
#endif

#ifndef END_BANNER
	#define END_BANNER \
"       ______ _____ __  __   \n"\
"      |  ____|_   _|  \\/  |  \n"\
"      | |__    | | | \\  / |  \n"\
"      |  __|   | | | |\\/| |  \n"\
"      | |     _| |_| |  | |_ \n"\
"      |_|    |_____|_|  |_|  \n"
#endif

#ifndef ENDL
	#define ENDL "\n"
#endif

#ifndef EOL
	#define EOL '\n'
#endif

#ifndef MAXLEN
	#define MAXLEN 128
#endif

#ifndef EXIT
	#define EXIT "SAIR"
#endif

#ifndef OK
	#define OK 0
#endif

#ifndef ERROR_CODE
	#define ERROR_CODE 1
#endif

#ifndef EXIT_CODE
	#define EXIT_CODE 2
#endif

#ifndef LOOP_CODE
	#define LOOP_CODE 3
#endif

#ifndef PROMPT
    #define PROMPT "$ "
#endif

void clear_stdin(void) {
    for(int chr = 0; chr != EOL && chr != EOF; chr = getchar());
}

int loop_step;
pid_t child_pid;

int exec_cmd(int argc, char *argv[]) {
	// Incrementa o contador, 6º Passo.
	loop_step++;

	// Realiza o Fork.
	pid_t id = fork();

	if (id == -1) { // Falha no Fork
		printf("O garfo falhou.\n");
		return ERROR_CODE;
	}
	else if (id == 0) { // Processo Filho
		/* Mostra o comando completo, formatado como se fosse digitado
		** na linha de comando.
		*/
		printf(PROMPT); for (int i=0; i<=argc; i++) printf("%s ", argv[i]); printf(ENDL);

		/* Executa o comando completo.
		*/
		if (execvp(argv[0], argv)) {
			printf("Comando Inválido :( lamento.\n");
			return EXIT_CODE;
		}

		/* Esta porçao do código nao deve ser executada JAMAIS.
		*/
		printf(ENDL);
		printf("PROBLEMAO");
		printf(ENDL);
		return ERROR_CODE;
	}
	else { // Processo Pai
		// Incrementa o contador, 7º Passo
		loop_step++;

		// Variável para guardar o status do processo Filho
		int status;

		// Aguarda o fim do processo Filho
		int pid_wait = wait(&status);

		if (pid_wait >= 0) { // Sucesso
			if (WIFEXITED(status)) {
				switch(WEXITSTATUS(status)) {
					case OK:
						return OK;
						break;
					default:
						return ERROR_CODE; 
						break;
				}
			}
			else {
				return ERROR_CODE;
			}
		}
		else {
			printf("Erro ao aguardar Processo Filho.\n");
			return ERROR_CODE;
		}
	}
}

int free_argv(int argc, char *argv[]) {
	for(int i=0; i<=argc; i++) {
		free(argv[i]);
	}
	free(argv);

	return 0;
}

int read_argv(int argc, char *argv[]) {

	char *arg = (char*) malloc(sizeof(char) * MAXLEN);
	if (arg == NULL) { // Falha na alocaçao dinâmica
		printf("Falha ao alocar memória.");
		return ERROR_CODE;
	}

	// Incrementa o contador, 5º Passo.
	loop_step++;

	for(int i=1; i<=argc; i++) {
		// Solicita o i-ésimo argumento.
		printf("Informe o %dº argumento, fazendo favor.\n", i); printf(PROMPT);

		// Lê o i-ésimo argumento.
		scanf("%s", arg);

		// Termina a string com o caracter '\0'
		argv[i] = strdup(arg);
	}

	// Marca o fim do array com NULL
	argv[argc + 1] = NULL;

	// Libera memória
	free(arg);

	return 0;
}

int read_cmd(char *cmd, int *argc) {
	// Incrementa o contador, 1º Passo.
	loop_step++;

	// Inicia a Interaçao com o Usuário.
	printf("Qual comando tu desejas executar?\n"); printf(PROMPT);

	// Incrementa o contador, 2º Passo.
	loop_step++;

	// Lê o comando do usuário.
	scanf("%s", cmd);

	/* Verifica se o usuário digitou `SAIR`
	** Isso foi pensado para o uso do shell em loop, permitindo sair
	** da repetiçao sem fazer uma chamada exit.
	*/
	if (!strcmp(cmd, EXIT)) {
		printf("Compreensivel, que tenhas um bom dia.\n");
		return EXIT_CODE;
	}

	// Incrementa o contador, 3º Passo.
	loop_step++;

	// Pergunta pelo número de argumentos.
	printf("Quantos argumentos tu desejas digitar?\n");
	printf(PROMPT);

	// Incrementa o contador, 4º Passo.
	loop_step++;

	// Lê o número de argumentos
	scanf("%d", argc);

	return 0;
}

int shell_loop(void) {
	int argc;
	char cmd[MAXLEN];

	// Reinicia o contador de passos
	loop_step = 0;

	switch (read_cmd(cmd, &argc)) {
		case OK: // Continua
			break;
		case EXIT_CODE: // Código de saida
			return EXIT_CODE;
			break;
		default: // Comportamento inesperado
			return ERROR_CODE;
			break;
	}

	// Dynamically allocate argument vector
	char** argv = (char**) malloc(sizeof(char**) * (argc + 2));

	if (argv == NULL) { // Falha na alocaçao dinâmica
		printf("Falha ao alocar memória.");
		return ERROR_CODE;
	}

	// Insere o nome do comando no inicio do array
	argv[0] = strdup(cmd);

	// Captura os argumentos e insere-os no array
	switch (read_argv(argc, argv)) {
		case OK: // Continua
			break;
		default: // Comportamento inesperado
			free_argv(argc, argv);
			return ERROR_CODE;
			break;
	}

	// Executa o comando completo
	switch (exec_cmd(argc, argv)) {
		case OK: // Continua
			break;
		default: // Comportamento inesperado
			free_argv(argc, argv);
			return ERROR_CODE;
			break;
	}

	// Successo, libera a memória alocada
	free_argv(argc, argv);
	return OK;
}

void child_signal_handler(int signo) {
	/* Termina o Processo Filho caso este esteja entre os passos
	** 2 e 5 do loop.
	*/
	if (signo == SIGUSR1 && (2 <= loop_step && loop_step <= 5)) {
		// Termina o Processo Filho indicando uma nova iteraçao
		printf(ENDL);
		exit(LOOP_CODE);
	}
}

void parent_signal_handler(int signo) {
	/* Quando o pai receber o sinal SIGUSR1, ele deve enviar um sinal
	** para interromper o processo filho.
	*/
    if (signo == SIGUSR1) {
		if (child_pid > 0) { // Processo filho ativo.
        	kill(child_pid, signo);
		}
	}
}

int shell_main(void) {
	/* Exibe a apresentação
	*/
	colorf(SHELL_BANNER);

	colorf(PID_BANNER, getpid());
	for (;;) { // Repetiçao indeterminada.
		// Redefine o pid do Processo Filho
		child_pid = -1;

		// Redefine o passo do loop
		loop_step = 0;

		// Realiza o Fork.
		pid_t pid = fork();

		if (pid == -1) { // Falha no Fork
			printf("O garfo falhou.\n");
			return ERROR_CODE;
		}
		else if (pid == 0) { // Processo Filho
			// Define o tratamento do sinal SIGUSR1 no Processo Filho
			if (signal(SIGUSR1, child_signal_handler) == SIG_ERR) {
				printf("@child Nao foi possivel tratar o sinal `SIGUSR1`.\n");
				return ERROR_CODE;
			}

			switch (shell_loop()) {
				case OK:
					return OK;
					break;
				case EXIT_CODE:
					return EXIT_CODE;
					break;
				default:
					return ERROR_CODE;
					break;
			}
		}
		else { // Processo Pai
			// Define o tratamento do sinal SIGUSR1 no Processo Pai
			if (signal(SIGUSR1, parent_signal_handler) == SIG_ERR) {
				printf("@parent Nao foi possivel tratar o sinal `SIGUSR1`.\n");
				return ERROR_CODE;
			}

			// Armazena de forma global o pid do Processo Filho
			child_pid = pid;

			// Cria variável para receber o status do Processo Filho
			int status;

			// Aguardando o fim do Processo Filho
			int pid_wait = wait(&status);

			if (pid_wait >= 0) { // Sucesso
				if (WIFEXITED(status)) {
					switch(WEXITSTATUS(status)) {
						case OK:
							colorf(END_BANNER);
							return OK;
							break;
						case EXIT_CODE:
							colorf(END_BANNER);
							return OK;
							break;
						case LOOP_CODE:
							break;
						default:
							return ERROR_CODE;
							break;
					}
					// Apresenta a reinicialização
					colorf(RESET_BANNER);
					continue; // Retorna ao primeiro passo
				}
				else {
					colorf(END_BANNER);
					return OK;
				}
			}
			else {
				printf("Erro ao aguardar Processo Filho.\n");
				return ERROR_CODE;
			}
		}
	}
}