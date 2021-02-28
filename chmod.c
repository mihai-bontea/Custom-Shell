/* Compiled as gcc -Wall -g -o chmod chmod.c
*/
#include "UtilIO.c"

int get_perm_number_numeric(const char *str)
{
  // length has to be at most 3
  if (strlen(str) > 3)
  {
    fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
    exit(EXIT_FAILURE);
  }
  // The digits need to be between 0 and 7
  for (int index = 0; str[index]; ++index)
    if (str[index] > '7')
    {
      fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
      exit(EXIT_FAILURE);
    }

  // If not specified, the default value of 0 is used
  int u_num = 0, g_num = 0, o_num = 0;

  // Other
  int index = strlen(str) - 1;
  o_num = str[index] - '0';

  // Group
  index--;
  if (index >= 0)
    g_num = str[index] - '0';

  // User
  index--;
  if (index >= 0)
    u_num = str[index] - '0';

  return 64 * u_num + 8 * g_num + o_num;
}

int get_perm_number_symbolic(const char *str)
{
  // Copy the contents of str
  char *temp = (char *)malloc(sizeof(char) * (strlen(str) + 1));
  strcpy(temp, str);

  // If not specified, the default value of 0 is used
  int nr = 0;

  char *current = strtok(temp, ",");
  while (current != NULL)
  {
    // length of a token should be at most 5 and at least 3
    if (strlen(current) > 5 || strlen(current) < 3)
    {
      fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
      free(temp);
      exit(EXIT_FAILURE);
    }

    // First character needs to be either u/g/o
    if (current[0] != 'u' && current[0] != 'g' && current[0] != 'o')
    {
      fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
      free(temp);
      exit(EXIT_FAILURE);
    }

    // Second character should always be =
    if (current[1] != '=')
    {
      fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
      free(temp);
      exit(EXIT_FAILURE);
    }

    int index = 2;

    while(current[index])
    {
    if (current[index] != 'r' && current[index] != 'w' && current[index] != 'x')
    {
      fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
      free(temp);
      exit(EXIT_FAILURE);
    }

    switch(current[0])
    {
      case 'u':
        switch(current[index])
        {
          case 'r':
            nr |= S_IRUSR;
            break;

          case 'w':
            nr |= S_IWUSR;
            break;

          case 'x':
            nr |= S_IXUSR;
            break;

          default:
            fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
            free(temp);
            exit(EXIT_FAILURE);
        }
        break;

      case 'g':
        switch(current[index])
        {
          case 'r':
            nr |= S_IRGRP;
            break;

          case 'w':
            nr |= S_IWGRP;
            break;

          case 'x':
            nr |= S_IXGRP;
            break;

          default:
            fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
            free(temp);
            exit(EXIT_FAILURE);
        }
        break;

      case 'o':
        switch(current[index])
        {
          case 'r':
            nr |= S_IROTH;
            break;

          case 'w':
            nr |= S_IWOTH;
            break;

          case 'x':
            nr |= S_IXOTH;
            break;

          default:
            fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
            free(temp);
            exit(EXIT_FAILURE);
        }
        break;

      default:
      fprintf(stderr, "chmod: invalid mode: `%s`\n", str);
      free(temp);
      exit(EXIT_FAILURE);
    }

    index++;
  }
  current = strtok(NULL, ",");
  }
  free(temp);
  return nr;
}

int get_perm_number(const char *str)
{
  // Finding whether the mode is numeric or symbolic
  int numeric = TRUE;
  for (int index = 0; str[index]; ++index)
    if (!isdigit(str[index]))
    {
      numeric = FALSE;
      break;
    }

  if (numeric)
    return get_perm_number_numeric(str);
  return get_perm_number_symbolic(str);
}

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    fprintf(stderr, "Not enough arguments given!\n");
    exit(EXIT_FAILURE);
  }

  // Get permissions
  int perm = get_perm_number(argv[1]);

  for (int index = 2; index < argc; ++index)
  {
    if(chmod(argv[index], perm) != 0)
      fprintf(stderr, "chmod: cannot access '%s': No such file or directory\n", argv[index]);
  }
  return 0;
}
