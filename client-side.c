#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_IP "10.10.10.7"
#define SERVER_PORT 12345
#define CLIENT_IP 
#define CLIENT_PORT


int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    
    // Create a UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket");
        exit(1);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);  // Same port as the server
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // Replace with the actual server's IP

    char message[] = "Hello, server!";
    
    // Send data to the server
    sendto(client_socket, message, sizeof(message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Receive a response from the server (optional)
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer), 0, NULL, NULL);
    if (bytes_received == -1) {
        perror("Error receiving data");
        exit(1);
    }

    printf("Received from server: %s\n", buffer);

    // Close the socket
    close(client_socket);

    return 0;
}
