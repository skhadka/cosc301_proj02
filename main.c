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
int actual_mode = 1;
int ex = 1;


/* Make array of commands */
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
	memset (command_size, 0, sizeof(command_size+1)); //initialize array with width of command 

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
		if(count!=0) actual_command_count++;
	}

	if (actual_command_count==0) return NULL; 

	char *** command = (char ***) malloc (sizeof(char **)*(actual_command_count+1));
	//memset (command, 0, sizeof(command)); //initialize all!!

	int command_index = 0;
	for (i=0; i<command_count; i++) { //make array of commands and free previous commmand with spc
		if (command_size[i]==0) {
			free (command_spc[i]); 
			continue;
		}
		command[command_index] = (char **)malloc(sizeof(char *) * (command_size[i]+1));
		//memset (command[command_index], 0, sizeof(command[command_index]));
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
	} 
	free(command_spc[i]);//all command_spc are freed!
	command[command_index] = NULL;	
		
	return command;	 
}


/* Set global mode variable */
void set_mode(char ** command) {
 
	if (command[1]==NULL) { //print mode
		if (actual_mode==1) printf("\tMode is Sequential--\n");
		if (actual_mode==0) printf("\tMode is Parallel--\n");
		return;
	}
	
	if (command[2]!=NULL) { 
		printf("\tError! Incorrect parameters for mode function!\n"); 
		return; 
	}

	if ((strcasecmp(command[1],"parallel")==0) || (strcasecmp(command[1],"p")==0)) {
		mode = 0; //set global mode varible to 0: parallel
		return;
	}

	if ((strcasecmp(command[1],"sequential")==0) || (strcasecmp(command[1],"s")==0)) {
		mode = 1; //set global mode variable to 1: sequential
		return;
	}

	printf("\tError! Incorrect parameters for mode function!\n");
}


/* Set global exit variable */
void set_exit(char ** command) {
	if (command[1]!=NULL) { printf("\tError! Incorrect parameters for exit function!\n"); return;}
	ex = 0; //set global exit variable to 0 
}


/* Run commands in Sequential mode */
void execute_sequential(char ***command) {

	int command_index = 0;
	while(command[command_index]!=NULL) { //execute commands
		
		if (strcasecmp(command[command_index][0],"mode")==0) { //check mode
			printf("Executing command %d: %s\n",command_index, command[command_index][0]);
			set_mode(command[command_index]);
			command_index++; //while loop!
			continue;
		}

		if (strcasecmp(command[command_index][0],"exit")==0) { //check exit
			printf("Executing command %d: %s\n",command_index, command[command_index][0]);
			set_exit(command[command_index]);
			command_index++; //while loop!
			continue;
		}	
	
	       //execv just exits once its done! hence must call a child process
		pid_t pid = fork();
		int childrv;
		if (pid>0) waitpid(pid, &childrv, 0); //if parent wait for the child to be executed before continuing
		if (pid==0) { //make child execute command
			printf("\nExecuting command %d: %s\n",command_index, command[command_index][0]);
			if(execv(command[command_index][0],command[command_index]) < 0) {
				printf("\tError! Command not found!: %s\n", command[command_index][0]);
				exit(-1); //have to stop the child process! kill it.. 
			}
		}
	command_index++;      
	}
}


/* Run commands in Parallel mode */
void execute_parallel(char *** command) {
	printf("---IN PARALLEL---\n");
	int command_index = 0;
	int children = 0;

	//first count the number of execv commands in array as we'd need to make those many child processes
	while (command[command_index]!=NULL) {
		if ((strcasecmp(command[command_index][0],"mode")!=0) && (strcasecmp(command[command_index][0],"exit")!=0)) children++;
		command_index++;
	}

	command_index = 0;
	while(command[command_index]!=NULL) { //execute commands	
	
		if (strcasecmp(command[command_index][0],"mode")==0) { //check mode
			printf("Executing command %d: %s\n",command_index, command[command_index][0]);
			set_mode(command[command_index]);
			command_index++; //while loop!
			continue;
		}

		if (strcasecmp(command[command_index][0],"exit")==0) { //check exit
			printf("Executing command %d: %s\n",command_index, command[command_index][0]);
			set_exit(command[command_index]);
			command_index++; //while loop!
			continue;
		}	
	
	       //execv just exits once its done! hence must call a child process
		fflush(stdout);
		pid_t pid = fork();
		
		if (pid==0) { //make child execute command
			printf("\nExecuting command %d: %s\n",command_index, command[command_index][0]);
			if(execv(command[command_index][0],command[command_index]) < 0) {
				printf("\tError! Command not found!: %s\n", command[command_index][0]);
				exit(-1); //have to stop the child process! kill it.. 
			}
		}
	command_index++;      
	}
	int childrv;
	int j = 0;
	for (; j<children; j++) wait(&childrv);	
}


/* Free command array */ 
void free_array(char *** command_array) {
			int i = 0;
		while (command_array[i]!=NULL) {
			int j = 0;
			while(command_array[i][j]!=NULL) {
				free(command_array[i][j]);
				j++;
			}	
			free(command_array[i][j]);
			free(command_array[i]);
			i++;
		}
		free(command_array[i]);
}


/* --------------------------------------------MAIN------------------------------------------------------------ */
 
int main(int argc, char **argv) {
  
	printf("%s","prompt:> ");
	fflush(stdout);
	char buffer[1024];
	while(fgets(buffer, 1024, stdin)!=NULL) { 

		//make an array of commands	
		char ***command_array = command(buffer); //recall it returns malloced stuff so have to free it!
		if (command_array==NULL) {printf("\n%s","prompt:> "); continue;} //no commands to be executed
		
		if (actual_mode) execute_sequential(command_array);
		else execute_parallel(command_array);
		free_array(command_array);
		free(command_array);
		actual_mode = mode; //set actual mode to the interm one assigned by set_mode fxn
		if (!ex) break; //break if global exit set to 0
		printf("\n%s","prompt:> ");
		fflush(stdout); 	
	}
	printf("\n\n------------------------Finished Execution!------------------------\n\n");
	return 0;
}

