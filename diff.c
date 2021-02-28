/* Compiled as gcc -Wall -g -o diff diff.c
*/
#include "UtilIO.c"

typedef struct
{
  int i, j;
}Pair;

int compare_brief(char **file1_tokens, int n, char **file2_tokens, int m)
{
  // The files have different number of lines
  if (n != m)
    return FALSE;

  // Compare each line
  for (int index = 0; index < n; ++index)
    if (strcmp(file1_tokens[index], file2_tokens[index]))
      return FALSE;

  return TRUE;
}

void free_tokens(char **file1_tokens, int n, char **file2_tokens, int m)
{
  for (int index = 0; index < n; ++index)
    free(file1_tokens[index]);
  free(file1_tokens);

  for (int index = 0; index < m; ++index)
    free(file2_tokens[index]);
  free(file2_tokens);
}

int LCS_length(int **C, char **file1_tokens, int n, char **file2_tokens, int m)
{
  // Initialize the first row and column of the matrix with 0
  for (int i = 0; i <= n; ++i)
    C[i][0] = 0;
  for (int j = 0; j <= m; ++j)
    C[0][j] = 0;

  int length = 0;
  for (int i = 1; i <= n; ++i)
    for (int j = 1; j <= m; ++j)
    {
      if (!strcmp(file1_tokens[i - 1], file2_tokens[j - 1]))
      {
        C[i][j] = C[i - 1][j - 1] + 1;
        if (C[i][j] > length)
          length = C[i][j];
      }
      else
        C[i][j] = ((C[i][j - 1] > C[i - 1][j])? C[i][j - 1] : C[i - 1][j]);
    }
  return length;
}

void compute_LCS(Pair *LCS_indexes, int **C, char **file1_tokens, char**file2_tokens, int i, int j, int k)
{
  if (i == 0 || j == 0 || k == -1)
    return;

  //printf("this: %d %d %d\n", i, j, k);
  if (!strcmp(file1_tokens[i - 1], file2_tokens[j - 1]))
  {
    LCS_indexes[k].i = i;
    LCS_indexes[k].j = j;
    compute_LCS(LCS_indexes, C, file1_tokens, file2_tokens, i - 1, j - 1, k - 1);
  }
  else if (C[i][j - 1] > C[i - 1][j])
    compute_LCS(LCS_indexes, C, file1_tokens, file2_tokens, i, j - 1, k);
  else
    compute_LCS(LCS_indexes, C, file1_tokens, file2_tokens, i - 1, j, k);
}

void produce_output(Pair *LCS_indexes, int length, char **file1_tokens, int n, char **file2_tokens, int m)
{
  int displacement = 0;
  for (int index = 0; index < length; ++index)
  {
    if ((LCS_indexes[index].i + displacement != LCS_indexes[index].j) ||
    (LCS_indexes[index].i != 1 && LCS_indexes[index].j != 1 && index == 0) ||
    (index != 0 && (LCS_indexes[index].i - 1 != LCS_indexes[index - 1].i) &&
    (LCS_indexes[index].j - 1 != LCS_indexes[index - 1].j)))
    {
      // 'a' case
      if (LCS_indexes[index].i == 1 || (index != 0 && LCS_indexes[index - 1].i + 1 == LCS_indexes[index].i))
      {
        int start_j = 0, end_j = 0;
        // Printing the beginning line and 'a'
        printf("%da", LCS_indexes[index].i - 1);
        if (LCS_indexes[index].i + displacement + 1 == LCS_indexes[index].j)
        {
          printf("%d\n", LCS_indexes[index].j - 1);
          start_j = end_j = LCS_indexes[index].j - 2;
        }
        else
        {
          printf("%d,%d\n", LCS_indexes[index - 1].j + 1 , LCS_indexes[index].j - 1);
          start_j = LCS_indexes[index - 1].j;
          end_j = LCS_indexes[index].j - 2;
        }

        // Printing the lines that need to be added
        for (int line_index = start_j; line_index <= end_j; ++line_index)
          printf("> %s", file2_tokens[line_index]);

        // Updating the displacement
        displacement += (end_j - start_j + 1);
      }
      // 'd' case
      else if (LCS_indexes[index].j == 1 || (index != 0 && LCS_indexes[index - 1].j + 1 == LCS_indexes[index].j))
      {
        int start_i = 0, end_i = 0;
        if (LCS_indexes[index].i + displacement - 1 == LCS_indexes[index].j)
        {
          printf("%dd", LCS_indexes[index].i - 1);
          start_i = end_i = LCS_indexes[index].i - 2;
        }
        else
        {
          int del_start = (index != 0)? (LCS_indexes[index - 1].i + 1) : 1;
          printf("%d,%dd", del_start, LCS_indexes[index].i - 1);
          start_i = del_start - 1;
          end_i = LCS_indexes[index].i - 2;
        }

        // Printing the line at which they sync
        printf("%d\n", start_i);

        // Printing the lines that need to be deleted
        for (int line_index = start_i; line_index <= end_i; ++line_index)
          printf("< %s", file1_tokens[line_index]);

        // Updating the displacement
        displacement -= (end_i - start_i + 1);
      }
      // 'c' case
      else
      {
        int start_i = 0, end_i = 0, start_j = 0, end_j = 0;

        // Printing range from file1 and 'c'
        if (LCS_indexes[index].i == 2 || (index != 0 && LCS_indexes[index - 1].i + 2 == LCS_indexes[index].i))
        {
          printf("%dc", LCS_indexes[index].i - 1);
          start_i = end_i = LCS_indexes[index].i - 2;
        }
        else
        {
          int c_start = (index != 0)? (LCS_indexes[index - 1].i + 1) : 1;
          printf("%d,%dc", c_start, LCS_indexes[index].i - 1);
          start_i = c_start - 1;
          end_i = LCS_indexes[index].i - 2;
        }

        // Printing range from file2
        if (LCS_indexes[index].j == 2 || (index != 0 && LCS_indexes[index - 1].j + 2 == LCS_indexes[index].j))
        {
          printf("%d\n", LCS_indexes[index].j - 1);
          start_j = end_j = LCS_indexes[index].j - 2;
        }
        else
        {
          int c_start = (index != 0)? (LCS_indexes[index - 1].j + 1) : 1;
          printf("%d,%d\n", c_start, LCS_indexes[index].j - 1);
          start_j = c_start - 1;
          end_j = LCS_indexes[index].j - 2;
        }

        // Printing the lines of file1 that need to be changed
        for (int line_index = start_i; line_index <= end_i; ++line_index)
          printf("< %s", file1_tokens[line_index]);

        // Printing the three separating dashes
        printf("---\n");

        // Printing the lines of file2 to which we change
        for (int line_index = start_j; line_index <= end_j; ++line_index)
          printf("> %s", file2_tokens[line_index]);

        // Updating the displacement
        displacement += ((end_j - start_j) - (end_i - start_i));
      }
    }
  }

}

int main(int argc, char **argv)
{
  // Default values, can be changed by giving extra arguments
  int brief = FALSE;
  int as_text = FALSE;

  // Parsing the arguments
  int c;
  while((c = getopt(argc, argv, "aq")) != -1)
    switch(c)
    {
      case 'a':
        as_text = TRUE;
        break;

      case 'q':
        brief = TRUE;
        break;

      case '?':
        exit(EXIT_FAILURE);
    }

  if (as_text)
  {
    printf("Files will be considered as text.\n");
  }

  // Checking if at least 2 arguments were given
  if (optind + 1 >= argc)
    print_and_exit("diff: missing operand after 'diff'\n");

  // Checking for extra operands
  if (optind + 2 != argc)
    print_and_exit("diff: extra operands given!\n");

  // Both arguments cannot be from stdin at the same time
  if (strcmp(argv[optind], "-") == 0 && strcmp(argv[optind + 1], "-") == 0)
    print_and_exit("diff: at least one argument should be a filename!\n");

  char *file1_contents = NULL;
  char *file2_contents = NULL;

  // Read file1 from stdin or file
  if (strcmp(argv[optind], "-") == 0)
    read_from_stdin(&file1_contents);
  else{
    int res = read_from_file(argv[optind], &file1_contents);
    switch(res)
    {
      case 1:
        fprintf(stderr, "diff: Error opening file %s\n", argv[optind]);
        exit(EXIT_FAILURE);
      case 2:
        fprintf(stderr, "diff: Cannot allocate enough memory for file %s\n", argv[optind]);
        exit(EXIT_FAILURE);
      case 3:
        fprintf(stderr, "diff: Cannot read %s\n", argv[optind]);
        exit(EXIT_FAILURE);
      default:
        break;
    }
  }
  // Read file2 from stdin or file
  if (strcmp(argv[optind + 1], "-") == 0)
    read_from_stdin(&file2_contents);
  else{
    int res = read_from_file(argv[optind + 1], &file2_contents);
    switch(res)
    {
      case 1:
        fprintf(stderr, "diff: Error opening file %s\n", argv[optind + 1]);
        exit(EXIT_FAILURE);
      case 2:
        fprintf(stderr, "diff: Cannot allocate enough memory for file %s\n", argv[optind + 1]);
        exit(EXIT_FAILURE);
      case 3:
        fprintf(stderr, "diff: Cannot read %s\n", argv[optind + 1]);
        exit(EXIT_FAILURE);
      default:
        break;
    }
  }

  // Split both files by '\n'
  char **file1_tokens = NULL;
  char **file2_tokens = NULL;
  int nr_lines_file1 = 0, nr_lines_file2 = 0;

  split_by_char(file1_contents, '\n', &file1_tokens, &nr_lines_file1);
  split_by_char(file2_contents, '\n', &file2_tokens, &nr_lines_file2);

  // Free the redundant memory
  free(file1_contents);
  free(file2_contents);

  // If brief option is selected, say only whether the files differ or not
  if (brief)
  {
    if (!compare_brief(file1_tokens, nr_lines_file1, file2_tokens, nr_lines_file2))
      printf("Files %s and %s differ\n", argv[optind], argv[optind + 1]);

    free_tokens(file1_tokens, nr_lines_file1, file2_tokens, nr_lines_file2);
    exit(0);
  }

  // Initialize a matrix which will hold the result of the LCS algorithm
  int **C = (int **)malloc((nr_lines_file1 + 1) * sizeof(int *));
  for (int index = 0; index <= nr_lines_file1; ++index)
    C[index] = (int *)malloc((nr_lines_file2 + 1) * sizeof(int));

  // Apply the LCS algorithm
  int length = LCS_length(C, file1_tokens, nr_lines_file1, file2_tokens, nr_lines_file2);

  // Allocate and compute the array of pairs (i, j) for which file1[i] = file2[j]
  Pair *LCS_indexes = malloc(length * sizeof(Pair));

  compute_LCS(LCS_indexes, C, file1_tokens, file2_tokens, nr_lines_file1, nr_lines_file2, length - 1);

  // Free the redundant memory
  for (int index = 0; index <= nr_lines_file1; ++index)
    free(C[index]);
  free(C);

  produce_output(LCS_indexes, length, file1_tokens, nr_lines_file1, file2_tokens, nr_lines_file2);

  return 0;
}
