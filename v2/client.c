#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define CLIENT_IP 0
#define CLIENT_PORT 0
#define MIN_PACKET_SIZE 10 //bytes
#define MAX_PACKET_SIZE 256 //bytes

#define DELIMITER '|'

#define HEADER_SIZE 8 //bytes (long int size) 

#define MIN_MESSAGE_SIZE (MIN_PACKET_SIZE - HEADER_SIZE)
#define MAX_MESSAGE_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)

#define SIZE 100

struct NetworkPacket {
    // header
	int seq;
	// payload
    char* message;
};

typedef struct NetworkPacket Packet;

struct PacketTime {
    // header
	int seq;
    long int timestamp;
};

typedef struct PacketTime Time_S;

struct PacketTime* hashArrayTime[SIZE];

int sequencer;

int hashCode(int seq) {
    return seq % SIZE;
}

long int searchTime(int seq) {
    int index = hashCode(seq);
    while(hashArrayTime[index] != NULL) {
        if (hashArrayTime[index]->seq == seq) {
            return hashArrayTime[index]->timestamp;
        }
        index++;
        index %= SIZE;
    }
}

void insert(int seq, int time) {
    Time_S* time_s = (Time_S*)malloc(sizeof(Time_S));
    time_s->seq = seq;
    time_s->timestamp = time;

    int index = hashCode(seq);

    while (hashArrayTime[index] != NULL && hashArrayTime[index]->seq != -1) {
        index++;
        index %= SIZE;
    }

    hashArrayTime[index] = time_s;

}

void delete(int seq) {
    int index = hashCode(seq);

    while (hashArrayTime[index] != NULL)    {
        if (hashArrayTime[index]->seq == seq) {
            free(hashArrayTime[index]);
        }
        index++;
        index %= SIZE;
    }
}


int generateRandomPacketSize() {
    srand(time(NULL));
    int num = rand();
    int length = (num % MAX_MESSAGE_SIZE);
    printf("Length: %d\n", length);
    if (length < MIN_MESSAGE_SIZE) {
        length += MIN_MESSAGE_SIZE;
    }

    return length;
}

// Function to generate a random string of variable length
char* generatePacketContent(int size) {    

    char *random_string = (char *)malloc(size + 1);  // +1 for the null terminator
    if (random_string == NULL) {
        perror("packet Content Generator: Memory allocation error");
        exit(1);
    }

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charset_length = sizeof(charset) - 1;

    random_string[0] = DELIMITER;

    for (int i = 1; i < size; i++) {
        random_string[i] = charset[rand() % charset_length];
    }

    random_string[size] = '\0';  // Null-terminate the string
    return random_string;
}

void createPacket(Packet* packet_S){
	
	packet_S->seq = sequencer;
	sequencer++;

    int message_size = generateRandomPacketSize();
    char *message = generatePacketContent(message_size);

	packet_S->message = (char *)malloc((strlen(message) + 1) * sizeof(char));
    if (packet_S->message == NULL) {
        perror("CreatePacket: Memory allocation error");
        exit(1);
    }

	memset(packet_S->message, '\0', strlen(message) + 1);
	strcpy(packet_S->message, message);
}

int main() {

    int client_socket;
    struct sockaddr_in server_addr;
    struct timespec t_send;
    struct timespec t_recv;

    sequencer = 0;


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


        Packet* packet = (Packet*)malloc(sizeof(Packet));
        createPacket(packet);

        int buffer_size = HEADER_SIZE + strlen(packet->message) + 1;
        char *frame = (char *)malloc(buffer_size * sizeof(char));
        memset(frame, '\0', buffer_size);
        snprintf(frame, sizeof(long int), "%d", packet->seq);
        memcpy(frame + strlen(frame), packet->message, strlen(packet->message));

        insert(packet->seq, time(NULL));

        printf("Message sent to server: %s\n", frame);

        // Send data to the server
        sendto(client_socket, frame, strlen(frame), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        free(packet->message);
        free(packet);
        free(frame);

        // Receive a response from the server (optional)
        char buffer[MAX_PACKET_SIZE];
        memset(buffer, '\0', MAX_PACKET_SIZE);
        int bytes_received = recvfrom(client_socket, buffer, MAX_PACKET_SIZE, 0, NULL, NULL);
        if (bytes_received == -1) {
            perror("Error receiving data");
            exit(1);
        }

        printf("Received from server: %s\n", buffer);

                

        //double diff = ((t_recv.tv_sec - t_send.tv_sec)*1000) + ((t_recv.tv_nsec - t_send.tv_nsec) / 1000000.0);
        //printf("Tx: %.5lf miliseconds\n", diff);

        //sleep(3);

    }

    // Close the socket
    close(client_socket);

    return 0;
}