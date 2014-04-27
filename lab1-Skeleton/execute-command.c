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

//All the dependencies are added to the node and all the dependencies of the subcommand are added to the node.
void add_dependencies(command_node_t node, command_t command)
{
  node_t temp1;
  //We get the command node and the command here.
  //First lets check if the input is not = 0, if not that means it is a command with an input and we need to take care of it.
  if(command->input != 0)
  {

    if(node->inputs == NULL)
    {
      //printf("new inputs\n");
      int size = sizeof(node_t);
      node_t head = checked_malloc(size);
      //We assign the head of our node_t struct to the input
      head->word = command->input;
      //Next is set to null
      head->next = NULL;
      node->inputs = head;
    } 
    else // This means that input is not null
    {
      //printf("exists outputs\n");
      //We need to add the word to the list. We iterate through the node struct till we reach the 
      //same input as the command input
      temp1 = node->inputs;
      while(strcmp(temp1->word, command->input) != 0) 
      { 
        if(temp1->next == NULL)
        {
          temp1->next = checked_malloc(sizeof(node_t));
          temp1->next->word = command->input; // Word getting added
          temp1->next->next = NULL;
        }
        else //continue to loop with the next word in the node
          temp1 = temp1->next;
      }
    } 
  }

  //Time for output. check if the command has an output
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
    else // This means that input is not null
    {
      //printf("exists outputs\n");
      //We need to add the word to the list. We iterate through the node struct till we reach the 
      //same input as the command input
      temp1 = node->outputs;
      while(strcmp(temp1->word, command->output) != 0) 
      { 
        if(temp1->next == NULL)
        {
          temp1->next = checked_malloc(sizeof(node_t));
          temp1->next->word = command->input; // Word getting added
          temp1->next->next = NULL;
        }
        else //continue to loop with the next word in the node
          temp1 = temp1->next;
      }
    }  
  }

  int temp = 0;
  //At this point we are done adding the input and output of the command to the structs. Let's see what kind of command it is 
  //and do things accordingly
  if(command->type == AND_COMMAND || command->type == OR_COMMAND || command->type == SEQUENCE_COMMAND || command->type == PIPE_COMMAND)
  {
    add_dependencies(node, command->u.command[0]); //Above mentioned commands could have input and out, so have to 
    //to add dependencies both for input and output. That is the left side of a pipe could itself be command.
    //So we need to take care of those dependecies as well. 
    add_dependencies(node, command->u.command[1]);
  }
  if(command->type == SUBSHELL_COMMAND)
  {
    add_dependencies(node, command->u.subshell_command);
  } 
  //If the command is a simple command then it is a different case. we do not need to call the add dependencies function again
  //Because simple command will not have a subcommand 
  if(command->type == SIMPLE_COMMAND  )
  { 
    temp = 1;
    //printf("%s\n", command->u.word[temp]);
    while(command->u.word[temp] != NULL)
    {
      if(node->inputs == NULL)
      { 
        //printf("new inputs simple\n");
        //Making the input of the node point to the head of node_t
        int size = sizeof(node_t);
        node_t head = checked_malloc(size);
        head->word = command->u.word[temp];
        head->next = NULL;
        node->inputs = head;
      }   
      else
      {
        //printf("exists inputs simple\n");
        //We need to add the word to the list. We iterate through the noed struct till we reach the 
        //same input as the command input
        temp1 = node->inputs;
        while(strcmp(temp1->word, command->u.word[temp]) != 0)
        { 
          if(temp1->next == NULL)
          {
            temp1->next = checked_malloc(sizeof(node_t));
            temp1->next->word = command->u.word[temp]; // Word getting added
            temp1->next->next = NULL;
          }
          else
            //traverse the list( node struct)
            temp1 = temp1->next;
        }
      }
        //This handles cases like echo hi > text
        //echo hello > text
        //cat text should hold text
      if(node->outputs == NULL)
      { 
          //printf("new outputs simple\n");
          //Making the input of the node point to the head of node_t
        int size = sizeof(node_t);
        node_t head = checked_malloc(size);
        head->word = command->u.word[temp];
        head->next = NULL;
        node->outputs = head;
      }   
      else
      {
          //printf("exists outputs simple\n");
          //We need to add the word to the list. We iterate through the noed struct till we reach the 
          //same input as the command output. To see if there is a dependency         
        temp1 = node->outputs;
        while(strcmp(temp1->word, command->u.word[temp]) != 0)
        { 
            //printf("%s\n", temp1->word);
          if(temp1->next == NULL)
          {
            temp1->next = checked_malloc(sizeof(node_t));
              temp1->next->word = command->u.word[temp]; // Word getting added
              temp1->next->next = NULL;
            }
            else
              //traverse the list( node struct)
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
        //increamenting dependencies and making the waiting list know
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
    //Stop condition, we have executed everything
      if(dep_head == NULL)
        break;  
      command_node_t current_node = dep_head;
    // For everyone on the list
      for(;;)
      {
        if(current_node==NULL)
          break;
      //This means there are no dependencies
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
        //We can fork now
          //printf("\n");
          print_command(current_node->c);
          int pid = fork();
          if(pid < 0)
            error(1,0,"Could not create new process");
          else if(pid == 0)
          {
            execute_command(current_node->c,0); //Get the command and do the same thing we did for part b
            exit(1); // If i do not exit here it goes into infinite loop
          }
          else if(pid > 0)
          {
            current_node->pid = pid;
          }
        }
      //Go to the next one,
        current_node = current_node->next;
      }
      
    //Waiting!!
      pid_t pid1 = waitpid(-1, &status, 0);
      

      command_node_t previous_node = NULL;
      command_node_t curr_node = dep_head;
      for(;;)
      {
        if(curr_node == NULL)
          break;
      //Done waiting
        if(curr_node->pid == pid1)
        {
          //printf("dependents\n");
          child_node_t current_dependency = curr_node->dependents;
        //Basically traverse the dependency list
          while(current_dependency != NULL)
          {
            command_node_t temp_node = current_dependency->dependent;

            temp_node->dependencies -= 1;
            //printf("%d\n", temp_node->dependencies);
            //print_command(temp_node->c);
            
            current_dependency = current_dependency->next;
          }
          //printf("dependents\n");
        //That one is done, time to move to the next one
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
  //Returning the command
    return final_command;
  }

//Because we are parallizing, it makes no sense to accept commands one by one.
//We had to take in the whole command stream and make
  command_t execute_parallel_stream (command_stream_t com)
  {
    int temp;
    command_node_t dep_head = NULL;

    command_t final_command = NULL;
    command_t command;
  //This function gets the command stream, we are reading the commands, one after the other. 
    for(;;)
    {
      command = read_command_stream(com);

    //This means we finished reading the command
      if(!command)
        break;
      
      command_node_t curr_node = NULL;

    //We got a new command, so create a new command, initialize everything to null and new_node->c to the command read
      command_node_t new_node = checked_malloc(sizeof(struct command_node_execute));
    //Initializae function will initialize everything!
      initialize(new_node, command);
    //Then we simply need to add the dependencies to our list
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

  //At this point the list has been populated and the dependencies have been added
      command_node_t current_node = dep_head;
      for(;;)
      {
      //This means there are no dependencie
        if(current_node == NULL)  
          break;
      //If there are dependencies found, we do comparisons and assign the current node accordingly
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

    //This means means we can add to the waiting list
      if(curr_node == NULL)
        dep_head = new_node;
      else
        curr_node->next = new_node;
    }
  //dep head
  // While there's someone on the waiting list
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
