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
char* generateRandomString() {
    srand(time(NULL));
    int num = rand();
    int length = (num % MAX_PACKET_SIZE);
    printf("Length: %d\n", length);
    if (length < MIN_PACKET_SIZE) {
        length += MIN_PACKET_SIZE;
    }

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
    struct timespec t_send;
    struct timespec t_recv;


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
        printf("Packet size: %u\n", strlen(packet));

        clock_gettime(CLOCK_REALTIME,&t_send);

        // Send data to the server
        sendto(client_socket, packet, strlen(packet), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        free(packet);

        // Receive a response from the server (optional)
        char buffer[MAX_PACKET_SIZE];
        memset(buffer, '\0', MAX_PACKET_SIZE);
        int bytes_received = recvfrom(client_socket, buffer, MAX_PACKET_SIZE, 0, NULL, NULL);
        clock_gettime(CLOCK_REALTIME,&t_recv);
        if (bytes_received == -1) {
            perror("Error receiving data");
            exit(1);
        }

        printf("Received from server: %s\n", buffer);

        double diff = ((t_recv.tv_sec - t_send.tv_sec)*1000) + ((t_recv.tv_nsec - t_send.tv_nsec) / 1000000.0);
        printf("Tx: %.5lf miliseconds\n", diff);

        //sleep(3);

    }

    // Close the socket
    close(client_socket);

    return 0;
}