#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_IP "10.10.10.7"
#define SERVER_PORT 12346
#define CLIENT_IP 0
#define CLIENT_PORT 0
#define MIN_PACKET_SIZE 10 //bytes
#define MAX_PACKET_SIZE 256 //bytes


int main() {

    int client_socket;
    struct sockaddr_in server_addr;
    
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
		// Receive packet from the server
		char buffer[MAX_PACKET_SIZE];
		memset(buffer, 0, sizeof(buffer));
		int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer), 0, NULL, NULL);
		if (bytes_received == -1) {
			perror("Error receiving data");
			exit(1);
		}

		printf("Received from server: %s\n", buffer);

		// Send data to the server
		sendto(client_socket, "ACK", sizeof("ACK"), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

	}    

    // Close the socket
    close(client_socket);
    return 0;
}
