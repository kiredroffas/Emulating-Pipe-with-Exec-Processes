/*  Erik Safford
 *  Creating a Shell Pipeline in C
 *  Spring 2019  */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> //For pid_t (process ID)
#include <sys/wait.h>  //For wait()
#include <errno.h>
#define MAX_ARG_SIZE 10

void freeArgs(char *leftArg[MAX_ARG_SIZE],char *rightArg[MAX_ARG_SIZE],int numLeft,int numRight,int colonP) {
	//Only want to free arguments if we had a : on command line
	if(colonP != 0) {
		for(int i = 0; i < numLeft; i++) {
			free(leftArg[i]);
		}
		for(int i = 0; i < numRight; i++) {
               	        free(rightArg[i]);
        	}
	}		
}

int main(int argc,char **argv) {

	int colonP = 0; //Place where the : is on the command line, 0 = no pipe -> execute normally
	int numColon = 0; //Number of colons entered on the command line
	int numLeft = 0;  //Number of arguments to the left of the : (not including executable)
	int numRight = 0; //Number of arguments to the right side of the :
	char *leftArg[MAX_ARG_SIZE];  //Argument(s) on the left side of the pipe, for execvp()
	char *rightArg[MAX_ARG_SIZE]; //Argument(s) on the right side of the pipe, for execvp()

	//Find out what spot the colon (:) is in, (represents pipe)
	for(int i = 1; i < argc; i++) {  
		if(numColon == 0) { //If havnt found a colon yet
			if(strcmp(argv[i],":") == 0) {  
				colonP = i; //Record the place in command line of the first colon (pipe)
				numColon++;			
			}
		}
		else{  //Else we already found the colon
			if(strcmp(argv[i],":") == 0) {
				numColon++; //Keep track of total colons (only want one)
			}
		}
	}
	
	//If no argv (after executable) before the colon,  ./connect :
	//have more then one colon on command line,        ./conncet ls : sort : uniq
        //or dont have minimum command line args, i.e.     ./connect
        //print error
        if(colonP == 1 || argc < 2 || numColon > 1) {
                fprintf(stderr,"\nIncorrect command line arguments.\nExample: ./connect <arg1> : <arg2> i.e. './connect ls : sort' represents 'ls | sort'\nWhere: <arg1> and <arg2> are optional parameters that specify the programs to be run.\nIf <arg1> is specified but <arg2> is not, then <arg1> will be run as though there was not a colon.\n\n");
                exit(1);
        }

	//If a colon was entered
	if(colonP != 0) {
		//Find out how many arguments are to the left of the colon	
		for(int i = colonP; i > 1; i--) {  //Dont want to include executable
			numLeft++;
		}	
		//Find out how many arguments are to the right of the colon
		for(int i = argc; i > colonP; i--) {
			numRight++;
		}
		numRight -= 1;
	}


	if(colonP == 0) {  //If we have ./connect ls  (with no colon)
		           //           ./connect ls -a -i (can include tags)
		//Execute the single command as normal
		execvp(argv[1],&argv[1]);
	}
	else if(numLeft > 0 && numColon == 1) { //If we have ./connect ls : sort            OR  ./connect ls :     (will read 2nd arg)
		                                //           ./connect ls -a -i : sort -r       ./connect ls -ai :

		int j = 0; //j is used to keep track of index in leftArg[]/rightArg[]
		
		//If we have argument to the right side of the colon
		//(read right arguments first to make dealing with memory malloc easier)
		if(numRight > 0) {
			j = 0; //rightArg[] utilizes j to keep track of where the NULL should be set to work with execvp
		        	     //i.e. *rightArg[] = {"sort",NULL}
			//Read in the arguments to the right of the colon from argv to rightArg[]
			for(int i = colonP + 1; i < argc; i++) {
                          	rightArg[j] = malloc( sizeof(char) * sizeof(argv[i]) );
                	        strcpy(rightArg[j],argv[i]);
			
				j++;
                	}
			rightArg[j] = NULL;
		}
		//Else if no right arguments we have to act like typing ./connect ls | on command line (read in 2nd arg to pipe)
		else if(numRight == 0) {
			printf(">");
			char str[10]; //2nd arg shouldnt be more then 10 characters long
			scanf("%10[^\n]",str); //Read in specified right argument ensuring no buffer overflow
			                       //can only read one rightside argument, no tags i.e. ./connect ls :
					       //                                                   >sort
					       //Will error when executing if any spaces/tags read in argument
			//So check to make sure user didnt put nothing or any tags or spaces before we malloc anything
			if(strlen(str) == 0) {
				fprintf(stderr,"Can't have tags or spaces in the 2nd specified argument.\nIf you want to use tags use the executable command i.e. ./connect ls -ai : sort -r\n");
                                exit(1);
			}
			for(int i = 0; i < strlen(str); i++) {
				if(str[i] == ' ') {
					fprintf(stderr,"Can't have tags or spaces in the 2nd specified argument.\nIf you want to use tags use the executable command i.e. ./connect ls -ai : sort -r\n");
					exit(1);
				}
			}
			rightArg[0] = malloc( sizeof(char) * sizeof(str) );
			strcpy(rightArg[0],str);
			rightArg[1] = NULL;
			numRight++; //We now have a malloced arg in rightArg, want to keep track of
		}

		j = 0; //j is used to track index in leftArg[] that should be NULL to set up args to work with execvp
                               //i.e. *leftArg[] = {"ls",NULL}
                //Read in the arguments to the left of the colon from argv to leftArg[]
                for(int i = 1; i < colonP; i++) {
                        leftArg[i-1] = malloc( sizeof(char) * sizeof(argv[i]) );
                        strcpy(leftArg[i-1],argv[i]);
                        j = i;
                }
                leftArg[j] = NULL;

		int fd[2];    //Pipe file descriptors, 0 = stdin, 1 = stdout
		int rdr,wtr;  //Reader and writer streams, must be intialized after pipe
		int status;   //To check status of process using wait()
                pid_t pid;    //Process ID of parent/child process

		//Setup pipe with file descriptors (fd[])
		if(pipe(fd) == -1) {
			fprintf(stderr,"Error = %d, %s\n",errno,strerror(errno));
			exit(1);
		}
		
		rdr = fd[0]; //Assign file descriptor 0 to reader (i.e. stdin)
		wtr = fd[1]; //Assign file descriptor 1 to writer (i.e. stdout)

		//Time to fork into parent and child process
		pid = fork();

		if(pid == -1) { //If we failed to fork, print error
			fprintf(stderr,"Error = %d, %s\n",errno,strerror(errno));
			exit(1);
		}
		//If fork() returns a positive pid of a child process -> parent process
		else if(pid > 0) {
			wait(&status); //Wait for writer child process to terminate
                                       //Could use wait(NULL) to achieve same effect
                        close(wtr); //close fd[1] writer
                        if(dup2(rdr,0) == -1) { //Redirect stdin to reader fd[0] (pipe[0])
                                                //Output will be via stdout
				fprintf(stderr,"Dup Error = %d, %s\n",errno,strerror(errno));
                        	exit(1);
			}
                        if(execvp(rightArg[0],rightArg) == -1) { //Execute commands to the right of the colon
				fprintf(stderr,"Execute Error = %d, %s\n",errno,strerror(errno));
                        	exit(1);
			}
                        close(rdr);  //Close reading stream
		}
		//If fork() returns a 0 -> child process
		else if(pid == 0) {
			close(rdr); //close fd[0] reader
                        if(dup2(wtr,1) == -1) { //Redirect stdout to writer fd[1] (pipe[1])
				fprintf(stderr,"Dup Error = %d, %s\n",errno,strerror(errno));
                        	exit(1);
			}
                        if(execvp(leftArg[0],leftArg) == -1) { //Execute commands to the left of the colon
				fprintf(stderr,"Execute Error = %d, %s\n",errno,strerror(errno));
                        	exit(1);
			}
                        close(wtr);  //Close writing stream
		}
	}
	//Free malloced command line arguments if any
	freeArgs(leftArg,rightArg,numLeft,numRight,colonP);
}
