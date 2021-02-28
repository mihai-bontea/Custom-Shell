/*  This file contains the bulk of the includes and defines needed for the project
 *  and the many functions required for parsing input or generating output.
 *  Compiled with: gcc -Wall -c -o UtilIO UtilIO.c
 *  The library libreadline6 needs to be installed for compilation to work(see below)
*/
#ifndef  UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <readline/readline.h>    // used for cmd history (1)
#include <readline/history.h>     // used for cmd history (2)
                                  // sudo apt-get install libreadline6 libreadline6-dev
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

#define TRUE 1
#define FALSE 0
#define PIPE_MAX 10
#define BOLD "\033[1m"
#define NORMAL "\x1B[0m"
#define GREEN "\x1b[32m"
#define BLUE "\x1B[34m"
#define YELLOW "\033[0;33m"
#define RED "\e[31m"
#define BLACK "\e[30m"
#define WHITE_BACKGROUND "\e[107m"
#define RESET "\e[0m"
#define BELL '\a'

void print_and_exit(char *message)
{
  fprintf(stderr, "%s", message);
  exit(EXIT_FAILURE);
}

/* Given the path of the directory in which to look and a command name, returns
 * the full path of the executable with the given name if it exists. Otherwise,
 * it simply sends the unmodified name back.
*/
char *get_exec_path(char *physical_location, char *command)
{
  DIR *d = opendir(physical_location);
  struct dirent *dir;

  int command_exists = FALSE;
  int size = strlen(physical_location) + strlen(command) + 2;
  char *path = malloc(size * sizeof(char));
  sprintf(path, "%s/%s", physical_location, command);

  while ((dir = readdir(d)) != NULL)
  {
    if (strcmp(dir->d_name, command) == 0)
    {
      command_exists = TRUE;
      break;
    }
  }
  closedir(d);
  if (command_exists)
    return path;
  free(path);
  return command;
}

/* Given a filename, will send the file contents to the given buffer
 * (handles memory allocation too)
*/
int read_from_file(const char *filename, char **buffer)
{
  // Open file for reading
  FILE *file = fopen(filename, "r");
  // Checking if file exists
  if (file == NULL)
  {
    fprintf(stderr, "Error opening file %s \n", filename);
    return 1;
  }

  // Determine the size of the file in bytes
  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  // Allocating memory for the file size
  *buffer = malloc(size);
  // Checking if enough memory could be allocated
  if (*buffer == NULL)
  {
    fprintf(stderr, "more: Cannot allocate %d bytes\n", size);
    fclose(file);
    return 2;
  }
  if (fread(*buffer, 1, size, file) != size)
  {
    fprintf(stderr, "more: Cannot fread %d bytes\n", size);
    fclose(file);
    return 3;
  }
  fclose(file);
  return 0;
}

/* Reads from stdin and writes the content to the given buffer.
 * (handles memory allocation too)
*/
void read_from_stdin(char **buffer)
{
  // Allocate memory for buffer
  *buffer = malloc(50000 * sizeof(char));
  if (*buffer == NULL)
  {
    fprintf(stderr, "Cannot allocate enough memory!\n");
    exit(EXIT_FAILURE);
  }

  // Read in the buffer until EOF
  int size = 0;
  int x = 0;
  do {
    x = read(STDIN_FILENO, (char *)((size_t)(*buffer) + size), 256);
    size += x;
  } while(x != 0);
}

/*  Given a char * and a char, it returns the number of times the given character
 *  appears in the string.
*/
int count_appearances(const char *str, char c)
{
  int count = 0;
  for (int index = 0; str[index]; ++index)
    if (str[index] == c)
      count++;
  return count;
}

int next_appearance(const char *str, char c)
{
  for (int index = 0; str[index]; ++index)
    if (str[index] == c)
      return index;
  return -1;
}

/* Splits the buffer into tokens. Unlike strtok, it doesn't damage the buffer and
 * includes the separator in the tokens. Handles memory allocation and sets nr_tokens
*/
void split_by_char(const char *buffer, const char separator, char ***tokens, int *nr_tokens)
{
  *nr_tokens = count_appearances(buffer, separator) + 1;
  // Allocating memory for nr_tokens tokens
  *tokens = (char **)malloc(sizeof(char *) * (*nr_tokens));

  int i = 0;
  for (int token_index = 0; token_index < (*nr_tokens); ++token_index)
  {
    int j = next_appearance(buffer + i, separator) + i;
    if (j == -1)
      j = strlen(buffer) - 1;

    int length = j - i + 1;
    // Allocate memory for token of 'length' + 1 for the NULL
    (*tokens)[token_index] = (char *)malloc(sizeof(char) * (length + 1));
    // Null-terminate the current token
    (*tokens)[token_index][length] = '\0';
    // Copy the data
    for (int index = 0; index < length; ++index)
      (*tokens)[token_index][index] = buffer[i + index];

    i = j + 1;
  }
}

int get_number(char *opt)
{
  int nr = 0;
  int negative = FALSE;
  // Determining the sign of the integer
  if ((char)(*opt) == '-')
  {
    negative = TRUE;
    opt++;
  }
  else if ((char)(*opt) == '+')
    opt++;
  // Converting from string to int
  while (*opt)
  {
    // Each character has to be between '0' and '9'
    if ((char)(*opt) < '0' || (char)(*opt) > '9')
    {
      fprintf(stderr, "Invalid character in number: %c\n", (char)(*opt));
      exit(EXIT_FAILURE);
    }

    nr += (char)(*opt) - '0';
    opt++;
    nr *= 10;
  }
  if (negative)
    nr *= -1;

  return nr / 10;
}

char getch()
{
  char buf = 0;
  struct termios old = {0};
  if (tcgetattr(0, &old) < 0)
    perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
    perror("tcsetattr ICANON");
  if (read(0, &buf, 1) < 0)
    perror ("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
    perror ("tcsetattr ~ICANON");
  return (buf);
}

/*  Builds and returns a char * containing a prompt(similar to the one of bash)
 *  using the user, home, and current working directory
*/
char *get_prompt()
{
  //LOGNAME@SESSION_MANAGER[6]:~*path after home*$
  char *prompt = malloc(256);

  // First component of prompt is the LOGNAME
  //strcpy(prompt, getenv("LOGNAME"));
  //strcat(prompt, "@");
  sprintf(prompt, "%s%s%s@",BOLD, GREEN, getenv("LOGNAME"));

  // Second component is the 6th part of the SESSION_MANAGER
  char buff[256];
  strcpy(buff, getenv("SESSION_MANAGER"));
  char *str = strtok(buff, "/");
  for (int i = 0; i < 5; ++i)
    str = strtok(NULL, "/");

  strcat(prompt, str);

  // Third component is the path after home
  size_t home_len = strlen(getenv("HOME"));
  strcat(prompt, BLUE);
  strcat(prompt, "~");
  char *dir_name;
  dir_name = getcwd((char *) NULL, 0);
  strcat(prompt, dir_name + home_len);
  free(dir_name);

  // Finally, Changing color and adding $
  strcat(prompt, NORMAL);
  strcat(prompt, "$ ");

  return prompt;
}

int are_arguments_valid(char *str)
{
  int inside_quotation = FALSE;
  for (int index = 0; index < strlen(str) - 1; ++index)
    if (str[index] == '\"')
      inside_quotation ^= 1;
    else
    {
      if (!inside_quotation && str[index] == '-' && str[index + 1] == '-')
        return FALSE;
    }
  return TRUE;
}

void print_help()
{
  printf("%s<------------------------------HELP------------------------------>%s\n", YELLOW, RESET);
  printf("%sThe commands available are:%s\n", YELLOW, RESET);
  printf("%s1) cd%s\n", GREEN, RESET);
  printf("%s2) diff -a -q%s\n", GREEN, RESET);
  printf("%s3) chmod%s\n", GREEN, RESET);
  printf("%s4) more -d -s%s\n", GREEN, RESET);
  printf("%s5) version%s\n", GREEN, RESET);
  printf("%s6) exit%s\n", RED, RESET);
  printf("%sThe shell also has support for executing external commands,%s\n", YELLOW, RESET);
  printf("%sincluding support for piped commands and redirections.%s\n", YELLOW, RESET);
  printf("%s<------------------------------HELP------------------------------>%s\n", YELLOW, RESET);
}

void print_version()
{
  printf("%s<-----------------------------VERSION----------------------------->%s\n", YELLOW, RESET);
  printf("%sAuthor: Mihai Bontea%s\n", GREEN, RESET);
  printf("%sVersion: 2.3%s\n", GREEN, RESET);
  printf("%s<-----------------------------VERSION----------------------------->%s\n", YELLOW, RESET);
}

/*  Given a char * it removes all excess whitespaces
*/
void strip_extra_whitespaces(char *str)
{
  int index, count;
  for(index = count = 0; str[index]; ++index)
    if(!isspace(str[index]) || (index > 0 && !isspace(str[index - 1])))
      str[count++] = str[index];
  str[count] = '\0';
}

/*  Given a char * it removes the whitespaces and '\n' characters
 *  from the beginning and end of the string.
 *  To be called after strip_extra_whitespaces at all times
*/
void strip_unn_whitespaces(char *str)
{
  if (isspace(str[0]))
  {
    for (int index = 0; str[index]; ++index)
      str[index] = str[index + 1];
  }

  int index = strlen(str) - 1;
  if (str[index] == '\n')
  {
    str[index] = '\0';
    index--;
  }

  if (isspace(str[index]))
    str[index] = '\0';
}

/*  Parses the given buffer,and splits it by the whitespace characters
 *  Allocates memory for command and arguments and populates them with what it finds
 *  Uses strtok therefore the buffer is destroyed after.
 *  The resulting arguments array is NULL-terminated
*/
void parse_command(char *buffer, char **command, char ***arguments)
{
  // Counting the amount of arguments
  int argc = 1;
  for (int index = 0; buffer[index]; ++index)
    if (isspace(buffer[index]))
      argc++;

  // Allocate memory to hold argc arguments
  *arguments = (char **)malloc(sizeof(char *) * (argc + 1));
  (*arguments)[argc] = NULL;
  // Splitting string by token ' '
  char *substring = strtok(buffer, " ");
  (*arguments)[0] = (char *)malloc(sizeof(char) * (strlen(substring) + 1));
  strcpy((*arguments)[0], substring);
  for (int index = 1; index < argc; ++index)
  {
    substring = strtok(NULL, " ");
    (*arguments)[index] = (char *)malloc(sizeof(char) * (strlen(substring) + 1));
    strcpy((*arguments)[index], substring);
  }
  *command = (char *)malloc(sizeof(char) * (strlen((*arguments)[0]) + 1));
  strcpy(*command, (*arguments)[0]);
}

/*  Parses the given buffer, and splits it by '|' characters
 *  Allocates memory for command_list and populates it with individual commands
 *  Modifies nr_commands to hold the number of appearances of '|' plus 1
 *  Uses strtok therefore the buffer is destroyed after.
 *  Not NULL-terminated
*/
void build_command_list(char *buffer, char ***command_list, int *nr_commands)
{
  int i = 0;
  *nr_commands = count_appearances(buffer, '|') + 1;
  *command_list = (char **)malloc(sizeof(char *) * (*nr_commands));

  char *current_cmd = strtok(buffer, "|");
  while (current_cmd != NULL)
  {
    strip_unn_whitespaces(current_cmd);
    (*command_list)[i] = (char *)malloc(sizeof(char) * (strlen(current_cmd) + 1));
    strcpy((*command_list)[i], current_cmd);
    current_cmd = strtok(NULL, "|");
    i++;
  }
}

/*  Frees the memory associated to the given char ** pointer
 *  The char ** is not NULL-terminated therefore requires the number
 *  of commands in the list to be given too.
*/
void free_command_list(char **command_list, int nr_commands)
{
  for (int index = 0; index < nr_commands; ++index)
    free(command_list[index]);
  free(command_list);
}

/*  Allocates space for 'nr_commands' pipes and initializes them.
 *  Will write to stderr if pipe() returns -1
*/
void initialize_pipes(int ***link, int nr_commands)
{
  // Allocate space for nr_commands pipes
  (*link) = (int **)malloc(nr_commands * sizeof(int *));

  // Allocate the individual pipes
  for (int index = 0; index < nr_commands; ++index)
    (*link)[index] = (int *)malloc(2 * sizeof(int));

  // Initialize the pipes
  for (int index = 0; index < nr_commands; ++index)
  {
    int res = pipe((*link)[index]);
    if (res == -1)
      fprintf(stderr, "Failure to initialize pipe at index %d\n", index);
  }
}

/*  Frees the memory associated to the pipe array
*/
void free_pipes(int **link, int nr_commands)
{
  for (int index = 0; index < nr_commands; ++index)
    free(link[index]);
  free(link);
}


#endif
