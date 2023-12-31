#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define LINK_SPEED 1200 //bit/s
#define MAX_PACKET_SIZE 2049 //max packet size is 2048
#define HEADER_SIZE 4 //bytes (long int syze) 
#define DELAYED_INTERFACE "eth1" 
#define ADD 1
#define DEL 0
#define ACK "|ACK"
#define DELIMITATOR "|"


struct NetworkPacket {
    // header
	int seq;
	// payload
    char* message;
};

typedef struct NetworkPacket Packet;

// This function is responsible for, given a packet size, computing the delay (for links with speed=1200 bit/s)
// WARNING: the packet size is given in bytes so a conversion to bits is needed
unsigned int link_delay_tx(int packet_size) {
	printf("Received packet size: %d\n", packet_size);
	float delay = (packet_size * 8.0)/LINK_SPEED;
	printf("Generated delay %f ms for packet size %d\n", round(delay*1000), packet_size);
	return (unsigned int)round(delay*1000); //conversion to milisseconds
}

// The delay for this function comes in milisseconds
void emulate_delay(unsigned int delay) {

	/*struct timespec ts;
    ts.tv_sec = delay / 1000;
    ts.tv_nsec = (delay % 1000) * 1000000;
    
    nanosleep(&ts, NULL);
    */
    clock_t start = clock();
    clock_t end = start + (delay * CLOCKS_PER_SEC/1000);

    while (clock() < end);
}


int run_command(int delay, int action) {

	FILE *fp;
	char path[1035];

	char* command = "";
	if (action) {
		printf("tc qdisc add dev %s root netem delay %dms\n", DELAYED_INTERFACE, delay, command);
	}
	else {
		printf("tc qdisc del dev %s root netem delay %dms\n", DELAYED_INTERFACE, delay, command);
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


void create_ack_packet(Packet* packet, int seq) {

    packet->seq = seq;

    int message_size = strlen(ACK) +1;
    packet->message = (char *)malloc(message_size * sizeof(char));
    
    memset(packet->message, '\0', message_size);

    strcpy(packet->message, ACK);   

}

void link_socket() {
	int server_socket;
    struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

//  --------------------------------- Create UDP sockets -------------------------------------------------
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(1);
    }

//  --------------------------------- Configure Server Addresses -------------------------------------------------

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); 

//  --------------------------------- Bind Server Sockets -------------------------------------------------


    // Bind the socket to the specific interface address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding server socket");
        exit(1);
    }

//  --------------------------------- Receive from RDP1 and RDP2 -------------------------------------------------

	char buffer[MAX_PACKET_SIZE];
    
    while (1) {
        // Receive data from rdp1
        memset(buffer, '\0', MAX_PACKET_SIZE);
        int bytes_received = recvfrom(server_socket, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (bytes_received == -1) {
            perror("Error receiving data");
            exit(1);
        }

        printf("[LOG] Received packet from client\n");

        // Process delay and call netemu

		unsigned int packet_delay = link_delay_tx(strlen(buffer)); // goes in bytes
        

		emulate_delay(packet_delay);

        // Process the received packet

        char* token;

        token = strtok(buffer, "|");

        int seq_number = atoi(token);

        printf("Seq: %d\n", seq_number);

        /*while (token != NULL) {
            printf("Token: %s\n", token);

            token = strtok(NULL, "|");
        } */ // we don't need to process the message 

        Packet* packet = (Packet*)malloc(sizeof(Packet));

        create_ack_packet(packet, seq_number);

        int packet_size = HEADER_SIZE + strlen(packet->message) + 1;

        char *frame = (char *)malloc(packet_size * sizeof(char));
        memset(frame, '\0', packet_size);
        snprintf(frame, sizeof(int), "%d", packet->seq);
        memcpy(frame + strlen(frame), packet->message, strlen(packet->message));

        unsigned int ack_delay = link_delay_tx(strlen(frame)); // goes in bytes 
        emulate_delay(ack_delay);  

        printf("Sent frame: %s\n", frame);    

		// Forward to RDP1
        sendto(server_socket, frame, 13, 0, (struct sockaddr*)&client_addr, client_addr_len);

		//run_command(delay, DEL);
        free(packet->message);
        free(packet);

    }

    // Close the socket
    close(server_socket);
}

int main() {

	link_socket();

	return 0;

}
