/* Compiled as gcc -Wall -g -o client client.c
*/
#include "UtilIO.c"

int main(int argc, char **argv)
{
    // Check whether enough arguments were given
    if (argc != 3)
      print_and_exit("Incorrect number of arguments: expected ./client IP port\n");

    char *IP;
    int port;

    IP = argv[1];
    port = atoi(argv[2]);

    // create a socket used to communicate with the server
    int to_server;
    if ((to_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      print_and_exit("client: failed to create socket\n");

    struct sockaddr_in serverDetails;
    serverDetails.sin_addr.s_addr = inet_addr(IP);
    serverDetails.sin_family = AF_INET;
    serverDetails.sin_port = htons(port);

    // Check whether the IP is valid
    if (inet_pton(AF_INET, IP, &(serverDetails.sin_addr)) == 0)
      print_and_exit("client: invalid IP\n");

    // Connect to the server
    if (connect(to_server, (struct sockaddr *)&serverDetails, sizeof(serverDetails)) < 0)
      print_and_exit("client: failed to connect to server!\n");

    char *cmd = malloc(1000 * sizeof(char));

    printf("%s$ Client connected, please enter command to send%s: ", YELLOW, RESET);
    size_t buff_size = 999;
    strcpy(cmd, "");
    getline(&cmd, &buff_size, stdin);

    // Send command to server
    if (send(to_server, cmd, strlen(cmd), 0) < 0)
    {
        free(cmd);
        print_and_exit("client: failed to send command to server!\n");
    }

    char response[1000];

    // Get the server response
    int size = 0;
    while ((size = recv(to_server, response, 999, 0)) > 0)
    {
        response[size] = '\0';
        printf("%s\n", response);
    }

    // Free memory
    free(cmd);
    close(to_server);
    return 0;
}
