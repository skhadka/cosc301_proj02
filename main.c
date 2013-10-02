#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>

int mode = 1;
int ex = 1;

char ***command(char * buffer) {//return array of addresses, that contains array of strings
	char *sep_command = ";\n"; //commands seperated by ; and \n
	char *word, *tmp;
	int command_count = 0;		
	
	char * input = strdup(buffer);
	for (word=strtok_r(input, sep_command, &tmp); word!=NULL; word=strtok_r(NULL, sep_command, &tmp)) {
		if (word!=NULL) command_count++;
	}
	free(input);

	input = strdup(buffer);	
	char *command_spc[command_count+1]; //array of commands with spaces
	int i =0;
	for (word=strtok_r(input, sep_command, &tmp); word!=NULL; word=strtok_r(NULL, sep_command, &tmp)) {
		if (word!=NULL) {
			command_spc[i] = strdup(word); //have to do this else will copy some garbage elements
			i++;
		}
	}	
	command_spc[i] = NULL;
	free(input);
	
	char *sep_spc = " \t";
	int command_size[command_count]; //keep count of size of each command
	memset (command_size, 0, sizeof(command_size)); //initialize array with width of command 

	int actual_command_count = 0;
	for (i=0; i<command_count; i++) { 
		int count = 0;
		char *s = strdup(command_spc[i]);
		for (word=strtok_r(s, sep_spc, &tmp); word!=NULL; word=strtok_r(NULL, sep_spc, &tmp)) {
			if ((count==0) && (word[0]=='#')) break; //size of command starting with # = 0 
			if (word!=NULL) count++;	
		}
		free(s);
		command_size[i] = count;
		if(count!=0) actual_command_count ++;
	}

	if (actual_command_count==0) return NULL; 
	char *** command = (char ***) malloc (sizeof(char **)*(actual_command_count+1));
	memset (command, 0, sizeof(command)); //initialize all!!
	printf("\n\n---WORKS till here---\n\n");
	int command_index = 0;
	for (i=0; i<command_count; i++) { //make array of commands and free previous commmand with spc
		if (command_size[i]==0) {free (command_spc[i]); continue;}
		command[command_index] = (char **)malloc(sizeof(char *) * (command_size[i]+1));
		memset (command[command_index], 0, sizeof(command[command_index]));
		int j=0;
		char *s = strdup(command_spc[i]);
		word = strtok_r(s, sep_spc, &tmp);
		for (; j<command_size[i]; j++) {
			command[command_index][j] = strdup(word);
			word = strtok_r(NULL, sep_spc, &tmp);
		}
		command[command_index][j] = NULL;//set last command string as null	
		free(s);
		command_index++;
		free(command_spc[i]);
	} //all command_spc are freed!
	command[command_index] = NULL;	
	
	printf("\n----print elements in command----\n");
	
	//check if this worked!!
	for(i=0; i<actual_command_count; i++)	{
		int j=0;
		printf("\nCommand %d: ",i);
		while(command[i][j]!=NULL) {
			printf("%s ",command[i][j]);
			j++;
		}
	}
	printf("\n--WORKING SO FAR--\n");
	return command;	 
}

void set_mode(char ** command) { 
	//mode = m;
}

void set_exit(int e) {
	//ex = e;
}

void execute_sequential(char ***command) {
	int command_index = 0;
	while(command[command_index]!=NULL) { //execute commands
		printf("\n--- Executing command %d: %s ---",command_index, command[command_index][0]);
		//check for mode/exit command before this...
				
		if 

	        //execv just exits once its done! hence must call a child process
		pid_t pid = fork();
		int childrv;
		if (pid>0) waitpid(pid, &childrv, 0); //if parent wait for the child to be executed before continuing
		if (pid==0) { //if child execute command
			if(execv(command[command_index][0],command[command_index]) < 0) {
				printf("\nError! Command not found!: %s\n", command[command_index][0]);
			}
			//return; //have to return once child is done executing only happens when error --do not need to return?? but child is in a loop check! 
		}
	command_index++;
        
	}
}

void execute_parallel(char *** command) {
}
int main(int argc, char **argv) {
  
	printf("%s","prompt:> ");
	fflush(stdout);
	char buffer[1024];
	while(fgets(buffer, 1024, stdin)!=NULL) { 

		//make an array of commands	
		char ***command_array = command(buffer);
		if (command_array==NULL) continue; //no commands to be executed
		
		if (mode) execute_sequential(command_array);
		else execute_parallel(command_array);
		
	}

	return 0;
}

