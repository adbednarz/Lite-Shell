#include <stdio.h>
#include <string.h>
#include <stdlib.h> //malloc(), free(), execvp()
#include <unistd.h> //chdir(), fork(), pid_t
#include <sys/wait.h>
#include <fcntl.h>

int flagP = 0;
int flag = 0;

char **globalTable;

void cntl_c_handler(int dummy) {
		
}

char **getFromLine(char *line, char *c) {
	int bufsize = 5;
	int counter = 0;
//malloc allocates the requested memory and returns a pointer to it, sizof(char*) is a size of a pointer
	char **parameters = malloc(bufsize * sizeof(char*));
	char *parameter;

	if (!parameters) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(1);
	}

	parameter = strtok(line, c);
	while (parameter != NULL) {
		parameters[counter] = parameter;
		counter++;
	
		if(counter >= bufsize) {
			bufsize += 64;
			parameters = realloc(parameters, bufsize * sizeof(char*));
			if (!parameters) {
				fprintf(stderr, "lsh: allocation error\n");
				exit(1);
			}
		}
//passing a NULL value, means asking to continue tokenizing the same thing as before
		parameter = strtok(NULL, c);
	}
	parameters[counter] = NULL;
	return parameters;
}

void addPid(char *line) {
	int i = 0;
	while(globalTable[i] != NULL) {
		i++;
	}
	globalTable[i] = line;
}

void cntl_z_handler(int dummy) {
	if(globalTable[0] != NULL && flag == 1) {
		int q = 0;
		pid_t x;
		while(globalTable[q] != NULL) {
			q++;
		}
		char *temp = malloc(10*sizeof(char));
		strcpy(temp, globalTable[q - 1]);
		char *pid = strtok(temp, " ");
		sscanf(pid, "%u", &x);
		kill(x, SIGTSTP);
	}
}

void deletePid(pid_t pid) {
	int i = 0;
	char temppid[10];
	sprintf(temppid, "%d", pid);
	char *ppid = temppid;
	

	while(globalTable[i] != NULL) {
		if(strstr(globalTable[i], ppid)) {
			while(globalTable[i + 1] != NULL) {
				globalTable[i] = globalTable[i + 1];
				i++;
			}
			globalTable[i] = NULL;
		}
		i++;
	}
}

void sig_handler(int dummy) {
	if(flag == 0) {
		pid_t pid;
   		int status;
		waitpid(pid, &status, 0);
		if(WIFEXITED(status)) {
			deletePid(pid);
		}
		if(WIFSIGNALED(status)) {
			deletePid(pid);
		}
	}
}

int drawGlobalTable() {
	int i = 0;
	while(globalTable[i] != NULL) {
		printf("%s\n", globalTable[i]);
		i++;
	}
	return 1;
}

char *getCommand() {
	char *line = NULL;
//representing the size of an allocated block of memory
	ssize_t bufsize = 0;
	ssize_t line_size = 0;
	line_size = getline(&line, &bufsize, stdin);
	if(line_size == -1) {
		exit(0);
	}
	return line;
}

char *deleteampersand(char *line) {
	flagP = 1;
	int len = strlen(line);
	line[len - 2] = '\0';
	return line;
}



int cdLsh(char **args) {
	if(args[1] == NULL) {
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	} else {
		int i = chdir(args[1]);
		if(i != 0) {
			perror("lsh");
		}
	}
	return 1;
}

int exitLsh(char **args) {
	exit(0);
}

int process(char **args) {
	int i = 0;
	pid_t pid;
	pid_t pidd;
	int state;
	int status;
	char *line = malloc(10 * sizeof(char*));
	if(flagP == 0)
		flag = 1;
	pid = fork();
	if(pid == 0) {
		state = execvp(args[0], args);
		if(state == -1) {
			perror("lsh");
		}
		exit(0);
	} else if (pid < 0) {
		perror("lsh");
	} else {
		char temppid[10];
		sprintf(temppid, "%d", pid);
		char *ppid = temppid;
		line = strcat(line, ppid);
		while(args[i] != NULL) {
			line = strcat(line, " ");
			line = strcat(line, args[i]);
			i++;
		}
		addPid(line);
		if(flagP == 0) {
			while(1) {
				pidd = waitpid(-1, &status, WUNTRACED);
				if(WIFEXITED(status)) {
					deletePid(pidd);
				}
				if(WIFSTOPPED(status)) {
					int i = 0;
					while(globalTable[i] != NULL) {
						if(strstr(globalTable[i], ppid)) {
							break;
						}
						i++;
					}
					if(!(strstr(globalTable[i], "STOPPED")))
						globalTable[i] = strcat(globalTable[i], " STOPPED");
				}
				if(WIFSIGNALED(status)) {
					deletePid(pidd);
				}
				if(pidd == pid)
					break;
			}
			flag = 0;
		}
	}
	return 1;
}

int processP(char **args) {
	int counterPipes = 0;
	pid_t pid;
	pid_t pidd;
	pid_t theLastPid;
	int state;
	int status;
	//zlicze ile potokow jest w zapytaniu
	while(args[counterPipes] != NULL) {
		counterPipes = counterPipes + 1;
	}
	counterPipes = counterPipes - 1; // potokow jest o 1 mniej niz elementow
	int pipefds[2*counterPipes];
	if(pipefds[2])	{
	}
	//tworzy wszystkie potoki
	for(int s = 0; s < counterPipes; s++) {
		if(pipe(pipefds + s*2) < 0) {
			exit(1);
		}
	}
	if(flagP == 0)
		flag = 1;
	for(int i = 0; i < counterPipes + 1; i++) { //iteruje po wszystkich elementach
		char *arg = args[i];
		char **argss = getFromLine(arg, " \t\n");
		pid = fork();
		if(pid == 0) {
			if(i != 0) {	//nie jest pierwszym elementem
				if(dup2(pipefds[(i - 1) * 2], 0) < 0) { //zamiana wczytywania z stdin na wyjscie pipe
                		exit(1);
            	}
			}
			if(i != counterPipes) { //nie jest ostatnim elementem
				if(dup2(pipefds[i*2 + 1], 1)  < 0) { //zamiana wpisywania do stdout na wejscie pipe
					exit(1);
				}
			}
			for(int l = 0; l < 2 * counterPipes; l++ ) {
    				close( pipefds[l] );
			}
			
			int j = 0;
			while(argss[j] != NULL) {
				if(strstr(argss[j], "2>")) {
					int fd = open(argss[j + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
					dup2(fd, 2);
					close(fd);
					int k = j;
					while(argss[k + 2] != NULL)	{
						argss[k] = argss[k + 2];
						k++;
					}
					argss[k] = NULL;
					j--; // w sprawdzone miejsce pojawia sie nowy element
				}
				if(strstr(argss[j], "<")) {
					int fd = open(argss[j + 1], O_RDWR, S_IRUSR | S_IWUSR);
					if(dup2(fd, 0) != 0) {
						printf("No such file or directory: %s", args[j + 1]);
						exit(1);
					}
					close(fd);
					int k = j;
					while(argss[k + 2] != NULL)	{
						argss[k] = argss[k + 2];
						k++;
					}
					argss[k] = NULL;
					j--; // w sprawdzone miejsce pojawia sie nowy element
				}
				j++;
			}
			state = execvp(argss[0], argss);
			if(state == -1) {
				perror("lsh");
			}
		} else if(pid < 0) {
			perror("lsh");
		} else {
			if(i == counterPipes)
				theLastPid = pid;
		}
	}
	for(int l = 0; l < 2 * counterPipes; l++ ){
			close( pipefds[l] );
	}
	if(flagP == 0) {
		while(1) {
			pidd = waitpid(-1, &status, WUNTRACED);
			if(WIFEXITED(status)) {
				deletePid(pidd);
			}
			if(pidd == theLastPid)
				break;
		}
		flag = 0;
	}
	return 1;
}
	
int processIO(char **args) {
	int state;
	int status;
	pid_t pid;
	pid_t pidd;
	if(flagP == 0)
		flag = 1;
	pid = fork();
	if(pid == 0) {
		int j = 0;
		while(args[j] != NULL) {
			if(strstr(args[j], "2>")) {
				int fd = open(args[j + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(fd, 2);
				close(fd);
				int k = j;
				while(args[k + 2] != NULL)	{
				args[k] = args[k + 2];
					k++;
				}
				args[k] = NULL;
				j--; // w sprawdzone miejsce pojawia sie nowy element
			}
			if(strstr(args[j], "<")) {
				int fd = open(args[j + 1], O_RDWR, S_IRUSR | S_IWUSR);
				if(dup2(fd, 0) != 0) {
					printf("No such file or directory: %s\n", args[j + 1]);
					exit(1);
				}
				close(fd);
				int k = j;
				while(args[k + 2] != NULL)	{
					args[k] = args[k + 2];
					k++;
				}
				args[k] = NULL;
				j--; // w sprawdzone miejsce pojawia sie nowy element
			}
			if(strstr(args[j], ">")) {
				int fd = open(args[j + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
				dup2(fd, 1);
				close(fd);
				int k = j;
				while(args[k + 2] != NULL)	{
					args[k] = args[k + 2];
					k++;
				}
				args[k] = NULL;
				j--; // w sprawdzone miejsce pojawia sie nowy element
			}
			j++;
		}
		state = execvp(args[0], args);
		if(state == -1) {
			perror("lsh");
		}
	} else if(pid < 0) {
		perror("lsh");
	} else {
		if(flagP == 0) {
			while(1) {
				pidd = waitpid(-1, &status, WUNTRACED);
				if(WIFEXITED(status)) {
					deletePid(pidd);
				}
				if(pidd == pid)
					break;
			}
			flag = 0;
		}
	}
	return 1;
}

int action(char **args) {
	int i;
	if(args[0] == NULL) {
		return 1;
	}
	if(strcmp(args[0], "cd") == 0) {
		return cdLsh(args);
	} else if(strcmp(args[0], "exit") == 0) {
		return exitLsh(args);
	} else if(strcmp(args[0], "jobs") == 0) {
		return drawGlobalTable();
	} else if(strcmp(args[0], "fg") == 0) {
		int status;
		if(args[1] != NULL && globalTable[0] != NULL) {
			flag = 1;
			pid_t x;
			pid_t pidd;
			sscanf(args[1], "%u", &x);
			kill(x, SIGCONT);
			while(1) {
				pidd = waitpid(-1, &status, WUNTRACED);
				if(WIFEXITED(status)) {
					deletePid(pidd);
				}
				if(WIFSTOPPED(status)) {
					char temppid[10];
					sprintf(temppid, "%d", pidd);
					char *ppid = temppid;
					int i = 0;
					while(globalTable[i] != NULL) {
						if(strstr(globalTable[i], ppid)) {
							break;
						}
						i++;
					}
					if(!(strstr(globalTable[i], "STOPPED")))
						globalTable[i] = strcat(globalTable[i], " STOPPED");
				}
				if(WIFSIGNALED(status)) {
					deletePid(pidd);
				}
				if(pidd == x)
					break;
			}
			flag = 0;
		} else if(globalTable[0] != NULL) {
			int q = 0;
			flag = 1;
			pid_t x;
			pid_t pidd;
			while(globalTable[q] != NULL) {
				q++;
			}
			char *temp = malloc(10*sizeof(char));
			strcpy(temp, globalTable[q - 1]);
			char *pid = strtok(temp, " ");
			sscanf(pid, "%u", &x);
			kill(x, SIGCONT);
			while(1) {
				pidd = waitpid(-1, &status, WUNTRACED);
				if(WIFEXITED(status)) {
					deletePid(pidd);
				}
				if(WIFSTOPPED(status)) {
					char temppid[10];
					sprintf(temppid, "%d", pidd);
					char *ppid = temppid;
					int i = 0;
					while(globalTable[i] != NULL) {
						if(strstr(globalTable[i], ppid)) {
							break;
						}
						i++;
					}
					if(!(strstr(globalTable[i], "STOPPED")))
						globalTable[i] = strcat(globalTable[i], " STOPPED");
				}
				if(WIFSIGNALED(status)) {
					deletePid(pidd);
				}
				if(pidd == x)
					break;
			}
			flag = 0;
		} else {
			printf("current: no such job\n");	
		}
		return 1;
	} else if(strcmp(args[0], "bg") == 0) {
		if(args[1] != NULL) {
			pid_t x;
			sscanf(args[1], "%u", &x);
			kill(x, SIGCONT);
			flag = 1;
		} else if(globalTable[0] != NULL) {
			int q = 0;
			pid_t x;
			while(globalTable[q] != NULL) {
				q++;
			}
			char *temp = malloc(10*sizeof(char));
			strcpy(temp, globalTable[q - 1]);
			char *pid = strtok(temp, " ");
			sscanf(pid, "%u", &x);
			kill(x, SIGCONT);
			flag = 1;
		} else {
			printf("current: no such job\n");	
		}
		return 1;
	} else {
		return process(args);
	}
}

int main() {
	char *line;
	char **args;
	char *p;
	char *s;
	char *t;
	char *u;
	char *buf;
	buf=(char *)malloc(100*sizeof(char));
	int len;
	int state;
	signal(SIGINT, cntl_c_handler);
	signal(SIGCHLD, sig_handler);
	signal(SIGTSTP, cntl_z_handler);
	globalTable = malloc(10*sizeof(char*));

	do {
		getcwd(buf, 100);
		printf("\033[38;5;2m%s: ", buf);
		printf("\033[38;5;255mlsh>");
		line = getCommand();
		p = strstr(line, "&");
		s = strstr(line, "|");
		t = strstr(line, ">");
		u = strstr(line, "<");
		if(p) {
			line = deleteampersand(line);
		}
		if(s) { // pipes
			args = getFromLine(line, "|");
			state = processP(args);
		} else if(t || u) {
			args = getFromLine(line, " \t\n");
			state = processIO(args);
		} else {
			args = getFromLine(line, " \t\n");
			state = action(args);	
		}
		free(line);
		free(args);
		flagP = 0;	
	} while (state);
	return 0;
}
