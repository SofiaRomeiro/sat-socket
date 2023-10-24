#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVER_IP "10.10.10.7"
#define SERVER_PORT_RDP1 12345
#define SERVER_PORT_RDP2 12346
#define LINK_SPEED 1200 //bit/s
#define MAX_BUFFER_SIZE 2049 //max packet size is 2048
#define DELAYED_INTERFACE "eth1" 
#define ADD 1
#define DEL 0

float delay = 1000;

// This function is responsible for, given a packet size, computing the delay (for links with speed=1200 bit/s)
// WARNING: the packet size is given in bytes so a conversion to bits is needed
float link_delay_tx(int packet_size) {
	printf("Received packet size: %d\n", packet_size);
	float delay = (packet_size * 8.0)/LINK_SPEED;
	printf("Generated delay: %f for packet size %d\n", delay, packet_size);
	return delay*1000; //conversion to milisseconds
}

int run_command(float delay, int action) {

	FILE *fp;
	char path[1035];

	char* command = "";
	if (action) {
		printf("tc qdisc add dev %s root netem delay %fms\n", DELAYED_INTERFACE, delay, command);
	}
	else {
		printf("tc qdisc del dev %s root netem delay %fms\n", DELAYED_INTERFACE, delay, command);
	}  

	/* Open the command for reading. */
	fp = popen(command, "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}

	printf("Output:\n");
  /* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path), fp) != NULL) {
		printf("%s", path);
	}

  /* close */
	pclose(fp);

  	return 0;
}

void link_socket() {
	int server_socket_rdp1;
	int server_socket_rdp2;
    struct sockaddr_in server_addr_rdp1;
	struct sockaddr_in server_addr_rdp2;
	struct sockaddr_in client_addr_rdp1;
	struct sockaddr_in client_addr_rdp2;
	socklen_t client_addr_rdp1_len = sizeof(client_addr_rdp1);
	socklen_t client_addr_rdp2_len = sizeof(client_addr_rdp2);

//  --------------------------------- Create UDP sockets -------------------------------------------------
    server_socket_rdp1 = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_rdp1 == -1) {
        perror("Error creating server socket");
        exit(1);
    }

	server_socket_rdp2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_rdp2 == -1) {
        perror("Error creating server socket");
        exit(1);
    }

//  --------------------------------- Configure Server Addresses -------------------------------------------------

    server_addr_rdp1.sin_family = AF_INET;
    server_addr_rdp1.sin_port = htons(SERVER_PORT_RDP1);
    server_addr_rdp1.sin_addr.s_addr = inet_addr(SERVER_IP); 

	server_addr_rdp2.sin_family = AF_INET;
    server_addr_rdp2.sin_port = htons(SERVER_PORT_RDP2);
    server_addr_rdp2.sin_addr.s_addr = inet_addr(SERVER_IP); 

//  --------------------------------- Bind Server Sockets -------------------------------------------------


    // Bind the socket to the specific interface address
    if (bind(server_socket_rdp1, (struct sockaddr*)&server_addr_rdp1, sizeof(server_addr_rdp1)) == -1) {
        perror("Error binding server socket");
        exit(1);
    }

	    // Bind the socket to the specific interface address
    if (bind(server_socket_rdp2, (struct sockaddr*)&server_addr_rdp2, sizeof(server_addr_rdp2)) == -1) {
        perror("Error binding server socket");
        exit(1);
    }

//  --------------------------------- Receive from RDP1 and RDP2 -------------------------------------------------

	char buffer_rdp1[MAX_BUFFER_SIZE];
	char buffer_rdp2[MAX_BUFFER_SIZE];
    
    while (1) {
        // Receive data from rdp1
        memset(buffer_rdp1, 0, MAX_BUFFER_SIZE);
        int bytes_received = recvfrom(server_socket_rdp1, buffer_rdp1, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&client_addr_rdp1, &client_addr_rdp1_len);
        if (bytes_received == -1) {
            perror("Error receiving data");
            exit(1);
        }

        printf("Received from RDP1: %s\n", buffer_rdp1);

		// Process delay and call netemu

		system("sudo qdisc del dev eth1 root");

		delay = link_delay_tx(bytes_received);

		run_command(delay, ADD);

		printf("Forwarding to RDP2\n");

        // Forward to RDP2
        sendto(server_socket_rdp2, buffer_rdp1, 13, 0, (struct sockaddr*)&client_addr_rdp2, client_addr_rdp2_len);

		// Wait for RDP2 answer
		memset(buffer_rdp2, 0, sizeof(buffer_rdp2));
        bytes_received = recvfrom(server_socket_rdp2, buffer_rdp2, sizeof(buffer_rdp2), 0, (struct sockaddr*)&client_addr_rdp2, &client_addr_rdp2_len);
        if (bytes_received == -1) {
            perror("Error receiving data");
            exit(1);
        }

		printf("Received from RDP2: %s\n", buffer_rdp2);

		// Forward to RDP1
        sendto(server_socket_rdp1, buffer_rdp2, 13, 0, (struct sockaddr*)&client_addr_rdp1, client_addr_rdp1_len);


    }

    // Close the socket
    close(server_socket_rdp1);
	close(server_socket_rdp2);
}

int main() {

	link_socket();

	return 0;

}
