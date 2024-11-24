#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 2727

void on_exit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int safe_socket(int domain, int type, int protocol) {
  int res = socket(domain, type, protocol);

  if (res < 0)
    on_exit("Socket creation has failed");

  return res;
}

int main() {
  struct sockaddr_in server_addr;
  char *message = "Hello, server!";
  int socket = safe_socket(AF_INET, SOCK_STREAM, 0);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  int bind_addr = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
  if (bind_addr <= 0)
    on_exit("This address is not supported\n");

  int conn_res = connect(socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (conn_res < 0) {
    on_exit("Connection has failed\n");
  }

  send(socket, message, strlen(message), 0);
  printf("The message has been sent\n");
  close(socket);

  return 0;
}
