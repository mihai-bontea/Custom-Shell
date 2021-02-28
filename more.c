/* Compiled as gcc -Wall -g -o more more.c
*/
#include "UtilIO.c"
#define HELP_FILE "instruct.txt"

int prompt_next_file(int display_help, char *instructions)
{
  int instruct_displayed = FALSE;
  while (TRUE)
  {
    char c = getch();
    if (c != 'q' && c != ' ')
    {
      if (!display_help)
      printf("%c", BELL);
      else
      {
        if (!instruct_displayed)
        {
          printf("\n%s%s[Press 'h' for instructions.]%s", WHITE_BACKGROUND, BLACK, RESET);
          fflush(stdout);
          instruct_displayed = TRUE;
        }
        char help_c = getch();
        if (help_c == 'h')
        {
          printf("\n%s", instructions);
          fflush(stdout);
        }
      }
      fflush(stdout);
    }
    else if (c == ' ')
    {
      printf("\n");
      return 0;
    }
    else if (c == 'q')
    {
      printf("\n");
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv, char **envp)
{
  int c;
  // Default values, can be changed by giving extra arguments
  int display_help = FALSE;
  int squeeze_blank = FALSE;
  int num_given = FALSE; int number = 0;
  char *instructions = NULL;

  // Parsing the arguments
  while((c = getopt(argc, argv, "dsn:")) != -1)
    switch(c)
    {
      case 'd':
        display_help = TRUE;
        break;

      case 's':
        squeeze_blank = TRUE;
        break;

      case 'n':
        num_given = TRUE;
        number = get_number(optarg);
        break;

      case '?':
        exit(EXIT_FAILURE);
    }

  // Load the instructions message from file
  if (display_help)
  {
    FILE *instr_file = fopen(HELP_FILE, "r");
    fseek(instr_file, 0, SEEK_END);
    int instr_size = ftell(instr_file);
    rewind(instr_file);

    instructions = malloc(instr_size);
    fread(instructions, 1, instr_size, instr_file);
  }

  char *piped_buffer = NULL;
  // Checking if file was given
  int i = optind;
  if (i >= argc)
  {
    read_from_stdin(&piped_buffer);
    if (strlen(piped_buffer) == 0)
    {
      fprintf(stderr, "more: No file given!\n");
      exit(EXIT_FAILURE);
    }
    else
    {
      printf("%s", piped_buffer);
      exit(0);
    }
  }

  // more displays filename too for multiple files
  int multiple_files = (i + 1 >= argc)? FALSE : TRUE;

  // Initialize pipe used to communicate with the child process
  int link[2];
  if (pipe(link) == -1)
  {
    fprintf(stderr, "more: error initializing pipe!\n");
    exit(EXIT_FAILURE);
  }

  int pid = fork();
  // Error checking the fork() statement
  if (pid < 0)
  {
    fprintf(stderr, "more: fork returned value = %d\n", pid);
    exit(EXIT_FAILURE);
  }

  // Child process
  if (pid == 0)
  {
    // This command will return the number of lines and columns of the current terminal
    char *args[] = {"stty", "size", NULL};
    // Redirect output to file
    dup2(link[1], STDOUT_FILENO);
    close(link[0]);
    close(link[1]);

    execvp(args[0], &args[0]);
    // Execution shouldn't reach here
    fprintf(stderr, "more: execvp issue!\n");
    exit(EXIT_FAILURE);
  }
  close(link[1]);

  // Wait for child process to finish
  wait(NULL);

  // Getting the number of lines and columns
  char buff[256];
  int nr_lines = 0, nr_columns = 0;
  read(link[0], (char *)buff, 256);
  sscanf(buff, "%d %d", &nr_lines, &nr_columns);

  int quit = FALSE;
  int file_index = i;
  while(file_index < argc || piped_buffer != NULL)
  {
    char *file_contents = NULL;
    if (piped_buffer == NULL)
    {
      FILE *file = fopen(argv[file_index], "r");
      // Checking if file exists
      if (file == NULL)
      {
        fprintf(stderr, "more: Error opening file %s \n", argv[file_index]);
        file_index++;
        continue;
      }

      // If it reached here, the file exists. If not first file, print prompt
      if (multiple_files && file_index != i)
      {
        printf("%s%s--More--(Next file: %s)%s", WHITE_BACKGROUND, BLACK, argv[file_index], RESET);
        fflush(stdout);
        quit = prompt_next_file(display_help, instructions);
      }

      if (quit)
        break;

      // Determine the size of the file in bytes
      fseek(file, 0, SEEK_END);
      int size = ftell(file);
      rewind(file);

      // Allocating memory for the file size
      file_contents = (char*)malloc(size);
      // Checking if enough memory could be allocated
      if (file_contents == NULL)
      {
        fprintf(stderr, "more: Cannot allocate %d bytes\n", size);
        exit(EXIT_FAILURE);
      }
      if (fread(file_contents, 1, size, file) != size)
      {
        fprintf(stderr, "more: Cannot fread %d bytes\n", size);
        exit(EXIT_FAILURE);
      }
      fclose(file);
    }
    else
      file_contents = piped_buffer;

    // Splitting the file by '\n' characters and counting the number of lines in the file
    int lines_in_file = 0;
    char **tokens;
    split_by_char(file_contents, '\n', &tokens, &lines_in_file);

    // If -s option is given, squeeze multiple blank lines into one
    if (squeeze_blank)
    {
      for (int x = 0; x < lines_in_file - 1; ++x)
        if (strlen(tokens[x]) == 1 && tokens[x][0] == '\n')
        {
          while (strlen(tokens[x + 1]) == 1 && tokens[x + 1][0] == '\n')
          {
            free(tokens[x + 1]);
            for (int j = x + 1; j < lines_in_file - 1; ++j)
              tokens[j] = tokens[j + 1];
            lines_in_file--;
          }
        }
    }
    // Finding the number of 'true' lines(a token from the above char** could be too big
    // to fit on a terminal line); this is used to display the percentage of how much
    // has been shown from the file so far
    int true_lines = 0;
    for (int index = 0; index < lines_in_file; ++index)
    {
      true_lines += (strlen(tokens[index]) / (nr_columns + 1));
      if (strlen(tokens[index]) % (nr_columns + 1) != 0)
        true_lines++;
    }

    int line_index = 0;
    int terminal_line_index = 0;
    int column_index = 0;

    // If -n option is given
    if (num_given)
    {
      if (number >= 0)
      {
        line_index = number;
        for (int index = 0; index < line_index; ++index)
        {
          terminal_line_index += (strlen(tokens[index]) / (nr_columns + 1));
          if (strlen(tokens[index]) % (nr_columns + 1) != 0)
            terminal_line_index++;
        }
      }
      else
        nr_lines = (number * -1);
    }

    // Put a newline to get rid of prompt
    if (multiple_files && file_index != i)
      printf("\n");
    if (multiple_files)
    {
      printf("::::::::::::::\n");
      printf("%s\n", argv[file_index]);
      printf("::::::::::::::\n");
    }
    int first_screen = TRUE;
    while (terminal_line_index < true_lines)
    {
      int count = 0;
      if (multiple_files && first_screen)
      {
        count += 3;
        first_screen = FALSE;
      }
      // Print nr_lines - 1 lines(so we leave room for the prompt)
      while (count++ < (nr_lines - 1) && terminal_line_index < true_lines)
      {
        // Real line fits into terminal line case
        if (strlen(tokens[line_index]) <= nr_columns + 1)
        {
          printf("%s", tokens[line_index]);
          line_index++;
          terminal_line_index++;
        }
        else
        {
          for (int index = column_index; index < column_index + nr_columns; ++index)
          {
            // Reached string end
            if (!tokens[line_index][index])
            {
              line_index++;
              column_index = 0;
              break;
            }
            printf("%c", tokens[line_index][index]);
            if (index + 1 == column_index + nr_columns)
            {
              column_index = index + 1;
              break;
            }
          }
          terminal_line_index++;
        }
      }

      // If everything was displayed, move on to the next file(if one exists)
      if (terminal_line_index == true_lines)
        break;

      // Print prompt
      printf("%s%s--More--(%d%%)%s", WHITE_BACKGROUND, BLACK, terminal_line_index * 100 / true_lines, RESET);
      if (display_help)
        printf("%s%s[Press space to continue, 'q' to quit.]%s", WHITE_BACKGROUND, BLACK, RESET);
      fflush(stdout);

        quit = prompt_next_file(display_help, instructions);
        if (quit)
          break;
      }
      if (piped_buffer != NULL)
      {
        free(piped_buffer);
        piped_buffer = NULL;
      }
      file_index++;
      fflush(stdout);
      // Freeing the memory
      free(file_contents);
      for (int index = 0; index < lines_in_file; ++index)
        free(tokens[index]);
      free(tokens);

      if (quit)
        break;
    }
  // Freeing the memory
  if (display_help)
    free(instructions);
  return 0;
}
