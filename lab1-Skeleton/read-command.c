// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
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
  WHITESPACE
};

struct token
{
  char *str;
  enum token_type type;
  char next_char;
};

typedef struct token *token_t;

token_t get_next_token(int (*get_next_byte) (void *), void *get_next_byte_argument)
{
  int token_size = 0;
  int max_token_size = 20;
  int c;
  char *str = checked_malloc(max_token_size*sizeof(char));
  str[token_size] = '\0';
  token_t t = (token_t)checked_malloc(sizeof(struct token));
  
  while(1)
  {
    c = get_next_byte(get_next_byte_argument);
    t->next_char = c;
    if(c == EOF)
    {
      goto return_token;
      return t;
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
    str[token_size] = c;
    token_size++;
  }

  return_token:
  if(token_size == max_token_size)
  {
    max_token_size += 1;
    str = checked_realloc(str, max_token_size*sizeof(char));
  }
  str[token_size] = '\0';
  t->str = str;
  return t;
}

command_stream_t make_command_stream(int (*get_next_byte) (void *), void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  token_t t;
  while(1)
  {
    t = get_next_token(get_next_byte, get_next_byte_argument);
    if(t->next_char == EOF)
      break;
    printf("%s\n", t->str);
  }
  return 0;
}

command_t read_command_stream(command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  //error (1, 0, "command reading not yet implemented");
    return 0;
}
