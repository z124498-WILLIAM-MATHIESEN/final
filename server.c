// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define MAX_FILE_SIZE (1024 * 1024 * 50) // 50 MB limit

typedef struct {
    int client_socket;
} ClientThreadArgs;

int client_sockets[MAX_CLIENTS] = {0};
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_file(const char *file_name, size_t file_size, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0 && client_sockets[i] != sender_socket) {
            // Send file name
            send(client_sockets[i], file_name, strlen(file_name), 0);
            usleep(1000); // Ensure file name is sent before file size

            // Send file size
            send(client_sockets[i], &file_size, sizeof(file_size), 0);
            usleep(1000); // Ensure file size is sent before file content

            // Open the file and send it in chunks
            FILE *file = fopen(file_name, "rb");
            if (!file) {
                perror("Failed to open file for broadcasting");
                continue;
            }

            char buffer[BUFFER_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                send(client_sockets[i], buffer, bytes_read, 0);
            }
            fclose(file);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_handler(void *arg) {
    ClientThreadArgs *args = (ClientThreadArgs *)arg;
    int client_socket = args->client_socket;
    free(args);

    char file_name[BUFFER_SIZE];
    size_t file_size;
    char buffer[BUFFER_SIZE];

    // Receive file name
    if (read(client_socket, file_name, BUFFER_SIZE) <= 0) {
        perror("Failed to receive file name");
        close(client_socket);
        return NULL;
    }
    printf("Receiving file: %s\n", file_name);

    // Receive file size
    if (read(client_socket, &file_size, sizeof(file_size)) <= 0) {
        perror("Failed to receive file size");
        close(client_socket);
        return NULL;
    }

    if (file_size > MAX_FILE_SIZE) {
        fprintf(stderr, "File size %zu exceeds the maximum allowed size.\n", file_size);
        close(client_socket);
        return NULL;
    }

    printf("File size: %zu bytes\n", file_size);

    // Write file content to disk directly
    FILE *file = fopen(file_name, "wb");
    if (!file) {
        perror("Failed to create file on server");
        close(client_socket);
        return NULL;
    }

    size_t bytes_received = 0;
    while (bytes_received < file_size) {
        ssize_t bytes = read(client_socket, buffer, BUFFER_SIZE);
        if (bytes <= 0) {
            perror("Failed to receive file content");
            fclose(file);
            close(client_socket);
            return NULL;
        }
        fwrite(buffer, 1, bytes, file);
        bytes_received += bytes;
    }
    fclose(file);
    printf("File '%s' received successfully.\n", file_name);

    // Broadcast the file to other clients
    broadcast_file(file_name, file_size, client_socket);

    close(client_socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addr_len = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allow port reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Bind socket to address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        printf("New client connected.\n");

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = client_socket;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Create a thread for each client
        pthread_t thread_id;
        ClientThreadArgs *args = malloc(sizeof(ClientThreadArgs));
        args->client_socket = client_socket;
        if (pthread_create(&thread_id, NULL, client_handler, (void *)args) != 0) {
            perror("Failed to create thread");
            free(args);
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
