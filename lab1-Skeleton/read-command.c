// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

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
    //! % + , - . / : @ ^ _
    else if(isalpha(str[i]) != 0 || isdigit(str[i]) != 0)
    {
      i++;
      continue;
    }
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

command_stream_t make_command_stream(int (*get_next_byte) (void *), void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  token_t t = checked_malloc(sizeof(struct token));
  t->prev = NULL;
  while(1)
  {
    token_t temp = get_next_token(get_next_byte, get_next_byte_argument, t);
    t->next = temp;
    printf("%s %d\n", temp->prev->str, temp->prev->type);
    printf("%s %d\n", temp->str, temp->type);
    if(temp->str[0] == EOF)
      break;
    t = temp;
  }
  return 0;
}

command_t read_command_stream(command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  //error (1, 0, "command reading not yet implemented");
    return 0;
}
