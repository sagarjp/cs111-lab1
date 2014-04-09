// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>

int lineNumber = 1;

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
enum token_type
{
  WORD,
  SEMI_COLON,
  SEQUENCE,
  NEWLINE,
  OPEN_ANGLE,
  CLOSE_ANGLE,
  AND,
  OR,
  OPEN_PARENTHESIS,
  CLOSE_PARENTHESIS,
  PIPE,
  SIMPLE,
  SUBSHELL,
  COMMENT,
  WHITESPACE,
  EMPTY,
  END_OF_FILE,
  INVALID,
};

struct command_node
{
  struct command *command;
  struct command_node *next;
  enum token_type type;
};

struct command_stream
{
  struct command_node *head;
  struct command_node *tail;
};

struct token_command
{
  command_t command;
  enum token_type type;
};

struct token
{
  char *str;
  enum token_type type;
  struct token *prev;
  struct token *next;
};

typedef struct token *token_t;

typedef struct token_command *token_command_t;

typedef enum token_type token_type_t;

enum token_type get_token_type(char *str)
{
  int i = 0;
  int len = strlen(str);
  if(len == 0)
    return EMPTY;
  while(str[i] != '\0')
  {
    if(str[i] == ' ' && len == 1)
    {
      return WHITESPACE;
    }
    else if(str[i] == '\n' && len == 1)
    {
      return NEWLINE;
    }
    else if(str[i] == EOF && len == 1)
    {
      return END_OF_FILE;
    }
    else if(isalpha(str[i]) != 0 || isdigit(str[i]) != 0 || str[i] == '!' || str[i] == '%' || str[i] == '+' 
      || str[i] == ',' || str[i] == '-' || str[i] == '.' || str[i] == '/' || str[i] == ':' || str[i] == '@'
      || str[i] == '^' || str[i] == '_')
    {
      i++;
      continue;
    }
    else if(str[i] == ';' && len == 1)
    return SEMI_COLON;
    else if(str[i] == '|')
    {
      if (len == 1)
        return PIPE;
      else if (len == 2 && str[i+1] == '|')
        return OR;
    }
    else if(str[i] == '&' && len == 2 && str[i+1] == '&')
      return AND;
    else if(str[i] == '(' && len == 1)
      return OPEN_PARENTHESIS;
      else if(str[i] == ')' && len == 1)
        return CLOSE_PARENTHESIS;
      else if(str[i] == '<' && len == 1)
        return OPEN_ANGLE;
      else if(str[i] == '>' && len == 1)
        return CLOSE_ANGLE;
      else if(str[i] == ';' && len == 1)
      return SEQUENCE;
      else 
        return INVALID;
      i++;
    }
    return WORD;
  }

  char* makeInputStream(int (*get_next_byte) (void *), void *get_next_byte_argument)
  {
    int inputStreamSize = 100;
    int byteCount = 0;
    int c;
    char *inputStream = (char*)checked_malloc(inputStreamSize*sizeof(char));

    while ((c = get_next_byte(get_next_byte_argument)) != EOF) {
      inputStream[byteCount] = c;
      byteCount++;

      if (byteCount == inputStreamSize) {
        inputStreamSize += 100;
        inputStream = checked_realloc(inputStream, inputStreamSize*sizeof(char));
      }
    }
    inputStream[byteCount] = EOF;
    byteCount++;

    return inputStream;
  }

  token_t get_next_token(char* inputStream, struct token *prev)
  {
    static int pos = -1;
    int token_size = 0;
    int max_token_size = 20;
    char *str = checked_malloc(max_token_size*sizeof(char));
    str[token_size] = '\0';
    token_t t1 = (token_t)checked_malloc(sizeof(struct token));
    token_t t2 = (token_t)checked_malloc(sizeof(struct token));
    static int parenthesisCounter = 0;

    for(;;)
    {
      pos++;
      if(inputStream[pos] == EOF)
      {
        if (prev->type != EMPTY && (prev->prev->type == AND || prev->prev->type == OR 
          || prev->prev->type == OPEN_ANGLE || prev->prev->type == CLOSE_ANGLE))
          error(1, 0, "%d: Incorrect syntax near EOF", lineNumber);
        if (parenthesisCounter != 0)
          error(1, 0, "%d: Incorrect syntax of parenthesis", lineNumber);
        goto return_token;
      }
      if(token_size == max_token_size)
      {
        max_token_size += 20;
        str = checked_realloc(str, max_token_size*sizeof(char));
      }

    //while ((inputStream[pos] == ' ' || inputStream[pos] == '\t') && prev->type == WHITESPACE)
      //pos++;

      if (inputStream[pos] == '`')
        error(1, 0, "%d: Incorrect syntax near token \'`\'", lineNumber);

      if(inputStream[pos] == ' ' || inputStream[pos] == '\n' || inputStream[pos] == '\t' || inputStream[pos] == '('
        || inputStream[pos] == ')' || inputStream[pos] == '<' || inputStream[pos] == '>' || inputStream[pos] == ';')
      {
        if (inputStream[pos] == '\n')
          lineNumber++;
        if ((inputStream[pos] == '>' || inputStream[pos] == '<') && (prev->type == EMPTY || pos == 0))
          error(1, 0, "%d: Incorrect syntax near I/O redirection", lineNumber);
        if (inputStream[pos] == '>' && inputStream[pos-1] == '>' && inputStream[pos+1] == '>')
          error(1, 0, "%d: Incorrect syntax near I/O redirection", lineNumber);
        if (inputStream[pos] == '>' && inputStream[pos+1] == EOF)
          error(1, 0, "%d: Incorrect syntax near I/O redirection", lineNumber);
        if (inputStream[pos] == ';' && (prev->type == SEMI_COLON || prev->type == EMPTY || pos == 0 || prev->type == NEWLINE))
        error(1, 0, "%d: Incorrect syntax near token \';\'", lineNumber);
        if (inputStream[pos] == ';')
        {
          if (prev->type == WHITESPACE) {
            token_t temp = prev;
            while (temp->type == WHITESPACE && temp->type != EMPTY)
            {
              if (temp->prev->type == NEWLINE)
                error(1, 0, "%d: Incorrect syntax near token \';\'", lineNumber);
              temp = temp->prev;
            }
          }
        }
        if (inputStream[pos] == '(') {
          parenthesisCounter++;
        }
        if (inputStream[pos] == ')') {
          parenthesisCounter--;
          if (parenthesisCounter < 0)
            error(1, 0, "%d: Incorrect closing parenthesis", lineNumber);
        }

        goto return_token;
      }
      if (inputStream[pos] == '&')
      {
        if (prev->type == AND || prev->type == EMPTY || pos == 0 || prev->type == NEWLINE)
          error(1, 0, "%d: Incorrect syntax near token \'&\'", lineNumber);
        if (prev->type == WHITESPACE) {
          token_t temp = prev;
          while (temp->type == WHITESPACE && temp->type != EMPTY)
          {
            if (temp->prev->type == NEWLINE)
              error(1, 0, "%d: Incorrect syntax near token \'&\'", lineNumber);
            temp = temp->prev;
          }
        }
        if (inputStream[pos+1] == '&')
        {
          pos++; t2->type = AND;
        }
        else
          t2->type = INVALID;
        goto return_token;
      }
      if (inputStream[pos] == '|')
      {
        if (prev->type == OR || prev->type == EMPTY || pos == 0 || prev->type == NEWLINE)
          error(1, 0, "%d: Incorrect syntax near token \'|\'", lineNumber);
        if (prev->type == WHITESPACE) {
          token_t temp = prev;
          while (temp->type == WHITESPACE && temp->type != EMPTY)
          {
            if (temp->prev->type == NEWLINE)
              error(1, 0, "%d: Incorrect syntax near token \'|\'", lineNumber);
            temp = temp->prev;
          }
        }
        if (inputStream[pos+1] == '|')
        {
          pos++; t2->type = OR;
        }
        goto return_token;
      }
      if (inputStream[pos] == '#')
      {
      // if (token_size != 0)
      //   fprintf(stderr, "error in comment token");

        while (inputStream[pos] != '\n')
        {
          pos++;
        }
        lineNumber++;
        goto return_token;
      }
      str[token_size] = inputStream[pos];
      token_size++;
    }

    return_token:
    t2->str = checked_malloc(2*sizeof(char));
    t2->str[0] = inputStream[pos];
    t2->str[1] = '\0';
    t2->prev = t1;

    if (t2 -> type != AND && t2->type != OR && t2->type != INVALID)
      t2->type = get_token_type(t2->str);

    if(token_size == max_token_size)
    {
      max_token_size += 1;
      str = checked_realloc(str, max_token_size*sizeof(char));
    }
    str[token_size] = '\0';
    t1->str = str;
    t1->prev = prev;
    prev->next = t1;
    t1->next = t2;
    t1->type = get_token_type(t1->str);
    if (t1 -> type == EMPTY)
    {
      token_t toDelete = t1;
      token_t toReturn = t2;
      t1->prev->next=t2;
      t2->prev = t1->prev;
      free(toDelete);
      return toReturn;
    }
    else
      return t2;
  }

  token_t remove_whitespace(token_t head)
  {
    token_t temp = head;
    token_t toGo, toDelete;
    while (temp != NULL)
    {
      if (temp->type == WHITESPACE)
      {
      if (temp->prev == NULL) // if temp is head
      {
        toDelete = temp;
        toGo = temp->next;
        temp->next->prev = NULL;
      }
      else if (temp->next == NULL) // if temp is tail
      {
        temp->prev->next = NULL;
        free(temp);
        return head;
      }
      else 
      {
        toDelete = temp;
        toGo = temp->next;
        temp->prev->next = temp->next;
        temp->next->prev = temp->prev;
      }
      free(toDelete);
      temp = toGo;
      continue;
    }
    temp = temp->next;
  }
  return head;
}

void convert_to_simple(token_t t)
{
  while(t != NULL)
  {
    if(t->type != WORD)
    {
      t = t->next;
      continue;
    }
    token_t h = t;
    int len = 0;
    int i = 0;
    while(t->type == WORD)
    {
      i++;
      len += strlen(t->str);
      t = t->next;
    }
    //printf("%s %s\n", h->str, t->prev->str);
    char *str = (char *)checked_malloc((len+1+i)*sizeof(char));
    t = h;
    len = 0;
    strcpy(&(str[len]), t->str);
    //printf("copied %s\n", t->str);
    len += strlen(t->str);
    t = t->next;
    while(t->type == WORD)
    {
      str[len] = ' ';
      len++;
      strcpy(&(str[len]), t->str);
      len += strlen(t->str);
      //printf("copied %s\n", t->str);
      t = t->next;
    }
    t = h->next;
    token_t temp;
    while(t->type == WORD)
    {
      temp = t->next;
      free(t->str);
      free(t);
      t = temp;
    }
    free(h->str);
    h->str = str;
    h->next = t;
    t->prev = h;
    //printf("%s.\n", h->str);
  }
}

void remove_newline(token_t t)
{
  bool isOperator = false;
  while(t != NULL)
  {
    if(t->type == NEWLINE)
    {
      if(t->prev == NULL)
      {
        //printf("prev null\n");
        t->next->prev = t->prev;
        token_t temp = t->next;
        free(t);
        t = temp;
        continue;
      }
      if(!isOperator && t->prev != NULL && t->next != NULL)
      {
        //printf("newline %s %s\n", t->prev->str, t->next->str);
        if(t->prev->type != NEWLINE && t->next->type != NEWLINE)
        {
          //printf("sequence %s %s %s\n", t->str, t->prev->str, t->next->str);
          t->type = SEQUENCE;
        }
        else if(t->next->type == NEWLINE)
        {
          token_t temp1 = t;
          token_t temp2 = t->next;
          t = t->next;
          while(t != NULL && t->type == NEWLINE)
          {
            temp2 = t->next;
            free(t);
            t = temp2;
          }
          temp1->next = temp2;
          if(temp2 != NULL)
            temp2->prev = temp1;
          t = temp2;
          continue;
        }
      }
      else if(isOperator && t->type == NEWLINE)
      {
        //printf("operator missing %s %s\n", t->prev->str, t->next->str);
        token_t temp1 = t->prev;
        token_t temp2 = t;
        while(t != NULL && t->type == NEWLINE)
        {
          temp2 = t->next;
          free(t);
          t = temp2;
        }
        temp1->next = temp2;
        if(temp2 != NULL)
          temp2->prev = temp1;
        t = temp2;
        continue;
      }
    }
    if(t->type == OR || t->type == SEQUENCE || t->type == AND || t->type == PIPE)
    {
      isOperator = true;
    }
    if(t->type == WORD && isOperator)
    {
      isOperator = false;
    }
    t = t->next;
  }
}

int number_of_words(char *str)
{
  int i = 0;
  int len = 1;
  while(str[i] != '\0')
  {
    if(str[i++] == ' ')
      len = len + 1;

  }
  return len;
}

token_command_t get_next_command(token_t t)
{
  if(t == NULL)
    return NULL;
  if(t->type == WORD)
  {
    const char delimiters[] = " ";
    char *cp = strdup(t->str);
    int n = number_of_words(t->str);
    //printf("%d\n", n);
    char **word = (char **)checked_malloc((n+1)*sizeof(char *));
    int i = 0;
    //printf("%s\n", cp);
    char *str = strtok(cp, delimiters);
    //printf("%s\n", str);
    while(str != NULL)
    {
      word[i] = str;
      i++;
      //printf("%d:%s\n", i, word[i-1]);
      str = strtok(NULL, delimiters);
    }
    word[i] = '\0';
    //printf("finished copying %d\n", i);
    //TODO:free str and cp
    command_t c = (command_t)checked_malloc(sizeof(struct command));
    c->type = SIMPLE_COMMAND;
    c->status = 1;
    c->input = NULL;
    c->output = NULL;
    c->u.word = word;
    token_command_t token_c = (token_command_t)checked_malloc(sizeof(struct token_command));
    token_c->command = c;
    token_c->type = SIMPLE;
    return token_c;
  }
  if(t->type == CLOSE_ANGLE || t->type == OPEN_ANGLE || t->type == OPEN_PARENTHESIS || t->type == CLOSE_PARENTHESIS)
  {
    token_command_t token_c = (token_command_t)checked_malloc(sizeof(struct token_command));
    token_c->command = (command_t)checked_malloc(sizeof(struct command));
    token_c->type = t->type;
    return token_c;
  }
  if(t->type == AND || t->type == OR || t->type == SEQUENCE || t->type == PIPE)
  {
    command_t c = (command_t)checked_malloc(sizeof(struct command));
    c->status = 1;
    c->input = NULL;
    c->output = NULL;
    token_command_t token_c = (token_command_t)checked_malloc(sizeof(struct token_command));
    token_c->command = c;
    token_c->type = t->type;
    if(t->type == AND)
    {
      c->type = AND_COMMAND;
    }
    if(t->type == OR)
    {
      c->type = OR_COMMAND;
    }
    if(t->type == SEQUENCE)
    {
      c->type = SEQUENCE_COMMAND;
    }
    if(t->type == PIPE)
    {
      c->type = PIPE_COMMAND;
    }
    return token_c;
  }
  return NULL;
}

struct command_node *pop(command_stream_t s)
{
  if(s->head == NULL)
    return NULL;
  struct command_node *n = s->head;
  s->head = s->head->next;
  return n;
}

void insert(command_stream_t s, token_command_t c)
{
  struct command_node *n = checked_malloc(sizeof(struct command_node));
  n->command = c->command;
  n->type = c->type;
  n->next = s->head;
  s->head = n;
}

struct command_node *peek(command_stream_t s)
{
  return s->head;
}

command_stream_t make_command(token_t t)
{
  token_command_t c = get_next_command(t);
  command_stream_t operators = checked_malloc(sizeof(struct command_stream));
  operators->head = NULL;
  operators->tail = NULL;
  command_stream_t commands = checked_malloc(sizeof(struct command_stream));
  commands->head = NULL;
  commands->tail = NULL;
  command_stream_t stream = checked_malloc(sizeof(struct command_stream));
  stream->head = NULL;
  stream->tail = NULL;
  while(t != NULL)
  {
    //printf("%s %d\n", t->str, t->type);
    if(t->type == WORD)
    {
      //printf("simple\n");
      insert(commands, c);
    }
    if(t->type == OPEN_PARENTHESIS)
    {
      //printf("open paranthesis\n");
      insert(operators, c);
    }
    if(t->type == CLOSE_PARENTHESIS)
    {
      //printf("close paranthesis\n");
      while(operators->head->type != OPEN_PARENTHESIS)
      {
        //printf("combine operators %d %d\n", t->type, operators->head->type);
        struct command_node *t2 = pop(commands);
        struct command_node *t1 = pop(commands);
        struct command_node *oper = pop(operators);
        oper->command->u.command[0] = t1->command;
        oper->command->u.command[1] = t2->command;
        //print_command(oper->command);
        oper->next = commands->head;
        commands->head = oper;
        if(operators->head == NULL)
          break;
      }
      pop(operators);
      command_t s = checked_malloc(sizeof(struct command_node));
      struct command_node *n = checked_malloc(sizeof(struct command_node));
      s->type = SUBSHELL_COMMAND;
      s->status = 1;
      s->input = NULL;
      s->output = NULL;
      //printf("command:\n");
      s->u.subshell_command = pop(commands)->command;
      n->type = SUBSHELL;
      n->command = s;
      n->next = commands->head;
      commands->head = n;
      //print_command(s);
      //print_command(commands->head->command);
    }
    if(t->type == PIPE || t->type == SEQUENCE || t->type == AND || t->type == OR)
    {
      //printf("operator\n");
      if(operators->head == NULL)
      {
        //printf("new operator stack %d\n", t->type);
        insert(operators, c);
      }
      else if(((int)t->type - (int)operators->head->type) > 1)
      {
        //printf("%d > %d\n", t->type, operators->head->type);
        insert(operators, c);
      }
      else
      {
        //printf("combine operators %d %d\n", t->type, operators->head->type);
        while(operators->head->type != OPEN_PARENTHESIS && ((int)t->type - (int)operators->head->type) <= 1)
        {
          //printf("combine operators %d %d\n", t->type, operators->head->type);
          struct command_node *t2 = pop(commands);
          struct command_node *t1 = pop(commands);
          struct command_node *oper = pop(operators);
          oper->command->u.command[0] = t1->command;
          oper->command->u.command[1] = t2->command;
          oper->next = commands->head;
          commands->head = oper;
          if(operators->head == NULL)
            break;
        }
        insert(operators, c);
      }
    }
    if(t->type == OPEN_ANGLE)
    {
      //printf("i operator\n");
      commands->head->command->input = t->next->str;
      t = t->next;
    }
    if(t->type == CLOSE_ANGLE)
    {
      //printf("o operator\n");
      commands->head->command->output = t->next->str;
      t = t->next;
    }
    if(t->type == NEWLINE)
    {
      if(operators->head == NULL)
      {
        //printf("no operator newline\n");
        struct command_node *n = checked_malloc(sizeof(struct command_node));
        n->command = commands->head->command;
        n->next = stream->head;
        stream->head = n;
      }
      else
      {
        //printf("operator newline\n");
        while(operators->head != NULL)
        {
          //printf("operators %d\n", operators->head->type);
          struct command_node *t2 = pop(commands);
          struct command_node *t1 = pop(commands);
          struct command_node *oper = pop(operators);
          oper->command->u.command[0] = t1->command;
          oper->command->u.command[1] = t2->command;
          oper->next = commands->head;
          commands->head = oper;
        }
        commands->head->next = stream->head;
        stream->head = commands->head;
        pop(commands);
      }
    }
    t = t->next;
    c = get_next_command(t);
  }
  if(operators->head != NULL)
  {
    while(operators->head != NULL)
    {
      //printf("combine last two operators %d\n", operators->head->type);
      if(operators->head->type == SEQUENCE && operators->head->next == NULL)
        break;
      struct command_node *t2 = pop(commands);
      struct command_node *t1 = pop(commands);
      struct command_node *oper = pop(operators);
      oper->command->u.command[0] = t1->command;
      oper->command->u.command[1] = t2->command;
      oper->next = commands->head;
      commands->head = oper;
    }
    commands->head->next = stream->head;
    stream->head = commands->head;
  }
  else if(commands->head != NULL)
  {
    commands->head->next = stream->head;
    stream->head = commands->head;
  }
  //printf("return stream\n");
  return stream;
}

command_stream_t make_command_stream(int (*get_next_byte) (void *), void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
     token_t t = checked_malloc(sizeof(struct token));
     token_t head = t;
     t->prev = NULL;
     t->str = checked_malloc(sizeof(char));
     t->str[0] = '\0';
     t->type = EMPTY;
     char* inputStream = makeInputStream(get_next_byte, get_next_byte_argument); 
     while(1) 
     { 
      token_t temp = get_next_token(inputStream, t); 
  //printf("%s %d\n", temp->prev->str, temp->prev->type); 
  //printf("%s %d\n", temp->str, temp->type); 
      if(temp->str[0] == EOF) 
      { 
        t = temp; break; 
      } 
      t = temp; 
    } 
    t->next = NULL; 
    t = remove_whitespace(head);

  // while (t != NULL)
  // {
  //   //printf("%s %d\n", t->str, t->type);
  //   //if (t -> next != NULL) {
  //     //printf("%s %d\n", t->next->str, t->next->type);
  //   //}
  //   t = t->next;
  // }
    convert_to_simple(head);
    remove_newline(head);
  // t = head;
  // while (t != NULL)
  // {
  //    printf("%s %d\n", t->str, t->type);
  //    //if (t -> next != NULL) {
  //      //printf("%s %d\n", t->next->str, t->next->type);
  //    //}
  //    t = t->next;
  // }
    //remove_newline(head);
   //  t = head;
   //  while (t != NULL)
   //  {
   //   //printf("%s %d\n", t->str, t->type);
   //   //if (t -> next != NULL) {
   //     //printf("%s %d\n", t->next->str, t->next->type);
   //   //}
   //   t = t->next;
   // }
    command_stream_t stream = make_command(head);
    command_stream_t c = checked_malloc(sizeof(struct command_stream));
    c->head = NULL;
    c->tail = NULL;
    while(stream->head != NULL)
    {
      struct command_node *n = stream->head->next;
      stream->head->next = c->head;
      c->head = stream->head;
      stream->head = n;
    }
  // while(c->head != NULL)
  // {
  //   printf("new command\n");
  //   print_command(c->head->command);
  //   c->head = c->head->next;
  // } 
  // t = head;
  // while(t != NULL)
  // {
  //   token_command_t c = get_next_command(t);
  //   //printf("%s\n", t->str);
  //   if(c != NULL)
  //   {
  //     if(c->type == SIMPLE)
  //     {
  //       int i = 0;
  //       while(c->command->u.word[i] != '\0')
  //       {
  //         //printf("%d:%s\n", i, c->command->u.word[i]);
  //         i=i+1;
  //       }
  //     }
  //     else 
  //     {
  //       //printf("%s %d\n", t->str, c->type);
  //     }
  //     //printf("\n");
  //   }
  //   t = t->next;
  // }
    return c;
  }

  command_t read_command_stream(command_stream_t t)
  {
  /* FIXME: Replace this with your implementation too.  */
  //error (1, 0, "command reading not yet implemented");
  //while(operators->head != NULL && ope)
    if(t->head == NULL)
      return NULL;
    command_t c = t->head->command;
    t->head = t->head->next;
    return c;
  }