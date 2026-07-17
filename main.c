#include <bits/posix2_lim.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define HIST_MAX 5

enum cmd_type {BUILT_IN, SYSTEM};

typedef struct{
	char *cmd_path;
	char *command;
	char *args[20];
	char *out_redir;
	char *in_redir;
	int num_args;
	enum cmd_type c_type;
} parse_object;


parse_object *command_history[HIST_MAX];
int last_cmd = 0;

void push_cmd(parse_object *);
parse_object *pop_cmd(void);
void print_hist(void);

parse_object *parse_token(char *);
void find_cmd(parse_object *);
void del_po(parse_object *);
void handle_builtin(parse_object *);
void handle_system(parse_object *);
void copy_po(parse_object *, parse_object *);
void define_redir(parse_object *);

void print_prompt(void);


int main(void){
	// FOR DEBUG DELETE LATER
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
		find_cmd(catch);

		if(catch->c_type == SYSTEM){
			handle_system(catch);
		}
		else {
			handle_builtin(catch);
		}
		print_prompt();
	}
}

parse_object *parse_token(char *line){
	char *cursor;
	int index = 0;
	int count = 0;

	// Used calloc to set all members to 0 on creation	
	parse_object *output = calloc(1, sizeof(parse_object));

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
	// send to a function to determine redirect? 
	define_redir(output);
	return output;
}

void find_cmd(parse_object *po){
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
	if(found){
		po->c_type = SYSTEM;
	}
	else {
		po->c_type = BUILT_IN;
		po->cmd_path = strdup("");
	}
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

void handle_builtin(parse_object *po){

	if(strcmp(po->command, "cd") == 0){
		char *target = po->args[1] ? po->args[1] : getenv("HOME");
		chdir(target);
	}
	else if(strcmp(po->command, "hist") == 0){
		print_hist();
		del_po(po);
		return;
	}		
	else if(strcmp(po->command, "exit") == 0){
		exit(EXIT_SUCCESS);
	}
	else if(po->command[0] == '!'){
		int hist_id = atoi(&po->command[1]);	
		if(hist_id == 0){
			return; // failed to convert
		}
		if(hist_id <= HIST_MAX && (last_cmd - hist_id) >= 0 ){
			parse_object *hst_po = malloc(sizeof(parse_object));
			copy_po(hst_po, command_history[last_cmd - hist_id]);
			if(hst_po->c_type == BUILT_IN){
				handle_builtin(hst_po);
			}
			else if(hst_po->c_type == SYSTEM){
				handle_system(hst_po);
			}
			else{
				printf("error unknown command\n");
			}
		}
		else{
			printf("No command at %d history\n", hist_id);
		}
		return;
	}
	push_cmd(po);
}

void handle_system(parse_object *po){
	pid_t pid;

	pid = fork();
	if(pid == 0){
		int fd;
		if(po->out_redir != NULL){
			fd = open(po->out_redir, O_WRONLY | O_CREAT | O_APPEND, 0644);
		}
		dup2(fd, STDOUT_FILENO);
		close(fd);
		execvp(po->cmd_path, po->args);
		fflush(stdout);
		perror("exec failed");
	}
	else{
		int status;
		waitpid(pid, &status, 0);
	}
	push_cmd(po);
}

void copy_po(parse_object *dest, parse_object *src){
	dest->cmd_path = strdup(src->cmd_path);
	dest->command = strdup(src->command);

	int i;
	for(i = 0; i < src->num_args; i++){
		dest->args[i] = strdup(src->args[i]);
	}
	dest->args[++i] = NULL;
	dest->num_args = src->num_args;

	if(src->c_type == SYSTEM){
		dest->c_type = SYSTEM;
	}
	else {
		dest->c_type = BUILT_IN;
	}
}

void define_redir(parse_object *po){
	int i, out_i, in_i;
	out_i = in_i = 0;
	for(i = 0; i < po->num_args; i++){
		if(strcmp(po->args[i], ">") == 0 && po->args[i + 1] != NULL){
			out_i = i;
			po->out_redir = strdup(po->args[i + 1]);
		}
		if(strcmp(po->args[i], "<") == 0 && po->args[i + 1] != NULL){
			in_i = i;
			po->in_redir = strdup(po->args[i + 1]);
		}
	}
	if(out_i > 0){
		if(po->args[out_i + 2] == NULL){
			free(po->args[out_i]);
			free(po->args[out_i + 1]);
			po->args[out_i] = NULL;
		}
		else {
			int offset = out_i + 2;
			free(po->args[out_i]);
			free(po->args[out_i + 1]);
			while (po->args[offset] != NULL) {
				po->args[out_i++] = strdup(po->args[offset]);
				free(po->args[offset++]);
			}
			po->args[out_i] = NULL;
			
		}
	}
	if(in_i > 0){
		if(po->args[in_i + 2] == NULL){
			free(po->args[in_i]);
			free(po->args[in_i + 1]);
			po->args[in_i] = NULL;
		}
		else {
			int offset = in_i + 2;
			free(po->args[in_i]);
			free(po->args[in_i + 1]);
			while (po->args[offset] != NULL) {
				po->args[in_i++] = strdup(po->args[offset]);
				free(po->args[offset++]);
			}
			po->args[in_i] = NULL;
			
		}
	}

	if(out_i > in_i && out_i > 0){
		po->num_args = out_i;
	}
	else if(in_i > out_i && in_i > 0){
		po->num_args = in_i;
	}
}
