#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>        

#ifndef MAXLEN
	#define MAXLEN 64
#endif

#ifndef EXIT
	#define EXIT "SAIR"
#endif

#ifndef SLASH
	#define SLASH '/'
#endif

#ifndef BIN_PATH
	#define BIN_PATH "/bin/"
#endif

#ifndef BIN_SIZE
	#define BIN_SIZE 5
#endif

#ifndef USR_BIN_PATH
	#define USR_BIN_PATH "/usr/bin/"
#endif

#ifndef USR_BIN_SIZE
	#define USR_BIN_SIZE 9
#endif

/* Retrieve available binaries */
char** ls(char path[]) {
	int link[2];
	char buffer[4096];

	pid_t id = fork();

	if (pipe(link)==-1) {
		printf("Tubo falhou. @get_bin");
		return 1;
	}
	if (id == -1) {
		printf("Garfo falhou. @get_bin");
		return 1;
	}
	if(id == 0) {
		dup2 (link[1], STDOUT_FILENO);
		close(link[0]);
		close(link[1]);
		execl("/bin/ls", "ls", path, NULL);
		return 0;
	} else {
		close(link[1]);
		int nbytes = read(link[0], buffer, sizeof(buffer));
		printf("Output: (%.*s)\n", nbytes, buffer);
		wait(NULL);
	}
  return 0;
}

void sig_handler(int signo) {
    if (signo == SIGUSR1)
        printf("SIGNAL: received SIGUSR1\n");
}

int main(int argc, char* argv[]) {

	/* Signal Handling */
	if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
        printf("Falha ao tratar o sinal SIGUSR1\n");
		return 1;
	}

	// Main command
	char cmd[MAXLEN];

	// Number of arguments
	int nargs = 0;

	while (1) {
		// Start shell iteration
		printf("Qual comando desejas tu executar?\n$ ");

		// Receive user command
		scanf("%s", cmd);

		// Checks for user exit
		if (!strcmp(cmd, EXIT)) {
			printf("Compreensivel, que tenhas um bom dia.\n");
			break;
		}

		// Checks for given path
		if (strncmp(cmd, BIN_PATH, BIN_SIZE) && strncmp(cmd, USR_BIN_PATH, USR_BIN_SIZE)){
			if (strchr(cmd, SLASH) != NULL) {
				printf("Executável inválido!\n");
				continue;
			}
		}

		// Ask for number of arguments
		printf("Quantos argumentos desejas tu digitar?\n$ ");
		
		// Read number of arguments
		scanf("%d", &nargs);

		// Dynamically allocate argument vector
		char** args = (char**) malloc(sizeof(char**) * (nargs + 2));

		// Inserts in the beggining of args
		args[0] = strdup(cmd);

		for(int i=1; i<=nargs; i++) {
			// Ask for the i-th argument
			printf("Informe o %dº argumento, fazendo favor.\n$ ", i);
			
			// Allocates memory for the i-th argument
			args[i] = (char*) malloc(sizeof(char*) * MAXLEN);

			// Reads the i-th argument
			scanf("%s", args[i]);

			// Terminates string with NULL
			args[i] = strdup(args[i]);
		}

		// Marks the end of the array
		args[nargs + 1] = NULL;

		//
		pid_t id = fork();

		if (id == -1) { // failure
			printf("O garfo falhou.\n");
			return -1;
		}
		else if (id == 0) { // Child proccess
			// Show POSIX-style formatted command
			printf("$"); for (int i=0; i<=nargs; i++) printf(" %s", args[i]); printf("\n");

			// Execute given command
			execv(cmd, args);

		} else { // Parent proccess
			int id_wait = wait(NULL);
			printf("pid: %d; rc_wait: %d;\n", id, id_wait);
			printf("errno: %d\n", errno);
			return 0;
		}
	}

	return 0;
}
