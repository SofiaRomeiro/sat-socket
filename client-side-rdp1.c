#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_IP "10.10.10.7"
#define SERVER_PORT 12345
#define CLIENT_IP 0
#define CLIENT_PORT 0
#define MIN_PACKET_SIZE 10 //bytes
#define MAX_PACKET_SIZE 256 //bytes

// Function to generate a random string of variable length
char *generateRandomString() {
    int length = rand() % (MAX_PACKET_SIZE - MIN_PACKET_SIZE + 1) + MIN_PACKET_SIZE;
    
    char *random_string = (char *)malloc(length + 1);  // +1 for the null terminator
    if (random_string == NULL) {
        perror("Memory allocation error");
        exit(1);
    }
    
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charset_length = sizeof(charset) - 1;
    
    for (int i = 0; i < length; i++) {
        random_string[i] = charset[rand() % charset_length];
    }
    
    random_string[length] = '\0';  // Null-terminate the string
    return random_string;
}

int main() {

    int client_socket;
    struct sockaddr_in server_addr;
    time_t t_send;
    time_t t_recv;

    srand(time(NULL));
    
//  --------------------------------- Create UDP socket -------------------------------------------------
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket");
        exit(1);
    }

//  --------------------------------- Configure Server Address -------------------------------------------------
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

// -------------------------- Generate the packet to send --------------------------

    while(1) {

        char *packet = generateRandomString();
        printf("Packet size: %u\n", sizeof(packet));

        time(&t_send);

        // Send data to the server
        sendto(client_socket, packet, sizeof(packet), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        free(packet);  // Remember to free the allocated memory when done

        // Receive a response from the server (optional)
        char buffer[MAX_PACKET_SIZE];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer), 0, NULL, NULL);
        time(&t_recv);
        if (bytes_received == -1) {
            perror("Error receiving data");
            exit(1);
        }

        printf("Received from server: %s\n", buffer);

        double diff = difftime(t_recv, t_send);
        printf("Tx: %.2f seconds\n", diff);

        sleep(3);

    }
    
    // Close the socket
    close(client_socket);

    return 0;
}
