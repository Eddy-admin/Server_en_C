#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define BUFFER_SIZE 2048

void handle_client(int client_socket);
void handle_get_request(const char *path, int client_socket);
void handle_post_request(const char *buffer, int client_socket);
void handle_head_request(int client_socket);
void send_404_response(int client_socket);
void reap_zombies(int sig);

#endif // SERVER_H
