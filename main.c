#include <bits/posix2_lim.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define HIST_MAX 5

typedef struct{
	char *cmd_path;
	char *command;
	char *args[20];
	int num_args;
} parse_object;


parse_object *command_history[HIST_MAX];
int last_cmd = 0;

void push_cmd(parse_object *);
parse_object *pop_cmd(void);
void print_hist(void);

parse_object *parse_token(char *);
int find_cmd(parse_object *);
void del_po(parse_object *);
void print_prompt(void);

int main(void){
	// int gdb = 1;
	// while (gdb) {
	//
	// }
        char line[LINE_MAX];
	char *token;

	print_prompt();
	while (fgets(line, LINE_MAX, stdin)) {
		if(*line == '\n'){
			print_prompt();
			continue;
		}
		parse_object *catch;
	
		catch = parse_token(line);

		// Builtin commands ------------------------------------------------------>
		if(strcmp(catch->command, "cd") == 0){
			char *target = catch->args[1] ? catch->args[1] : getenv("HOME");
			chdir(target);
		}
		else if(strcmp(catch->command, "hist")	== 0){
			print_hist();
		}		
		else if(strcmp(catch->command, "exit") == 0){
			exit(EXIT_SUCCESS);
		}
		// Builtin commands <------------------------------------------------------

		int j = find_cmd(catch);

		pid_t pid;

		if(j){
			pid = fork();
			if(pid == 0){
				execvp(catch->cmd_path, catch->args);
			}
			else{
				int status;
				waitpid(pid, &status, 0);
			}
			//del_po(catch);
			push_cmd(catch);
		}
		//fgets(line, LINE_MAX, stdin);
		print_prompt();
	}
}

parse_object *parse_token(char *line){
	char *cursor;
	int index = 0;
	int count = 0;

	
	parse_object *output = malloc(sizeof(parse_object));

	cursor = line;
	while (*line != '\0') {
		while(!isspace(*cursor++) && *cursor != '\0')
			index++;
	
		if(index == 0){
			line = cursor; // advance line to next space that cursor was advanced to
			continue;
		}
		if(count == 0){
			output->command = malloc(index + 1);
			output->args[count] = malloc(index + 1);
			memcpy(output->command, line, index);
			output->command[index] = '\0'; 
			memcpy(output->args[count], line, index);
			output->args[count][index] = '\0';
			count++;
		}
		else {
			output->args[count] = malloc(index + 1);
			memcpy(output->args[count], line, index);
			output->args[count][index] = '\0';
			count++;
		}
		line = cursor; // The cursor was sitting on a space? No it wasn't it fell on a space and incremented regardless
		index = 0;
	}

	output->num_args = count;
	output->args[count] = NULL;
	return output;
}

int find_cmd(parse_object *po){
	char *path_org;
	char *path;
	char *tokn;
	int found = 0;

	path_org = getenv("PATH");
	path = strdup(path_org);
	tokn = strtok(path, ":");

	char fpath[LINE_MAX];
	while (tokn != NULL) {
		strcpy(fpath, tokn);
		strcat(fpath, "/");
		strcat(fpath, po->command);
		if(access(fpath, F_OK) == 0){
			po->cmd_path = malloc(strlen(fpath) + 1);
			strcpy(po->cmd_path, fpath);
			found = 1;
			break;
		}
		tokn = strtok(NULL, ":");

	}
	free(path);
	return found;
}

void del_po(parse_object *po){
	free(po->cmd_path);
	free(po->command);
	for(int i = 0; i < po->num_args; i++){
		free(po->args[i]);
	}
	free(po);
}

void print_prompt(void){
	char prmpt[LINE_MAX];
	getcwd(prmpt, sizeof(prmpt));
	
	strcat(prmpt, ": ");
	fputs(prmpt, stdout);
	fflush(stdout);
}

void push_cmd(parse_object *po){
	if(last_cmd < HIST_MAX){
		command_history[last_cmd++] = po;
	}
	else{
		del_po(command_history[0]);
		for(int i = 0; i < (HIST_MAX - 1); i++){
			command_history[i] = command_history[i + 1];
		}
		command_history[HIST_MAX - 1] = po;
	}
	// else oldest command needs to be remmoved and destroyed and 
	// new command put in its place? Or shift every one down?
}

parse_object *pop_cmd(void){
	if(last_cmd > 0)
		return command_history[last_cmd--];
	return NULL;
}

void print_hist(void){
	if(last_cmd == 0){
		printf("No commands in history buffer\n");
	}
	else{
		for(int i = 0; i < last_cmd; i++){
			printf("%d: ", last_cmd - i);
			for(int j = 0; j < command_history[i]->num_args; j++){
				printf("%s ", command_history[i]->args[j]);
			}
			printf("\n");
		}
	}
}
