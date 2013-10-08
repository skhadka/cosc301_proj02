//Shreeya Khadka
//Project 2

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
int full_path = 1; //implement adding full path to commands
struct node * background_commands = NULL; //save background commands and pids

struct pid_command { //make a seperate array of pid's and commands before execution 
	pid_t pid;
	char ** command;
};

struct node {
	pid_t pid;
	char * command;
	int running;
	struct node *next;
};


/* Print Command */

void print_command(char ** command) {
	int i = 0;
	while (command[i]!=NULL) {
		printf("%s ",command[i]);
		i++;
	}
	printf("\n");
}


/* For parallel: insert active background programs to backround_commands*/ 

void list_insert(struct pid_command pid_com) {
	struct node * n = malloc(sizeof(struct node));
	n->command = strdup((pid_com.command)[0]); //keep only main commands in heap//have to free it!
	n->pid = pid_com.pid;
	n->running = 1;
	n->next = background_commands;
	background_commands = n;
}


/* For parallel: delete background commands once executed*/ 

int list_delete(pid_t pid, struct node **head) {
 	if (head==NULL) return 0; 
	struct node *trial = *head; 
	if (trial==NULL) return 0;
	struct node *prev = NULL;
	int check = 0;
	while (trial!=NULL){
		if (pid==(trial->pid)) 
		{
			check=1; 
			break;
		}
		prev = trial;		
		trial=trial->next;
	}
	if (check==0) return 0;
	if (prev==NULL) {
	// have to remove the head
		*head = (*head)->next;
		free(trial->command);
		free(trial); //trial points to old head
		return 1;
	}
	prev->next = trial->next;
	free(trial->command);
	free(trial);
	return 1;
}


/* For parallel: print finished commands*/ 

void print_background(pid_t pid) {
	struct node * head = background_commands;
	while (head!=NULL) {
		if ((head->pid)==pid) { //found match
			printf("\n\tFinished execution> pid: %d, command: %s\n",head->pid, head->command);
			return;
		}
	head = head->next;
	}	
} 


/* Make an array of commands */

char ***command(char * buffer, char ** path, int * len) {//return array of addresses, that contains array of strings

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

	int command_index = 0;
	for (i=0; i<command_count; i++) { //make array of commands and free previous commmand with spc -- wait don't free it pass it to the background_commands for printing
		if (command_size[i]==0) {
			free (command_spc[i]); 
			continue;
		}
		command[command_index] = (char **)malloc(sizeof(char *) * (command_size[i]+1));
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
	free(command_spc[i]); //all command_spc are freed!
	command[command_index] = NULL;	
	
	if (full_path) { //have to add full path to the commands
		i = 0;
		while(command[i]!=NULL) {
			if ((strcmp(command[i][0],"mode")==0) || (strcmp(command[i][0],"exit")==0))  {
				i++; 
				continue;
			}		
			if ((!mode) && (((strcmp(command[i][0],"jobs")==0)) || (((strcmp(command[i][0],"pause")==0))) || ((strcmp(command[i][0],"resume")==0)))) { //for parallel mode checking
				i++;
				continue;
			}
			struct stat stateresult;
			int rv = stat(command[i][0], &stateresult);
			if (rv==0) {
				i++;// <----------while loop.. don't forget!!
				continue; //else add path to command[i][0]
			}
			else { //try to find match in path array
				int j = 0;	
				int lenc = (int) strlen(command[i][0]);
				while (path[j]!=NULL) {
					int lent = len[j]+lenc + 2;
					char * s = (char *) malloc(sizeof(char)*lent);
					s = memcpy (s, path[j],len[j]);
					s[len[j]] = '/';
					int k = len[j]+1;
					for (; k<lent; k++) s[k] = command[i][0][(k-len[j]-1)];	
					rv = stat(s, &stateresult);
					if (rv==0) { //found path match -> add path to the command then break i.e continue checking with other commands;
						free(command[i][0]);
						command[i][0] = strdup(s);
						free(s);
						break;
					}
					free(s);
					j++;
				}				
			}
			i++;	 
		} //finished checking all commands 
	}	
	return command;	 
}



/* Set global mode variable */

void set_mode(char ** command) {
 
	if (command[1]==NULL) { //print mode
		if (actual_mode==1) printf("\tMode is Sequential!\n");
		if (actual_mode==0) printf("\tMode is Parallel!\n");
		return;
	}
	
	if (command[2]!=NULL) { 
		printf("\n\tError! Incorrect parameters for mode function!\n"); 
		return; 
	}

	if ((strcmp(command[1],"parallel")==0) || (strcmp(command[1],"p")==0)) {
		mode = 0; //set global mode varible to 0: parallel
		return;
	}

	if ((strcmp(command[1],"sequential")==0) || (strcmp(command[1],"s")==0)) {
		mode = 1; //set global mode variable to 1: sequential
		return;
	}

	printf("\n\tError! Incorrect parameters for mode function!\n");
}


/* Set global exit variable */

void set_exit(char ** command) {
	if (command[1]!=NULL) { printf("\n\tError! Incorrect parameters for exit function!\n"); return;}
	if (background_commands!=NULL) {
		printf("\n\tError! Cannot exit when background processes are running in parallel mode! Switch to parallel mode to stop jobs and exit!\n");
		printf("\n%s","prompt:> ");
		fflush(stdin);
		return;
	}
	ex = 0; //set global exit variable to 0 
}


/* Run commands in Sequential mode */

void execute_sequential(char ***command) {

	int command_index = 0;
	while(command[command_index]!=NULL) { //execute commands
		
		printf("\nExecuting command %d: ",command_index);
		print_command(command[command_index]);

		if (strcmp(command[command_index][0],"mode")==0) { //check mode
			set_mode(command[command_index]);
			command_index++; //while loop!
			continue;
		}

		if (strcmp(command[command_index][0],"exit")==0) { //check exit
			set_exit(command[command_index]);
			command_index++; //while loop!
			continue;
		}	
	
	       //execv just exits once its done! hence must call a child process
		pid_t pid = fork();
		int childrv;
		if (pid>0) waitpid(pid, &childrv, 0); //if parent wait for the child to be executed before continuing
		if (pid==0) { //make child execute command
			if(execv(command[command_index][0],command[command_index]) < 0) {
				printf("\n\tError! Command not found!: ");
				print_command(command[command_index]);
				exit(-1); //have to stop the child process! kill it.. 
			}
		}
	printf("\n%s","prompt:> "); //for main prompt		
	fflush(stdout);
	command_index++;      
	}
}





/* Display running/paused jobs */

void getjobs() {

	printf("\nPID\tstatus\tcommand\n");
	if (background_commands==NULL) {
		//printf("BCG NULL\n");
		return; //no BCG jobs
	}
	struct node * head = background_commands;
	while (head!=NULL) {
		pid_t pid = head->pid;
		int childrv;
		int alive = waitpid(pid, &childrv, WNOHANG);
		if (alive==-1) // finished all
			break;
		if (alive==0) {	//is alive
			if (!(head->running)) printf("%d\t%s\t%s\n",pid,"paused",head->command); //paused	
			else printf("%d\t%s\t%s\n",pid,"running",head->command);	
		}
		head = head->next;
	}
	printf("\n");
}


/* Run commands in Parallel mode */

void execute_parallel(char *** command) {
	int command_index = 0;
	int children = 0;
    struct pid_command * pid_com = NULL;
	while (command[command_index]!=NULL) {
		if ((strcmp(command[command_index][0],"mode")!=0) && (strcmp(command[command_index][0],"exit")!=0) && (strcmp(command[command_index][0],"jobs")!=0) && (strcmp(command[command_index][0],"pause")!=0) && (strcmp(command[command_index][0],"resume")!=0)) {
			children++;
		}
		command_index++;
	} 

	if (children!=0) {
		pid_com = malloc(sizeof(struct pid_command)*(children+1));
	}

	command_index = 0;
	int pid_index = 0;
	int jobs = -1;

	while(command[command_index]!=NULL) { //execute commands	
	
		printf("\nExecuting command %d: ",command_index);
		print_command(command[command_index]);
		printf("\n%s","prompt:> "); //for main prompt
		fflush(stdout);

		if (strcmp(command[command_index][0],"jobs")==0) {
			if (command[command_index][1]!=NULL) printf("\n\tError! Incorrect parameters for jobs command\n");
			else jobs=command_index;
			command_index++;
			printf("\n%s","prompt:> "); //for main prompt
			fflush(stdout); 
			continue;
		}	


		if (strcmp(command[command_index][0],"pause")==0) {//handles pause command
			if (command[command_index][2]!=NULL) {
				printf("\n\tError! Incorrect parameters for pause command\n");
			}
			pid_t pid = (pid_t) atoi(command[command_index][1]);
			int status = kill(pid, SIGSTOP);
			if (status==-1) printf("\n\tPID not found!\n");
			else {
				struct node * head = background_commands;
				while (head!=NULL) {
					if((head->pid)==pid) { //if its not there then program must have been finished
						head->running = 0;
						break;
					}
				head = head->next;
				}
			}
			command_index++;
			printf("\n%s","prompt:> "); //for main prompt
			fflush(stdout); 	
			continue;
		}

		if (strcmp(command[command_index][0],"resume")==0) {//handles resume command
			if (command[command_index][2]!=NULL) {
				printf("\n\tError! Incorrect parameters for pause command\n");
			}
			pid_t pid = (pid_t) atoi(command[command_index][1]);
			int status = kill(pid, SIGCONT);
			if (status==-1) printf("\n\tPID not found!\n");
			else {
				struct node * head = background_commands;
				while (head!=NULL) {
					if((head->pid)==pid) { //if its not there then program must have been finished
						head->running = 1;
						break;
					}
				head = head->next;
				}
			}
			command_index++;
			printf("\n%s","prompt:> "); //for main prompt
			fflush(stdout); 
			continue;
		}

		if (strcmp(command[command_index][0],"mode")==0) { //check mode
			set_mode(command[command_index]);
			command_index++; //while loop!
			printf("\n%s","prompt:> "); //for main prompt
			fflush(stdout); 
			continue;
		}

		if (strcmp(command[command_index][0],"exit")==0) { //check exit
			set_exit(command[command_index]);
			command_index++; //while loop!
			printf("\n%s","prompt:> "); //for main prompt
			fflush(stdout);
			continue;
		}	
	
	     //execv just exits once its done! hence must call a child process

		(pid_com[pid_index]).command = command[command_index];
		//(pid_com[pid_index]).running = 1;
		fflush(stdout);
		(pid_com[pid_index]).pid = fork();
		if ((pid_com[pid_index]).pid==0) { //make child execute command and add pid[pid_index] to background command list
			//list_insert(pid_com[pid_index]); //will only change child's background_commands which is not accessible
			if(execv(command[command_index][0],command[command_index]) < 0) {
				printf("\n\tError! Command not found!: ");
				print_command(command[command_index]);
				printf("\n%s","prompt:> "); //for main prompt
				fflush(stdout);
				exit(-1); //have to stop the child process! kill it.. 
			}
		}
		pid_index++;
		command_index++;
	}
	if (children!=0) {
		(pid_com[children]).command= NULL;
		int i = 0;
		for (; i<children; i++) {
			list_insert(pid_com[i]); 
		}	
		free(pid_com); 
	}

	if(jobs!=-1) {
		printf("\nExecuting command %d: jobs\n", jobs); 
		getjobs(); //display the jobs only after running all other process
		printf("\n%s","prompt:> "); //for main prompt
		fflush(stdout);
	} 
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
  
	char *path[1024]; //assuming shell-config has max 1024 lines
	int len[1024]; //keep track of length of each path[i]
	FILE *datafile = NULL;
	datafile = fopen ("shell-config", "r");
	if (datafile==NULL) full_path = 0;
	else { //read contents of path file and put it in an array
		int i = 0;
		char item[126]; //pathlength max=125
		while (fgets(item, 126, datafile)!=NULL) { //get rid of spaces and such from the input file 
			const char * spc = " \n\t";
			char* tmp;
			char* word;
			for(word=strtok_r(item,spc,&tmp); word!=NULL; word=strtok_r(NULL,spc,&tmp)) {
				path[i] = strdup(word); // <--------------------------remember to free it later
				len[i] = (int) strlen(path[i]);
				i++; //avoid reading empty lines!
				break; //file should have only have 1 path per line
			}
			if (i==1023) {
				printf("Program can only parse maximum of 1023 paths in the shell-config file\n\tParsed first 1023 paths in shell-config\n");
			break;
			}  
		}
		path[i]= NULL;
	}
	fclose(datafile);


	printf("%s","prompt:> ");
	fflush(stdout);
	char buffer[1024];
	while(fgets(buffer, 1024, stdin)!=NULL) { 

		//make an array of commands	
		char ***command_array = command(buffer, path, len); //recall it returns malloced stuff so have to free it!
	
		if (command_array==NULL) {
			printf("\n%s","prompt:> "); 
			continue;
		} //no commands to be executed
		
		if (actual_mode) { /**********Sequential*************/
			execute_sequential(command_array);
			free_array(command_array);
			free(command_array);
			actual_mode = mode; //set actual mode to the interm one assigned by set_mode fxn
			if (background_commands!=NULL) { //parallel background commands in sequential mode
				struct node * head = background_commands;	
				/*while (head!=NULL) { //only checks for completion when a command is entered!! check continuously
					int childrv;
					pid_t pid = waitpid(head->pid, &childrv, WNOHANG);
					if (pid==-1) { //no more childs! empty background command list
						while(head!=NULL) {
						free(head->command);
						background_commands = head->next;
						free(head);
						head = background_commands;
						}
					} else if (pid>0) {//child is dead! remove from background
						print_background(pid);
						list_delete(pid, &background_commands);
						head = background_commands;
						printf("\n%s","prompt:> ");
						fflush(stdout);
					} else { //child is still running check for another child
						head=head->next;
						continue;
					}
				}*/
				int redo = 0;
				while (head!=NULL) { //-----------------Loop1	
				int childrv;
				pid_t pid = waitpid(-1, &childrv, WNOHANG); //wait a bit for the process?	
				if (pid==-1) { //all executions have been done! empty background_commands if any
					struct node * head = background_commands;
					while(head!=NULL) {
						free(head->command);
						background_commands = head->next;
						free(head);
						head = background_commands;
					}
					break;
				} else if (pid>0) { //child complete ->display finale statement and empty from background
					print_background(pid);
					list_delete(pid, &background_commands);
					head = background_commands;
					printf("\n%s","prompt:> ");
					fflush(stdout);
				} else {//process not complete take stdin while it runs in the background
					struct pollfd pfd;
					pfd.fd = 0; //stdin is file descriptor 0
					pfd.events = POLLIN;
					pfd.revents = 0;
					fflush(stdout); 
					int rv = poll (&pfd , 1, 1000);	
					if (rv == 0) { //timeout -> check process completetion again
						continue; //goes to Loop1
					} else if (rv > 0) { //input in stdin
						redo = 1;
						break; //read the stdin input buffer
					} else {
						printf ("\tError in reading stdin!\n");
					} 					
				}
			}
			if (redo) 
				continue; 
		}
		if (!ex) break; //break if global exit set to 0	
		printf("\n%s","prompt:> ");
		fflush(stdin);
		}
		else { /**********Parallel*************/
			execute_parallel(command_array);
			free_array(command_array);
			free(command_array);
			int redo = 0;
			struct node * head = background_commands;
			while (head!=NULL) { //-----------------Loop1	
				int childrv;
				pid_t pid = waitpid(-1, &childrv, WNOHANG); //wait a bit for the process?	
				if (pid==-1) { //all executions have been done! empty background_commands if any
					struct node * head = background_commands;
					while(head!=NULL) {
						free(head->command);
						background_commands = head->next;
						free(head);
						head = background_commands;
					}
					break;
				} else if (pid>0) { //child complete ->display finale statement and empty from background
					print_background(pid);
					list_delete(pid, &background_commands);
					head = background_commands;
					printf("\n%s","prompt:> ");
					fflush(stdout);
				} else {//process not complete take stdin while it runs in the background
					struct pollfd pfd;
					pfd.fd = 0; //stdin is file descriptor 0
					pfd.events = POLLIN;
					pfd.revents = 0;
					fflush(stdout); 
					int rv = poll (&pfd , 1, 1000);	
					if (rv == 0) { //timeout -> check process completetion again
						continue; //goes to Loop1
					} else if (rv > 0) { //input in stdin
						redo = 1;
						break; //read the stdin input buffer
					} else {
						printf ("\tError in reading stdin!\n");
					} 					
				}
				
			} //out of Loop1
			actual_mode = mode; //set the global mode to interim one
			if (redo) {
				continue; //continues onto main fgets loop
			}
		}
		if (!ex) break; //break if global exit set to 0
	} //fgets while ends here
	if (full_path) { //free path
		int i = 0;
		while (path[i]!=NULL) {
			free(path[i]);
			i++;	
		}
	}
	printf("\n\n------------------------Finished Execution!------------------------\n\n");
	return 0;
}

