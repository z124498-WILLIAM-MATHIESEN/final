// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void send_file(int sock, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // Send file name
    const char *file_name = strrchr(file_path, '/');
    if (!file_name) file_name = file_path;
    else file_name++;
    send(sock, file_name, strlen(file_name), 0);
    printf("File name sent: %s\n", file_name);

    // Send file size
    struct stat file_stat;
    stat(file_path, &file_stat);
    size_t file_size = file_stat.st_size;
    send(sock, &file_size, sizeof(file_size), 0);

    // Send file content in chunks
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }
    fclose(file);
    printf("File '%s' uploaded successfully.\n", file_name);
}

void receive_file(int sock) {
    char file_name[BUFFER_SIZE];
    size_t file_size;

    // Receive file name
    ssize_t bytes = read(sock, file_name, BUFFER_SIZE);
    if (bytes <= 0) {
        perror("Failed to receive file name");
        return;
    }
    file_name[bytes] = '\0'; // Ensure null-termination
    printf("Receiving file: %s\n", file_name);

    // Receive file size
    if (read(sock, &file_size, sizeof(file_size)) <= 0) {
        perror("Failed to receive file size");
        return;
    }
    printf("File size: %zu bytes\n", file_size);

    // Open file for writing
    FILE *file = fopen(file_name, "wb");
    if (!file) {
        perror("Failed to create file");
        return;
    }

    size_t bytes_received = 0;
    char buffer[BUFFER_SIZE];
    while (bytes_received < file_size) {
        bytes = read(sock, buffer, BUFFER_SIZE);
        if (bytes <= 0) {
            perror("Failed to receive file content");
            fclose(file);
            return;
        }
        fwrite(buffer, 1, bytes, file);
        bytes_received += bytes;
        printf("Received %zu/%zu bytes\n", bytes_received, file_size);
    }

    fclose(file);
    printf("File '%s' received and saved successfully.\n", file_name);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        // This line attempts to connect to the server at the specified IP and port.
        // If the server is unreachable or not running, it will fail and print an error.
        perror("Connection failed");
        return -1;
    }
    printf("Connected to server.\n");

    if (argc == 2) {
        // Check if a file path argument is provided. If so, upload the file to the server.
        send_file(sock, argv[1]);
    } else {
        printf("Listening for file from server...\n");
        receive_file(sock);
    }

    close(sock);
    return 0;
}
