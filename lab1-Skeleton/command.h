// UCLA CS 111 Lab 1 command interface

#include <stdbool.h>

/*
 echo abc = simple command
 simple command separated by newline or ;
 struct command *simpleCommand = (struct command *)malloc(sizeof(struct command));
 simpleCommand->type = SIMPLE_COMMAND;
 simpleCommand->status = -1;
 simpleCommand->input = NULL;
 simpleCommand->output = NULL;
 simpleCommand->u.word = (char**)malloc(2*sizeof(char*))
 simpleCommand->u.word[0] = str (char *str = "echo abc");
 simpleCommand->u.word[1] = 0;
 
 if echo < abc : str = echo, input = abc, type = SIMPLE_COMMAND
 PIPE_COMMAND: echo abc | less, output of echo becomes input of less
 
 a && b || c
 simple command a;
 simple command b;
 simple command c;
 && = AND_COMMAND
 struct command andCommand = (struct command*)malloc(sizeof(struct command));
 andCommand->u.command[0] = a;
 andCommand->u.command[1] = b;
 
 orCommand->u.command[0] = andCommand;
 orCommand->u.command[1] = c;
 
 d || e | f && g
 PIPE_COMMAND has highest precedence
 1. e | f
 2. d || (e|f)
 3. (d || (e|f)) && g
 
 f->input = e;
 
 pipeCommand->u.command[0] = e;
 pipeCommand->u.command[1] = f;
 
 orCommand->u.command[0] = d;
 orCommand->u.command[1] = pipe;
 
 andCommand->u.command[0] = orCommand;
 andCommand->u.command[1] = g
 
 
 commandStack and operatorStack
 
 going from left to right:
 1. if it is new command, push on commandStack
 2. if it is new operator:
    1. if operator stack is empty, push new operator on top
    2. else if precedence new operator > precedence top operator then push new operator on stack
    3. else 
        while top operator != OPEN_PARANTHESES && precedence new operator <= top operator
        {
            operator = pop operator stack
            command1 = pop command stack
            command2 = pop command stack
            new command = combine command1 and command2 with operator
            push new command
            top operator  = peak operator stack
            if top operator == NULL
              break
        }
        push new operator
 
 after parsing combine the last operator on operator stack with last two commands on command stack
 operator precedence: 
  semicolon, newline (lowest)
  and, or
  pipe (highest)
 
 a ; b \n c
 linked list: Seq(Seq(a,b),c)
 
 a ; b \n\n c
 linked list: Seq(a,b) -> Seq(c)
 d || e | f && g
 
 1.
 command stack = d
 
 2.
 command stack = d
 operator stack = ||
 
 3. 
 command stack = d e 
 operator stack = ||
 
 4. 
 command stack = d e
 operator stack = || |
 
 5.
 command stack = d e f
 operator stack = || |
 
 6.
 new operator &&
 
 pop |
 pop f e
 combine f and e with |
 command stack = d (e|f)
 operator stack = ||
 
 pop ||
 pop (e|f) d
 combine (e|f) and d with ||
 command stack = (d||(e|f))
 operator stack = NULL
 
 push new operator 
 command stack = (d||(e|f))
 operator stack = &&
 
 7.
 command stack = (d||(e|f)) g
 operator stack = &&
 
 after parsing, combine g and (d||(e|f)) with &&
 command stack = ((d||(e|f))&&g)
 operator stack = NULL
 
 
 a && b || c
 d || e
 
 command_stream = a&&b||c -> d||e -> NULL;
 */
struct command_node
{
  struct command *command;
  struct command_node *next;
};

typedef struct command *command_t;

struct command_stream
{
  struct command_node *head;
  struct command_node *tail;
};

typedef struct command_stream *command_stream_t;

/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the flag is set.  */
void execute_command (command_t, bool);

/* Return the exit status of a command, which must have previously
   been executed.  Wait for the command, if it is not already finished.  */
int command_status (command_t);

