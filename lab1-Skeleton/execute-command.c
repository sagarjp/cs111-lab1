// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <stdio.h>
#include <error.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/wait.h> 
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>   

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

void execute_wrapper(command_t command);

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
      int inputdir = open(command->input, O_RDONLY, 0644);
      if(inputdir < 0)                                            
        error(1, 0, "Input file does not exist");
      if(dup2(inputdir,0) < 0)
        exit(1); 
      close(inputdir);
    }

    if(command->output != NULL) {
      int outputdir = open(command->output,O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if(outputdir < 0)
        error(1,0,"Could not write to output file");
      if(dup2(outputdir,1) < 0)
        exit(1);
      close(outputdir);
    }

    if(execvp(command->u.word[0], command->u.word) < 0) {
      error(1, 0, "Command execution failed");
    }
    _exit(command->status);
  }
  else {
    waitpid(pid,&command->status,0);
  }

  command->status = WEXITSTATUS(command->status);
  //_exit(command->status);
}        

void execute_and_command(command_t command)
{
  pid_t pid1 = fork();

  if(pid1 < 0) {
    error(1,0,"Could not create new process");
  }

  if(pid1 == 0) {
    execute_wrapper(command->u.command[0]);
    _exit(command->u.command[0]->status);
  }
  else {
    waitpid(pid1, &command->u.command[0]->status, 0);
  }
  command->status = WEXITSTATUS(command->u.command[0]->status);
  if(command->u.command[0]->status != 0) {
    command->status = 1;
  }
  else {
    pid_t pid2 = fork();
    if(pid2 < 0) {
      error(1,0,"Could not create new process");
    }
    if(pid2 == 0) {
      execute_wrapper(command->u.command[1]);
      _exit(command->u.command[1]->status);
    }
    else {
      waitpid(pid2, &command->u.command[1]->status, 0); 
    }
    command->status = WEXITSTATUS(command->u.command[1]->status);
  }
  //_exit(command->status);
}

void execute_or_command(command_t command)
{
  pid_t pid1 = fork();
  if(pid1 < 0) {
    error(1,0,"Could not create new process");
  }
  if(pid1 == 0) {
    execute_wrapper(command->u.command[0]);
    _exit(command->u.command[0]->status);
  }
  else {
    waitpid(pid1, &command->u.command[0]->status, 0);
  }
  command->status = WEXITSTATUS(command->u.command[0]->status);
  //printf("or %d\n",command->status);
  if(command->u.command[0]->status != 0) {
    pid_t pid2 = fork();
    if(pid2 < 0) {
      error(1,0,"Could not create new process");
    }
    if(pid2 == 0) {
      execute_wrapper(command->u.command[1]);
      _exit(command->u.command[1]->status);
    }
    else {
      waitpid(pid2, &command->u.command[1]->status, 0); 
    }
    command->status = WEXITSTATUS(command->u.command[1]->status);
  }
  else {
    command->status = 0;
  }
  //_exit(command->status);
}        

void execute_pipe_command(command_t command)
{
  pid_t returnedPid;
  pid_t firstPid;
  pid_t secondPid;
  int buffer[2];
  int eStatus;

  if ( pipe(buffer) < 0 )
  {
    error (1, 0, "pipe was not created");
  }

  firstPid = fork();
  if (firstPid < 0)
    {
    error(1, 0, "fork was unsuccessful");
    }
  else if (firstPid == 0) //child executes command on the right of the pipe
  {
    close(buffer[1]); //close unused write end

        //redirect standard input to the read end of the pipe
        //so that input of the command (on the right of the pipe)
        //comes from the pipe
    if ( dup2(buffer[0], 0) < 0 )
    {
      error(1, 0, "error with dup2");
    }
    execute_wrapper(command->u.command[1]);
    _exit(command->u.command[1]->status);
  }
  else 
  {
    // Parent process
    secondPid = fork(); //fork another child process
                            //have that child process executes command on the left of the pipe
    if (secondPid < 0)
    {
      error(1, 0, "fork was unsuccessful");
    }
        else if (secondPid == 0)
    {
      close(buffer[0]); //close unused read end
      if(dup2(buffer[1], 1) < 0) //redirect standard output to write end of the pipe
            {
        error (1, 0, "error with dup2");
            }
      execute_wrapper(command->u.command[0]);
      _exit(command->u.command[0]->status);
    }
    else
    {
      // Finishing processes
      returnedPid = waitpid(-1, &eStatus, 0); //this is equivalent to wait(&eStatus);
                        //we now have 2 children. This waitpid will suspend the execution of
                        //the calling process until one of its children terminates
                        //(the other may not terminate yet)

      //Close pipe
      close(buffer[0]);
      close(buffer[1]);

      if (secondPid == returnedPid )
      {
          //wait for the remaining child process to terminate
        waitpid(firstPid, &eStatus, 0); 
        command->status = WEXITSTATUS(eStatus);
        return;
      }
      
      if (firstPid == returnedPid)
      {
          //wait for the remaining child process to terminate
          waitpid(secondPid, &eStatus, 0);
        command->status = WEXITSTATUS(eStatus);
        return;
      }
    }
  } 
}


void execute_subshell_command(command_t command)
{
  pid_t pid1;
  if(command->input != NULL) {
    int fd0 = open(command->input, O_RDONLY, 0644);
    if(fd0 < 0)                                            
      error(1,0,"Input file does not exist");
    if(dup2(fd0,0) < 0)
      exit(1);
    close(fd0);
  }
  
  if(command->output != NULL) {
    int fd1 = open(command->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd1 < 0)
      error(1,0,"Could not write to output file");
    if (dup2(fd1,1) < 0)
      exit(1);
    close(fd1);
  }
  pid1 = fork();
  if(pid1 < 0) {
    error(1,0,"Could not create new process");
  }
  if(pid1 == 0) {
    execute_wrapper(command->u.subshell_command);
    _exit(command->u.subshell_command->status);
  }
  else {
    waitpid(pid1, &command->u.subshell_command->status, 0);
  }
  command->status = WEXITSTATUS(command->u.subshell_command->status);
  //_exit(command->status);
}

void execute_sequence_command(command_t command)
{
  pid_t pid1, pid2;
  pid1 = fork();
  if(pid1 < 0) {
    error(1,0,"Could not create new process");
  }
  if(pid1 == 0) {
    execute_wrapper(command->u.command[0]);
    _exit(command->u.command[0]->status);
  }
  else {
    waitpid(pid1, &command->u.command[0]->status, 0);
  }
  pid2 = fork();
  if(pid2 < 0) {
    error(1,0,"Could not create new process");
  }
  if(pid2 == 0) {
    execute_wrapper(command->u.command[1]);
    _exit(command->u.command[1]->status);
  }
  else {
    waitpid(pid2, &command->u.command[1]->status, 0);
  }
  command->status = WEXITSTATUS(command->u.command[1]->status);
  //_exit(command->status); 
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
