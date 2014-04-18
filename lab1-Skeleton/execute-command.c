// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <error.h>

#include "alloc.h"
#include <sys/types.h>
#include <error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>  
#include <sys/wait.h> 
#include <unistd.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>   
#include <fcntl.h>
#include <stdio.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
int
command_status (command_t c)
{
  return WEXITSTATUS(c->status);
}

void execute_wrapper(command_t c);

void execute_simple_command(command_t command)
{
  pid_t pid = fork();

  if(pid < 0) {
    error(1,0,"Could not create new process");
  }

  if(pid == 0) {
    if(command->input != NULL) {
      int inputdir = open(command->input,O_RDONLY,0666); //Open Input File
      if(inputdir < 0)                                            
              error(1,0,"Input file does not exist");
      dup2(inputdir,0);  //Copy File descriptor to STDIN
      close(inputdir);
    }

    if(command->output != NULL) {
      int outputdir = open(command->output,O_WRONLY | O_CREAT | O_TRUNC,0666); //Open Output File
      if(outputdir < 0)
        error(1,0,"Could not write to output file");
        dup2(outputdir,1);  //Copy File descriptor to STDOUT
        close(outputdir);
    }

    if(execvp(command->u.word[0], command->u.word) < 0) {
      error(1,0,"Command execution failed");
    }
  }
  else {
    waitpid(pid,&command->status,0);
  }
  //printf("simple %d\n", WEXITSTATUS(command->status));
  if(command->status != 0)
    command->status = 1;
  else 
    command->status = 0;
  _exit(command->status);
}        

void execute_and_command(command_t command)
{
  pid_t pid1 = fork();
  if(pid1 == 0) {
    execute_wrapper(command->u.command[0]);
  }
  else {
    waitpid(pid1, &command->u.command[0]->status, 0);
  }
  command->status = WEXITSTATUS(command->u.command[0]->status);
  if(command_status(command->u.command[0]) != 0) {
    command->status = 1;
  }
  else {
    pid_t pid2 = fork();
    if(pid2 == 0) {
      execute_wrapper(command->u.command[1]);
    }
    else {
      waitpid(pid2, &command->u.command[1]->status, 0); 
    }
    command->status = WEXITSTATUS(command->u.command[1]->status);
  }
  _exit(command->status);
}

void execute_or_command(command_t command)
{
  pid_t pid1 = fork();
  if(pid1 == 0) {
    execute_wrapper(command->u.command[0]);
  }
  else {
    waitpid(pid1, &command->u.command[0]->status, 0);
  }
  command->status = WEXITSTATUS(command->u.command[0]->status);
  //printf("or %d\n",command->status);
  if(command_status(command->u.command[0]) != 0) {
    pid_t pid2 = fork();
    if(pid2 == 0) {
      execute_wrapper(command->u.command[1]);
    }
    else {
      waitpid(pid2, &command->u.command[1]->status, 0); 
    }
    command->status = WEXITSTATUS(command->u.command[1]->status);
  }
  else {
    command->status = 0;
  }
  _exit(command->status);
}        

void execute_pipe_command(command_t command)
{

  int pipe_array[2];
  pid_t pid1 , pid2;

  if(pipe(pipe_array) == -1) error(1, 0, "pipe creation failed");

  pid1 = fork();
  if(pid1 > 0)
  {
    pid2 = fork();
    if(pid2 >0 )
    {
      close(pipe_array[0]);
      close(pipe_array[1]);

      pid_t temp = waitpid(-1,&command->status,0);
      if(temp == pid1)
      {
        waitpid(pid2,&command->status,0);
        return;
      }

      if(temp == pid2)
      {
        waitpid(pid1,&command->status,0);
        return;
      }

    }

    else if(pid2 == 0)
    {

      close(pipe_array[0]);
      dup2(pipe_array[1],1);
      execute_wrapper(command->u.command[0]);
      exit(command->u.command[0]->status);
    }

  }
  else if (pid1 == 0)
  {
    close(pipe_array[1]);
    dup2(pipe_array[0],0);
    execute_wrapper(command->u.command[1]);
    exit(command->u.command[1]->status);
  }
  else {
    error(1,0,"Fork failure");
  }
}


void execute_subshell_command(command_t command)
{
  if(command->input != NULL) {
    int fd0 = open(command->input,O_RDONLY,0666); //Open Input File
    if(fd0 < 0)                                            
      error(1,0,"Input file does not exist");
    dup2(fd0,0);  //Copy File descriptor to STDIN
    close(fd0);
  }
  
  if(command->output != NULL){
    int fd1 = open(command->output,O_WRONLY | O_CREAT | O_TRUNC,0666); //Open Output File
    
    if(fd1 < 0)
      error(1,0,"Could not write to output file");
      dup2(fd1,1);  //Copy File descriptor to STDOUT
      close(fd1);
    }
  
  execute_wrapper(command->u.subshell_command);
  command->status = command->u.subshell_command->status;
}

void execute_wrapper(command_t command);

void execute_sequence_command(command_t command)
{
  execute_wrapper(command->u.command[0]);
  execute_wrapper(command->u.command[1]);
  command->status = command->u.command[1]->status; 
}


void execute_wrapper(command_t c)
{
  switch(c->type) {                                        
    case (SIMPLE_COMMAND):
      execute_simple_command(c);
      break;
    case (AND_COMMAND):
      execute_and_command(c);
      break;
    case (OR_COMMAND):
      execute_or_command(c);
      break;
    case (SEQUENCE_COMMAND):
      execute_sequence_command(c);
      break;
    case (PIPE_COMMAND):
      execute_pipe_command(c);
      break;
    case (SUBSHELL_COMMAND):
      execute_subshell_command(c);
      break;
  }
}

void 
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  //error (1, 0, "command execution not yet implemented");
  time_travel = 0;
  if(time_travel == 0)
    time_travel = 0;
  execute_wrapper(c);
}
