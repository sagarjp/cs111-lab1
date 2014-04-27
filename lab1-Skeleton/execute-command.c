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

typedef struct node* node_t;
typedef struct child_node* child_node_t;
typedef struct command_node_execute* command_node_t;

// A struct that holds the word and a pointer to the next word
struct node
{
  char* word;
  struct node* next;
};

//the child node which stores the word, a pointer to the enxt child and a pointer to the dependent
struct child_node
{
  struct command_node_execute* dependent;
  struct child_node* next;
  char* word;
};

// the main command node, stores the dependencies and the input outputs for a command. also has the next pointer.
// might be redundant but we did it cause it was easier
struct command_node_execute
{
  struct command* c;
  struct node* inputs;
  struct node* outputs;
  int dependencies;
  struct child_node* dependents;
  int pid;
  struct command_node_execute* next;
  char *word;
  struct command_node_execute** before;
  int length;
};

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

void add_dependencies(command_node_t node, command_t command)
{
  node_t temp1;
  if(command->input != 0)
  {

    if(node->inputs == NULL)
    {
      //printf("new inputs\n");
      int size = sizeof(node_t);
      node_t head = checked_malloc(size);
      head->word = command->input;
      head->next = NULL;
      node->inputs = head;
    } 
    else 
    {
      //printf("exists outputs\n");
      temp1 = node->inputs;
      while(strcmp(temp1->word, command->input) != 0) 
      { 
        if(temp1->next == NULL)
        {
          temp1->next = checked_malloc(sizeof(node_t));
          temp1->next->word = command->input; 
          temp1->next->next = NULL;
        }
        else 
          temp1 = temp1->next;
      }
    } 
  }

  if(command->output != 0)
  {
    if(node->outputs == NULL)
    {
      //printf("new outputs\n");
      int size = sizeof(node_t);
      node_t head = checked_malloc(size);
      head->word = command->output;
      head->next = NULL;
      node->outputs =  head;
    }
    else
    {
      //printf("exists outputs\n");
      temp1 = node->outputs;
      while(strcmp(temp1->word, command->output) != 0) 
      { 
        if(temp1->next == NULL)
        {
          temp1->next = checked_malloc(sizeof(node_t));
          temp1->next->word = command->input; 
          temp1->next->next = NULL;
        }
        else 
          temp1 = temp1->next;
      }
    }  
  }

  int temp = 0;

  if(command->type == AND_COMMAND || command->type == OR_COMMAND || command->type == SEQUENCE_COMMAND || command->type == PIPE_COMMAND)
  {
    add_dependencies(node, command->u.command[0]);  
    add_dependencies(node, command->u.command[1]);
  }
  if(command->type == SUBSHELL_COMMAND)
  {
    add_dependencies(node, command->u.subshell_command);
  } 

  if(command->type == SIMPLE_COMMAND  )
  { 
    temp = 1;
    //printf("%s\n", command->u.word[temp]);
    while(command->u.word[temp] != NULL)
    {
      if(node->inputs == NULL)
      { 
        //printf("new inputs simple\n");
        int size = sizeof(node_t);
        node_t head = checked_malloc(size);
        head->word = command->u.word[temp];
        head->next = NULL;
        node->inputs = head;
      }   
      else
      {
        //printf("exists inputs simple\n");
        temp1 = node->inputs;
        while(strcmp(temp1->word, command->u.word[temp]) != 0)
        { 
          if(temp1->next == NULL)
          {
            temp1->next = checked_malloc(sizeof(node_t));
            temp1->next->word = command->u.word[temp];
            temp1->next->next = NULL;
          }
          else
            temp1 = temp1->next;
        }
      }
      if(node->outputs == NULL)
      { 
          //printf("new outputs simple\n");
        int size = sizeof(node_t);
        node_t head = checked_malloc(size);
        head->word = command->u.word[temp];
        head->next = NULL;
        node->outputs = head;
      }   
      else
      {
          //printf("exists outputs simple\n");       
        temp1 = node->outputs;
        while(strcmp(temp1->word, command->u.word[temp]) != 0)
        { 
            //printf("%s\n", temp1->word);
          if(temp1->next == NULL)
          {
            temp1->next = checked_malloc(sizeof(node_t));
              temp1->next->word = command->u.word[temp];
              temp1->next->next = NULL;
            }
            else
              temp1 = temp1->next;
          }
        } 
        temp++;
      }
    }
  }


  int helper(command_node_t previous, node_t outputs, node_t inputs, command_node_t next_dependent)
  {
    node_t current_output = outputs;

    while(current_output != NULL)
    {
      node_t current_input = inputs;
      for(;;)
      {
        if(current_input == NULL)
          break;
        //printf("%s %s\n", current_output->word, current_input->word);
        if(strcmp(current_input->word, current_output->word) == 0)
        {
          next_dependent->dependencies += 1;

          child_node_t dependecylist = previous->dependents;
          child_node_t curr_node = dependecylist;
          while(dependecylist != NULL)
          {
            curr_node = dependecylist;
            dependecylist = dependecylist->next;
          }
          child_node_t new_node = checked_malloc(sizeof(struct child_node));
          new_node->dependent = next_dependent;
          new_node->next = NULL;
          if(curr_node == NULL)
            previous->dependents = new_node;
          else
            curr_node->next = new_node;
          //print_command(next_dependent->c);
          //printf("dependent found %d\n", next_dependent->dependencies);
          return -1;
        }
        current_input = current_input->next;
      }
      current_output = current_output->next;
    }
    return 1;
  }

  void initialize(command_node_t new_node,command_t command)
  {
    new_node->c = command;
    new_node->inputs = NULL;
    new_node->outputs = NULL;
    new_node->dependencies = 0;
    new_node->dependents = NULL;
    new_node->pid = -1;
    new_node->word = NULL;
    new_node->before = NULL;
    new_node->length = 0;
  }

  command_t
  forkingandwaiting(command_node_t dep_head, command_t final_command)
  {
    int temp1 = 0;
    int status;
    for(;;)
    {
      if(dep_head == NULL)
        break;  
      command_node_t current_node = dep_head;
      for(;;)
      {
        if(current_node==NULL)
          break;
        if(current_node->dependencies == 0 && current_node->pid < 1)
        {
          temp1 = 0;
          while(temp1 != current_node->length) 
          {
            if(current_node->before[temp1]->pid < 1)
              continue;
            waitpid(current_node->before[temp1]->pid, &status, 0);
            temp1++;
          }
          //printf("\n");
          print_command(current_node->c);
          int pid = fork();
          if(pid < 0)
            error(1,0,"Could not create new process");
          else if(pid == 0)
          {
            execute_command(current_node->c,0); 
            exit(1); 
          }
          else if(pid > 0)
          {
            current_node->pid = pid;
          }
        }
        current_node = current_node->next;
      }
      
      pid_t pid1 = waitpid(-1, &status, 0);
      

      command_node_t previous_node = NULL;
      command_node_t curr_node = dep_head;
      for(;;)
      {
        if(curr_node == NULL)
          break;
        if(curr_node->pid == pid1)
        {
          //printf("dependents\n");
          child_node_t current_dependency = curr_node->dependents;
          while(current_dependency != NULL)
          {
            command_node_t temp_node = current_dependency->dependent;

            temp_node->dependencies -= 1;
            //printf("%d\n", temp_node->dependencies);
            //print_command(temp_node->c);
            
            current_dependency = current_dependency->next;
          }
          //printf("dependents\n");
          if(previous_node == NULL)
            dep_head = curr_node->next;
          else
            previous_node->next = curr_node->next;
          break;
        }
        previous_node = curr_node;
        curr_node = curr_node->next;
      }
    }
    return final_command;
  }

  command_t execute_parallel_stream (command_stream_t com)
  {
    int temp;
    command_node_t dep_head = NULL;

    command_t final_command = NULL;
    command_t command;
    for(;;)
    {
      command = read_command_stream(com);

      if(!command)
        break;
      
      command_node_t curr_node = NULL;

      command_node_t new_node = checked_malloc(sizeof(struct command_node_execute));
      initialize(new_node, command);
      add_dependencies(new_node, command);
      //print_command(new_node->c);
    // while(new_node->inputs != NULL) 
    // {
    //   printf("%s ", new_node->inputs->word);
    //   new_node->inputs = new_node->inputs->next;
    // }
    // printf("\n");  
    // while(new_node->outputs != NULL)
    // {
    //   printf("%s ", new_node->outputs->word);
    //   new_node->outputs = new_node->outputs->next;
    // }
    // printf("\n");  
      final_command = command;

      command_node_t current_node = dep_head;
      for(;;)
      {
        if(current_node == NULL)  
          break;
        //printf("outputs inputs\n");
        temp = helper(current_node, new_node->outputs, current_node->inputs, new_node);
        if(temp == 1) 
        {
          //printf("inputs outputs\n");
          temp = helper(current_node, current_node->outputs, new_node->inputs, new_node);
        }
        if(temp == 1)
        {
          //printf("outputs outputs\n");
          temp = helper(current_node, current_node->outputs, new_node->outputs, new_node);
        }
        //printf("%d\n", temp);
        if(temp == -1)
        {
          if(new_node->before == NULL)
            new_node->before = (command_node_t *)checked_malloc(10*sizeof(struct command_node_execute *));
          if(new_node->length < 10)
            new_node->before[new_node->length] = current_node;
          new_node->length++;
        }
        curr_node = current_node;
      //Traversing the list
        current_node = current_node->next;
      }

      if(curr_node == NULL)
        dep_head = new_node;
      else
        curr_node->next = new_node;
    }
  // while(dep_head != NULL)
  // {
  //   printf("%d\n", dep_head->dependencies);
  //   print_command(dep_head->c);
  //   while(dep_head->length != 0)
  //   {
  //     print_command(dep_head->before[dep_head->length-1]->c);
  //     dep_head->length--;
  //   }
  //   dep_head = dep_head->next;
  // }
    command_t retcommand = forkingandwaiting(dep_head,final_command);
    return retcommand;
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
