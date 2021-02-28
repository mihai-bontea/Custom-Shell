/* Compiled as gcc -Wall -g -o server server.c -pthread
*/
#include "UtilIO.c"
#define PORT 5000

/* Receives a file descriptor from which it reads a command
 * and to which it sends the output of the shell
*/
void *serve_connection(void *arg)
{
  int fd = *((int *)arg);

  char client_cmd[256];
  char tmp[256];

  // Get client command
  int size = recv(fd, client_cmd, 255, 0);
  client_cmd[size] = '\0';
  printf("%sClient command: %s%s\n",GREEN, client_cmd, RESET);

  // Set up the arguments for the shell
  char **shell_args = malloc(3 * sizeof(char *));
  shell_args[0] = (char *)"main";
  shell_args[1] = client_cmd;
  shell_args[2] = NULL;

  // Create a new process to run the shell in
  pid_t pid;
  if ((pid = fork()) == 0)
  {
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    sprintf(tmp, "%s$ Server returned:%s\n", GREEN, RESET);
    send(fd, tmp, strlen(tmp), 0);

    execv(shell_args[0], shell_args);
    fprintf(stderr, "server: Execution should not reach here.\n");
    exit(EXIT_FAILURE);
  }
  waitpid(pid, NULL, 0);

  sprintf(tmp, "%s%s%s\n", RED, "End of communication.", RESET);
  send(fd, tmp, strlen(tmp), 0);

  close(fd);
  return NULL;
}

int main(int argc, char **argv)
{
    int server_socket;
    const int optval = TRUE;

    // Initializing the socket used to connect to the server
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      print_and_exit("server: failed to create socket!\n");

    // Set port to reusable
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
      print_and_exit("server: setsockopt error!\n");

    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Binding server to socket
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0)
      print_and_exit("server: failed to bind server to socket!\n");

    // Listen for connections
    if (listen(server_socket, 1) < 0)
      print_and_exit("server: listen error!\n");

    int connection_socket;
    size_t size = sizeof(struct sockaddr_in);
    printf("%s%sServer started%s\n", WHITE_BACKGROUND, BLACK, RESET);

    // Accept as many connections and assign a thread to each one
    int nr_connected = 0;
    while ((connection_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&size)))
    {
        printf("%s%d Connected.%s\n",YELLOW, ++nr_connected, RESET);
        pthread_t th;
        pthread_create(&th, NULL, &serve_connection, &connection_socket);
    }

    close(server_socket);
    return 0;
}
