/*
  Implements a minimal shell.  The shell simply finds executables by
  searching the directories in the PATH environment variable.
  Specified executable are run in a child process.
  AUTHOR: Gigi Hsueh
  Collaboration statement: I worked on part I with Evan. Part II with help from Weronika. Part III by myself.
*/

#include "bshell.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

int parsePath(char *dirs[]);
char *lookupPath(char *fname, char **dir,int num);
int parseCmd(char *cmdLine, Command *cmd);

/*
  Read PATH environment var and create an array of dirs specified by PATH.
  PRE: dirs allocated to hold MAX_ARGS pointers.
  POST: dirs contains null-terminated list of directories.
  RETURN: number of directories.
  NOTE: Caller must free dirs[0] when no longer needed.
*/
int parsePath(char *dirs[]) {
  int i, numDirs;
  char *pathEnv;
  char *thePath;
  char *nextcharptr; /* point to next char in thePath */ 
  char *token;

  for (i = 0; i < MAX_PATHS; i++) dirs[i] = NULL;
  pathEnv = (char *) getenv("PATH");
  thePath = (char *) malloc(MAX_PATH_LEN);

  if (pathEnv == NULL) return 0; /* No path var. That's ok.*/

  /* for safety copy from pathEnv into thePath */
  strcpy(thePath,pathEnv);

#ifdef DEBUG
  printf("Path: %s\n",thePath);
#endif

  /* Now parse thePath */
  nextcharptr = thePath;

  /* 
     Find all substrings delimited by DELIM.  Make a dir element
     point to each substring.
     TODO: approx a dozen lines.
  */
  i = 0;
  token = strtok(thePath, DELIM);
  while ((token != NULL && i< MAX_PATHS)){
    dirs[i] = token;
    token = strtok(NULL, DELIM);
    numDirs++;
    i++;
  }

  /* Print all dirs */
#ifdef DEBUG
  for (i = 0; i < MAX_PATHS; i++) {
    printf("dirs[%i] contains %s\n", i,dirs[i]);
  }
#endif

  free(thePath);    
  return numDirs;
}


/*
  Search directories in dir to see if fname appears there.  This
  procedure is correct!
  PRE dir is valid array of directories
  PARAMS
   fname: file name
   dir: array of directories
   num: number of directories.  Must be >= 0.
  RETURNS full path to file name if found.  Otherwise, return NULL.
  NOTE: Caller must free returned pointer.
*/

char *lookupPath(char *fname, char **dir,int num) {
  char *fullName; // resultant name
  int maxlen; // max length copied or concatenated.
  int i;

  fullName = (char *) malloc(MAX_PATH_LEN);
  /* Check whether filename is an absolute path.*/
  if (fname[0] == '/') {
    strncpy(fullName,fname,MAX_PATH_LEN-1);
    if (access(fullName, F_OK) == 0) {
      return fullName;
    }
  }

  /* Look in directories of PATH.  Use access() to find file there. */
  else {
    for (i = 0; i < num; i++) {
      // create fullName
      maxlen = MAX_PATH_LEN - 1;
      strncpy(fullName,dir[i],maxlen);
      maxlen -= strlen(dir[i]);
      strncat(fullName,"/",maxlen);
      maxlen -= 1;
      strncat(fullName,fname,maxlen);
      // OK, file found; return its full name.
      if (access(fullName, F_OK) == 0) {
	return fullName;
      }
    }
  }
  fprintf(stderr,"%s: command not found\n",fname);
  free(fullName);
  return NULL;
}

/*
  Parse command line and fill the cmd structure.
  PRE 
   cmdLine contains valid string to parse.
   cmd points to valid struct.
  PST 
   cmd filled, null terminated.
  RETURNS arg count
  Note: caller must free cmd->argv[0..argc]
*/
int parseCmd(char *cmdLine, Command *cmd) {
  int argc = 0; // arg count
  char* token;
  int i = 0;

  token = strtok(cmdLine, SEP);
  while (token != NULL && argc < MAX_ARGS){    
    cmd->argv[argc] = strdup(token);
    printf("%s\n", cmd->argv[argc]);
    token = strtok (NULL, SEP);
    argc++;
  }
  
  //printf("you're further inside parseCmd\n");
  cmd->argv[argc] = NULL;  
  cmd->argc = argc;

#ifdef DEBUG
  printf("CMDS (%d): ", cmd->argc);
  for (i = 0; i < argc; i++)
    printf("CMDS: %s",cmd->argv[i]);
  printf("\n");
#endif
  
  return argc;
}

int main(int argc, char *argv[]) {

  char *dirs[MAX_PATHS]; // list of dirs in environment
  int i, numPaths, tf = 1, amp = 0, ssize;
  char cmdline[LINE_LEN];
  char cwd[1024];
  int *jobIDs[1024];
  char *jobNames[1024];

  numPaths = parsePath(dirs);
  // TODO
  Command *cmd = malloc(MAX_ARGS);

  //signal ctrl-c
  signal(SIGINT, SIG_IGN);

  while (tf){
    //get current directory
    getcwd(cwd, sizeof(cwd));

    //asking for a command
    printf("%s%s", cwd, PROMPT);
    fgets(cmdline, LINE_LEN, stdin);
    if (cmdline != NULL && strcmp(cmdline,"\n")!=0){ //checks if there is a command

      argc = parseCmd(cmdline, cmd); 
    
      //look up path
      char *fullPath = malloc(MAX_PATHS *1000);
      fullPath = lookupPath(cmd->argv[0], dirs, numPaths);
      printf("fullPath: %s\n", fullPath);
    
      //ampersand, this checks if the last character is an ampersand. If it is, we chop it off and decrement argc
      if (strcmp(cmd->argv[argc-1], "&") == 0){
        //printf("ampersand exist");
        amp = 1;
        cmd->argv[argc-1] = NULL;
        cmd->argc--;
      }

      //where the hard works are! builtIn command and regular commands
      if (strcmp(cmd->argv[0], "exit")== 0){
        tf = 0; //if exit command is given, tf is False, which is what keeps us in the while loop
      }else if (strcmp(cmd->argv[0], "cd") == 0){
        if (cmd->argv[1] != NULL){
          //printf("cd"); 
          chdir(cmd->argv[1]); 
        }else{
          chdir("/home"); //if no argument given after cd, cd to home
        }
      }else if (strcmp(cmd->argv[0], "jobs") ==0){
        for (i=0;i<10; i++){
          if (jobNames[i] != NULL){ //print out jobs if they exist
            printf("%i\t%s\n",jobIDs[i], jobNames[i]);
          }
        }
      }else if (strcmp(cmd->argv[0], "kill") == 0){ //to use it, say kill, and then it will ask for pid after
        int pid; 
        printf("please enter the pid that you want to kill\n");
        scanf("%d", &pid);
        printf("id:%d\n", pid);
        for (i=0;i<10;i++){
          if ((int) pid == (int) jobIDs[i]){
            kill(pid, 15);
            printf("we killed it!");
            jobNames[i] = NULL;
            jobIDs[i] = NULL;
            i--;
          }
        }
      }
       else{
        if (fullPath != NULL){
          int status; 
          pid_t child = fork(); //fork bomb! jk
          if (child == 0){
            printf("this is the child");
            execv(fullPath, cmd->argv);
            exit(0);
          }else if (child<0){
            printf("ERROR: fork failed\n");
            //exit(1);
          }else if (amp == 0){ //there's no ampersand at the end of the command
	    printf("amp1: %i\n", amp);
            pid_t wt = waitpid(child, &status, 0);
          }else if (amp == 1){ //ampersand exist!
            jobIDs[i] = (int*) child;
            jobNames[i] = (char*) cmd->argv[0];
            printf("jobIDs[%i] is %i\n", i, child);
            printf("jobNames[%i] is %s\n", i, cmd->argv[0]);
            i++;
          }
        }free(fullPath);
      }
    }
  }free(cmd);
}
