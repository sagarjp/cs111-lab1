// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <ctype.h>
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
  SIMPLE,
  SUBSHELL,
  COMMENT,
  NEWLINE,
  WHITESPACE,
  EMPTY,
  END_OF_FILE,
  INVALID,
};

struct token
{
  char *str;
  enum token_type type;
  struct token *prev;
  struct token *next;
};

typedef struct token *token_t;

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

  for(;;)
  {
    pos++;
    if(inputStream[pos] == EOF)
    {
      goto return_token;
    }
    if(token_size == max_token_size)
    {
      max_token_size += 20;
      str = checked_realloc(str, max_token_size*sizeof(char));
    }

    //while ((inputStream[pos] == ' ' || inputStream[pos] == '\t') && prev->type == WHITESPACE)
      //pos++;

    if(inputStream[pos] == ' ' || inputStream[pos] == '\n' || inputStream[pos] == '\t' || inputStream[pos] == '('
      || inputStream[pos] == ')' || inputStream[pos] == '<' || inputStream[pos] == '>' || inputStream[pos] == ';')
    {
      goto return_token;
    }
    if (inputStream[pos] == '&')
    {
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
  t = head;
  while (t != NULL)
  {
    printf("%s %d\n", t->str, t->type);
    //if (t -> next != NULL) {
      //printf("%s %d\n", t->next->str, t->next->type);
    //}
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