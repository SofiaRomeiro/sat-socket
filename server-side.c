#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_IP "10.10.10.7"
#define SERVER_PORT 12345
#define CLIENT_IP 
#define CLIENT_PORT

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    // Create a UDP socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(1);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);  // Port number
    server_addr.sin_addr.s_addr = SERVER_IP;

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding server socket");
        exit(1);
    }

    char buffer[1024];
    
    while (1) {
        // Receive data from the client
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (bytes_received == -1) {
            perror("Error receiving data");
            exit(1);
        }

        printf("Received: %s\n", buffer);

        // Respond to the client (optional)
        sendto(server_socket, "Hello, client!", 13, 0, (struct sockaddr*)&client_addr, client_addr_len);
    }

    // Close the socket
    close(server_socket);

    return 0;
}
