/*  This file contains the main logic of the shell
 *  Compiled with: gcc -Wall -g -o main main.c -lreadline
*/

#include "UtilIO.c"

/*  Changes the working directory of the shell to the one given in arguments[0]
 *  Prints the according message if more or less than one argument is given
 *  The changes of this function reflect in the prompt
*/
void cd(char **arguments, char *PWD_ENV)
{
  int index = 0;
  while (arguments[index] != NULL)
    index++;

  if (index != 2)
  {
    printf("%s%s%s\n", RED, "CD expects exactly one argument", NORMAL);
    return;
  }

  if (chdir(arguments[1]) == -1)
    printf("%s%s: no such file or directory\n%s",RED, arguments[1], NORMAL);
  else
  {
    strcpy(PWD_ENV, "PWD=");
    strcat(PWD_ENV, getcwd((char *) NULL, 0));
    putenv(PWD_ENV);
  }
}

int redir_filename(char **arguments, char **in_file, char **out_file)
{
  int append = FALSE;
  // Find the end position
  int end_pos = 1;
  while (arguments[end_pos])
    end_pos++;

  // Initialize both filenames with NULL
  *in_file = NULL;
  *out_file = NULL;

  if (end_pos == 1)
    return 0;

  int index = 1;
  while (index < end_pos)
  {
    int redir = TRUE;
    // Input redirection detected
    if (strcmp(arguments[index], "<") == 0)
    {
      if (!arguments[index + 1])
        print_and_exit("Redirection error: no filename given after < !\n");

      if (*in_file)
        print_and_exit("Redirection error: multiple input files given!\n");

      *in_file = arguments[index + 1];

    }
    else if (strcmp(arguments[index], ">") == 0)
    {
      if (!arguments[index + 1])
        print_and_exit("Redirection error: no filename given after > !\n");

      if (*out_file)
        print_and_exit("Redirection error: multiple output files given!\n");

      *out_file = arguments[index + 1];
    }
    else if (strcmp(arguments[index], ">>") == 0)
    {
      if (!arguments[index + 1])
        print_and_exit("Redirection error: no filename given after >> !\n");

      if (*out_file)
        print_and_exit("Redirection error: multiple output files given!\n");

      *out_file = arguments[index + 1];
      append = TRUE;
    }
    else
      redir = FALSE;

    if (redir)
    {
      // Remove ('<' or '>' or '>>') and filename from arguments
      free(arguments[index]);
      for (int j = index; j < end_pos - 1; ++j)
        arguments[j] = arguments[j + 2];

      end_pos -= 2;
    }
    index++;
  }
  return append;
}

int main(int argc, char **argv, char **envp)
{
  int networking_mode = FALSE;
  if (argc != 1)
    networking_mode = TRUE;

  // Saving the initial PWD: useful for getting the path of the
  // executables that I need to implement
  char physical_location[256];
  strcpy(physical_location, getenv("PWD"));

  // By default, the shell should start in home/user
  chdir(getenv("HOME"));
  // Modify the envp so the child processes will have the same working directory
  char *PWD_ENV = (char *)malloc(sizeof(char) * 256);
  strcpy(PWD_ENV, "PWD=");
  strcat(PWD_ENV, getcwd((char *) NULL, 0));
  putenv(PWD_ENV);

  while (TRUE)
  {
    char *prompt = get_prompt();
    char *user_input;
    if (!networking_mode)
    {
      user_input = readline(prompt);

      // If command is not ENTER, add it to history
      if (strlen(user_input) > 0)
        add_history(user_input);
        // Command is just ENTER, ignore
        else
        {
          free(user_input);
          free(prompt);
          continue;
        }
    }
    else
    {
      user_input = (char *)malloc(strlen(argv[1]));
      strcpy(user_input, argv[1]);
      if (strlen(user_input) == 0)
      {
        free(user_input);
        free(prompt);
        exit(0);
      }
    }

    // Getting rid of extra whitespaces and '\n' characters
    strip_extra_whitespaces(user_input);
    strip_unn_whitespaces(user_input);

    // Check whether arguments are in the wrong format
    if (!are_arguments_valid(user_input))
    {
      printf("The --option long argument format is not allowed!\n");
      free(user_input);
      free(prompt);
      continue;
    }

    // Display information
    if (strcmp(user_input, "help") == 0)
    {
      print_help();
      free(user_input);
      free(prompt);
      continue;
    }

    // Display version
    if (strcmp(user_input, "version") == 0)
    {
      print_version();
      free(user_input);
      free(prompt);
      continue;
    }

    // Exit loop if "exit" command is selected
    if (strcmp(user_input, "exit") == 0)
    {
      printf("%s%s%s\n", RED, "Exiting...", NORMAL);
      free(user_input);
      free(prompt);
      break;
    }

    // Splitting the user_input by the '|' character
    int nr_commands = 0;
    char **command_list = NULL;
    build_command_list(user_input, &command_list, &nr_commands);

    // Special check for cd command as it needs to be executed in the main process
    // For it to have an effect
    if (nr_commands == 1)
    {
      char temp[256];
      strcpy(temp, command_list[0]);

      char *temp_command = NULL;
      char **temp_arguments = NULL;
      parse_command(temp, &temp_command, &temp_arguments);
      // If command is cd, execute it in the main process
      if (strcmp(temp_command, "cd") == 0)
      {
        cd(temp_arguments, PWD_ENV);

        // Cleaning up memory
        free(user_input);
        free_command_list(command_list, nr_commands);
        continue;
      }
    }

    int **link = NULL;
    initialize_pipes(&link, nr_commands);

    for (int index = 0; index < nr_commands; ++index)
    {
      if (fork() == 0)
      {
        // Break command_list[index] into command and arguments
        char *command = NULL;
        char **arguments = NULL;
        parse_command(command_list[index], &command, &arguments);

        // Getting the filenames for redirection
        char *in_file = NULL;
        char *out_file = NULL;
        int append = redir_filename(arguments, &in_file, &out_file);

        // Checking if syntax is respected
        if (index != 0 && (index + 1) != nr_commands && (in_file || out_file))
          print_and_exit("Redirection error: only the first or last commands in a pipe chain can redirect to file!\n");
        if (index == 0 && nr_commands != 1 && out_file)
          print_and_exit("Redirection error: first command in the pipe chain cannot redirect output to file!\n");
        if (index != 0 && (index + 1) == nr_commands && in_file)
          print_and_exit("Redirection error: last command in the pipe chain cannot take input from file!\n");

        // If not first command, take input from a pipe
        if (index != 0)
          dup2(link[index][0], STDIN_FILENO);
        // Else, take input from file if one is given
        else
        {
          if (in_file)
          {
            int fd = open(in_file, O_RDONLY);
            if (fd < 0)
            {
              fprintf(stderr, "Redirection error: file %s does not exist!\n", in_file);
              exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
          }
        }

        // If not last command, redirect output to a pipe from which the next command will read
        if (index + 1 != nr_commands)
          dup2(link[index + 1][1], STDOUT_FILENO);
        else
        {
          if (out_file)
          {
            umask(0000);
            int fd;
            if (!append)
              fd = open(out_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            else
              fd = open(out_file, O_CREAT | O_WRONLY | O_APPEND, 0644);

            if (fd < 0)
            {
              fprintf(stderr, "Could not create file %s!\n", out_file);
              exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
          }
        }

        // Closing all the pipes of the child process
        for (int j = 0; j < nr_commands; ++j)
        {
          close(link[j][0]);
          close(link[j][1]);
        }
        // Decide which path to use for execvp depending on whether I implemented the command
        execvp(get_exec_path(physical_location, command), arguments);
        // Execution reaches here when the command does not exist
        fprintf(stderr, "%s: command not found. Type %s'help'%s for more information.\n", command, RED, RESET);
        exit(EXIT_FAILURE);
      }
    }
    // Closing all the pipes of the main process
    for (int j = 0; j < nr_commands; ++j)
    {
      close(link[j][0]);
      close(link[j][1]);
    }
    // Waiting for all child processes to finish
    while (wait(NULL) != -1);
    // Cleaning up memory
    free_pipes(link, nr_commands);
    free_command_list(command_list, nr_commands);
    free(user_input);
    free(prompt);

    if (networking_mode)
      break;
  }
  return 0;
}
