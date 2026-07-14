#include <bits/posix2_lim.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct{
	char *cmd_path;
	char *command;
	char *args[20];
	int num_args;
} parse_object;

parse_object parse_token(char *);
int find_cmd(parse_object *);
void del_po(parse_object *);
void print_prompt(void);

int main(void){
        char line[LINE_MAX];
	char *token;

	print_prompt();
	while (fgets(line, LINE_MAX, stdin)) {
		if(*line == '\n'){
			print_prompt();
			continue;
		}
		parse_object catch = {0};
	
		catch = parse_token(line);
		if(strcmp(catch.command, "cd") == 0){
			char *target = catch.args[1] ? catch.args[1] : getenv("HOME");
			chdir(target);
		}
		int j = find_cmd(&catch);

		pid_t pid;

		if(j){
			pid = fork();
			if(pid == 0){
				execvp(catch.cmd_path, catch.args);
			}
			else{
				int status;
				waitpid(pid, &status, 0);
			}
			del_po(&catch);
		}
		//fgets(line, LINE_MAX, stdin);
		print_prompt();
	}
}

parse_object parse_token(char *line){
	char *cursor;
	int index = 0;
	int count = 0;

	
	parse_object output;

	cursor = line;
	while (*line != '\0') {
		while(!isspace(*cursor++) && *cursor != '\0')
			index++;
	
		if(index == 0){
			line = cursor; // advance line to next space that cursor was advanced to
			continue;
		}
		if(count == 0){
			output.command = malloc(index + 1);
			output.args[count] = malloc(index + 1);
			memcpy(output.command, line, index);
			output.command[index] = '\0'; 
			memcpy(output.args[count], line, index);
			output.args[count][index] = '\0';
			count++;
		}
		else {
			output.args[count] = malloc(index + 1);
			memcpy(output.args[count], line, index);
			output.args[count][index] = '\0';
			count++;
		}
		line = cursor; // The cursor was sitting on a space? No it wasn't it fell on a space and incremented regardless
		index = 0;
	}

	output.num_args = count;
	output.args[count] = NULL;
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
}

void print_prompt(void){
	char prmpt[LINE_MAX];
	getcwd(prmpt, sizeof(prmpt));
	
	strcat(prmpt, ": ");
	fputs(prmpt, stdout);
	fflush(stdout);
}
/* 
// Take a line break each word out and store it in an array of pointers to chars
// Except maybe the first one which will be the command name
*/
