#include "srv.h"

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];

    // Lecture de la requête HTTP
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; 
        printf("Reçu du client :\n%s\n", buffer);

        char method[16], path[256];
        sscanf(buffer, "%15s %255s", method, path);
        printf("Méthode : %s, Chemin : %s\n", method, path);

        // Gestion des méthodes
        if (strcmp(method, "GET") == 0) {
            handle_get_request(path, client_socket);
        } else if (strcmp(method, "POST") == 0) {
            handle_post_request(buffer, client_socket);
        } else if (strcmp(method, "HEAD") == 0) {
            handle_head_request(client_socket);
        } else {
            const char *response =
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 23\r\n"
                "\r\n"
                "Method Not Allowed\n";
            send(client_socket, response, strlen(response), 0);
        }
    } else {
        perror("Erreur lors de la réception des données");
    }

    close(client_socket);
}

void handle_get_request(const char *path, int client_socket) {
    char local_path[512] = "./";
    strncat(local_path, path + 1, sizeof(local_path) - strlen(local_path) - 1);

    struct stat file_stat;
    if (stat(local_path, &file_stat) != 0) {
        send_404_response(client_socket);
        return;
    }

    const char *html_response =
        "<html>"
        "<head><title>Bienvenue</title></head>"
        "<body><h1>Hello, world!</h1><p>Bienvenue sur mon serveur HTTP.</p></body>"
        "</html>";

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             strlen(html_response), html_response);
    send(client_socket, response, strlen(response), 0);
}

void handle_post_request(const char *buffer, int client_socket) {
    char *body = strstr(buffer, "\r\n\r\n");
    if (body != NULL) {
        body += 4; 
        printf("Données reçues dans le POST : %s\n", body);
    }

    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 20\r\n"
        "\r\n"
        "POST reçu avec succès\n";
    send(client_socket, response, strlen(response), 0);
}

void handle_head_request(int client_socket) {
    const char *html_response =
        "<html>"
        "<head><title>Bienvenue</title></head>"
        "<body><h1>Hello, world!</h1><p>Bienvenue sur mon serveur HTTP.</p></body>"
        "</html>";

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "\r\n",
             strlen(html_response));
    send(client_socket, response, strlen(response), 0);
}

void send_404_response(int client_socket) {
    const char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Page non trouvée\n";
    send(client_socket, response, strlen(response), 0);
}

void reap_zombies(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : 8000; // Par défaut : port 8000

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t taille_addr = sizeof(client_addr);

    signal(SIGCHLD, reap_zombies);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 5) < 0) {
        perror("listen");
        close(server_socket);
        return -1;
    }

    printf("En écoute sur 127.0.0.1:%d\n", port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &taille_addr);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        printf("Client connecté depuis %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_socket);
        } else if (pid == 0) {
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        } else {
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}
