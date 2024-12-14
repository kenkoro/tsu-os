#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 2727
#define BACKLOG 5
#define BUFSIZE 2048

volatile sig_atomic_t sighup_received = 0;

void handler() { sighup_received = 1; }

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

int safe_bind(int socket_fd, const struct sockaddr *addr, socklen_t addrlen) {
  int res = bind(socket_fd, addr, addrlen);

  if (res < 0)
    on_exit("Binding has failed");

  return res;
}

int safe_listen(int socket_fd, int backlog) {
  int res = listen(socket_fd, backlog);

  if (res < 0)
    on_exit("Couldn't perform listening");

  return res;
}

int safe_accept(int socket_fd, struct sockaddr *addr, socklen_t *addrlen) {
  int res = accept(socket_fd, addr, addrlen);

  if (res < 0)
    on_exit("Couldn't accept the socket");

  return res;
}

int max(int a, int b) { return a > b ? a : b; }

int main() {
  int max_fd, bytes;
  int incoming_socket_fd = 0;
  struct sockaddr_in socket_addr;
  struct sigaction s_action;
  fd_set readfds;
  sigset_t blocked_mask, orig_mask;
  char buf[BUFSIZE] = {0};

  int server_socker_fd = safe_socket(AF_INET, SOCK_STREAM, 0);

  socket_addr.sin_family = AF_INET;
  int bind_addr = inet_pton(AF_INET, "127.0.0.1", &socket_addr.sin_addr);
  if (bind_addr <= 0)
    on_exit("This address is not supported\n");
  socket_addr.sin_port = htons(PORT);

  safe_bind(server_socker_fd, (struct sockaddr *)&socket_addr,
            sizeof(socket_addr));
  safe_listen(server_socker_fd, BACKLOG);

  sigaction(SIGHUP, NULL, &s_action);
  s_action.sa_handler = handler;
  s_action.sa_flags |= SA_RESTART;
  sigaction(SIGHUP, &s_action, NULL);

  sigemptyset(&blocked_mask);
  sigemptyset(&orig_mask);
  sigaddset(&blocked_mask, SIGHUP);
  sigprocmask(SIG_BLOCK, &blocked_mask, &orig_mask);

  for (;;) {
    FD_ZERO(&readfds);
    FD_SET(server_socker_fd, &readfds);

    if (incoming_socket_fd > 0)
      FD_SET(incoming_socket_fd, &readfds);

    max_fd = max(incoming_socket_fd, server_socker_fd);

    if (pselect(max_fd + 1, &readfds, NULL, NULL, NULL, &orig_mask) < 0 &&
        errno != EINTR) {
      on_exit("Couldn't monitor fds\n");
    }

    if (sighup_received) {
      printf("Sighup signal has been received\n");
      sighup_received = 0;
      continue;
    }

    if (incoming_socket_fd > 0 && FD_ISSET(incoming_socket_fd, &readfds)) {
      bytes = read(incoming_socket_fd, buf, BUFSIZE);

      if (bytes > 0) {
        printf("Received bytes: %d\n\n", bytes);
      } else {
        if (bytes == 0) {
          close(incoming_socket_fd);
          incoming_socket_fd = 0;
        } else {
          perror("Couldn't read incoming bytes of the socket fd\n");
        }
      }

      continue;
    }

    if (FD_ISSET(server_socker_fd, &readfds)) {
      int addrlen = sizeof(socket_addr);
      incoming_socket_fd =
          safe_accept(server_socker_fd, (struct sockaddr *)&socket_addr,
                      (socklen_t *)&addrlen);

      printf("New connection has been established: %d\n", incoming_socket_fd);
    }
  }
  close(server_socker_fd);

  return 0;
}
