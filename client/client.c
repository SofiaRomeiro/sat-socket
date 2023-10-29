#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define CLIENT_IP 0
#define CLIENT_PORT 0
#define MIN_PACKET_SIZE 10 //bytes
#define MAX_PACKET_SIZE 256 //bytes

#define DELIMITER '|'

#define HEADER_SIZE 4 //bytes (int size) 

#define MIN_MESSAGE_SIZE 6
#define MAX_MESSAGE_SIZE 252

#define SIZE 100 //number of packets that can be in queue

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

struct timeval time_tx;
struct timeval time_rcv;

int sequencer;

/********************************************
*            HASH TABLE FUNCTIONS           *
********************************************/

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
    printf("[ERROR] Time not found in search!\n");
    return -1;
}

void insert(int seq, long int time) {
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

    while (hashArrayTime[index] != NULL) { 
        if (hashArrayTime[index]->seq == seq) {
            free(hashArrayTime[index]);
            hashArrayTime[index] = NULL;
            return;
        }
        index++;
        index %= SIZE;
    }
}

/********************************************
*          TIMEOUT AND RTT FUNCTIONS        *
********************************************/

// Source: www.educative.io

float dev_rtt;
float est_rtt;
float avg_rtt;

/*
This function receives:
    - the sample_rtt, which is the value for the newly received packet
*/
void compute_estRTT(float sample_rtt){
    float alpha = 0.125;
    est_rtt = ((1 - alpha) * avg_rtt) + (alpha * sample_rtt);
}

void compute_devRTT(float sample_rtt){
    float beta = 0.125;
    dev_rtt = ((1 - beta) * dev_rtt) + (beta * fabs(sample_rtt - est_rtt));
}

int compute_timeout_interval(){
    float timeout = (4 * dev_rtt) + est_rtt;
    return timeout;
}

int handle_rtt(float rtt) {
    if (sequencer < 1) {
        avg_rtt = rtt;
    }
    float sum = (avg_rtt * (sequencer-1)) + rtt;
    avg_rtt = sum / sequencer;
    compute_estRTT(rtt);
    compute_devRTT(rtt);
    int timeout = compute_timeout_interval();
    return timeout;
}

/********************************************
*               TIME FUNCTIONS              *
********************************************/

// returns ms
long int timecompare(long int start, long int end) {
    long int diff = end - start;
    while (diff < 0) {
        diff += 1000000;
    }
    return round(diff/1000);
}

/********************************************
*          PACKET RELATED FUNCTIONS         *
********************************************/

int generateRandomPacketSize() {
    srand(time(NULL));
    int num = rand();
    int length = (num % MAX_MESSAGE_SIZE);
    if (length < MIN_MESSAGE_SIZE) {
        length += MIN_MESSAGE_SIZE;
    }

    return length;
}

// Function to generate a random string of variable length
char* generatePacketContent(int size) {    

    char *random_string = (char *)malloc(size);
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
        perror("CreatePacket: Memory allocation error\n");
        exit(1);
    }

	memset(packet_S->message, '\0', strlen(message) + 1);
	strcpy(packet_S->message, message);
}

/********************************************
*                   MAIN                    *
********************************************/

int main() {

    int client_socket;
    struct sockaddr_in server_addr;
    struct timespec t_send;
    struct timespec t_recv;

    sequencer = 0;


//  --------------------------------- Create UDP socket -------------------------------------------------
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("Error creating client socket\n");
        exit(1);
    }

//  --------------------------------- Configure Server Address -------------------------------------------------
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

// -------------------------- Generate the packet to send --------------------------

    Packet* packet = (Packet*)malloc(sizeof(Packet));

    while(1) {

        printf("\n***** NEW PACKET ******\n");

        createPacket(packet);

        char frame[MAX_PACKET_SIZE];

        memset(frame, '\0', MAX_PACKET_SIZE);
        snprintf(frame, sizeof(long int), "%d", packet->seq);
        memcpy(frame + strlen(frame), packet->message, strlen(packet->message));

        gettimeofday(&time_tx, NULL);

        printf("[LOG] Message sent to server\n");

        // Send data to the server
        sendto(client_socket, frame, strlen(frame), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // avoid delaying due to processing

        insert(packet->seq, time_tx.tv_usec);

        free(packet->message);
        //printf("Free packet message OK\n");

        packet->seq = -1;     

        // Receive a response from the server (optional)
        char buffer[MAX_PACKET_SIZE];
        memset(buffer, '\0', MAX_PACKET_SIZE);
        int bytes_received = recvfrom(client_socket, buffer, MAX_PACKET_SIZE, 0, NULL, NULL);
        if (bytes_received == -1) {
            perror("Error receiving data\n");
            exit(1);
        }

        gettimeofday(&time_rcv, NULL);

        char* token;
        token = strtok(buffer, "|");
        int seq_number = atoi(token);

        printf("Seq: %d\n", seq_number);

        long int time_tx_i = searchTime(seq_number);
        delete(seq_number);

        float tmp_rtt = timecompare(time_tx_i, time_rcv.tv_usec);

        //printf("[LOG] Received from server at %ld and sent at %ld\n", time_rcv.tv_usec , time_tx_i);
        printf("[LOG] The current RTT is %f ms\n", tmp_rtt);

        float tOut = handle_rtt(tmp_rtt);

        printf("[LOG] The devRTT is %f ms\n", dev_rtt);
        printf("[LOG] The estRTT is %f ms\n", est_rtt);
        printf("[LOG] The avgRTT is %f ms\n", avg_rtt);
        printf("[LOG] The timeout is %f ms\n", tOut);


    }

    free(packet);
    // Close the socket
    close(client_socket);

    return 0;
}