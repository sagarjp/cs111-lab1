// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
enum token_type
{
  WORD,
  SEMI_COLON,
  PIPE,
  AND,
  OR,
  OPEN_PARENTHESIS,
  CLOSE_PARENTHESIS,
  OPEN_ANGLE,
  CLOSE_ANGLE,
  SEQUENCE,
  SIMPLE,
  SUBSHELL,
  COMMENT,
  NEWLINE,
  WHITESPACE,
  EMPTY,
  END_OF_FILE,
  INVALID,
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

token_t get_next_token(int (*get_next_byte) (void *), void *get_next_byte_argument, struct token *prev)
{
  int token_size = 0;
  int max_token_size = 20;
  int c;
  char *str = checked_malloc(max_token_size*sizeof(char));
  str[token_size] = '\0';
  token_t t1 = (token_t)checked_malloc(sizeof(struct token));
  token_t t2 = (token_t)checked_malloc(sizeof(struct token));

  while(1)
  {
    c = get_next_byte(get_next_byte_argument);
    if(c == EOF)
    {
      goto return_token;
    }
    if(token_size == max_token_size)
    {
      max_token_size += 20;
      str = checked_realloc(str, max_token_size*sizeof(char));
    }
    if(c == ' ' || c == '\n')
    {
      goto return_token;
    }
    if (c == '#')
    {
      // if (token_size != 0)
      //   fprintf(stderr, "error in comment token");

      while (c != '\n')
      {
        c = get_next_byte(get_next_byte_argument);
      }
      goto return_token;
    }
    str[token_size] = c;
    token_size++;
  }

  return_token:
  t2->str = checked_malloc(2*sizeof(char));
  t2->str[0] = c;
  t2->str[1] = '\0';
  t2->prev = t1;
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
    token_c->command = NULL;
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
  while(1)
  {
    token_t temp = get_next_token(get_next_byte, get_next_byte_argument, t);
    //printf("%s %d\n", temp->prev->str, temp->prev->type);
    //printf("%s %d\n", temp->str, temp->type);
    if(temp->str[0] == EOF)
    {
      t = temp;
      break;
    }
    t = temp;
  }
  t->next = NULL;
  t = remove_whitespace(head);

  // while (t != NULL)
  // {
  //   printf("%s %d\n", t->str, t->type);
  //   //if (t -> next != NULL) {
  //     //printf("%s %d\n", t->next->str, t->next->type);
  //   //}
  //   t = t->next;
  // }
  convert_to_simple(head);
  // t = head;
  // while (t != NULL)
  // {
  //   printf("%s %d\n", t->str, t->type);
  //   //if (t -> next != NULL) {
  //     //printf("%s %d\n", t->next->str, t->next->type);
  //   //}
  //   t = t->next;
  // }
  t = head;
  while(t != NULL)
  {
    token_command_t c = get_next_command(t);
    //printf("%s\n", t->str);
    if(c != NULL)
    {
      if(c->type == SIMPLE)
      {
        int i = 0;
        while(c->command->u.word[i] != '\0')
        {
          printf("%d:%s\n", i, c->command->u.word[i]);
          i=i+1;
        }
      }
      else 
      {
        printf("%s %d\n", t->str, c->type);
      }
      printf("\n");
    }
    t = t->next;
  }
  return 0;
}

command_t read_command_stream(command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  //error (1, 0, "command reading not yet implemented");
    return 0;
}
