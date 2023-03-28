#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <limits.h>

#define BUFFER_SIZE 1000

int main(int argc, char *argv[]) {
    // check arguments
    if (argc != 3) {
        fprintf(stderr, "Arguments: %s <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }

    // convert port from str to int
    int server_port = atoi(argv[2]);

    // create socket
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket err. Check Out!");
        exit(1);
    }

    // set server address
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(server_port);

    char buffer[BUFFER_SIZE];

    int pckt_send = 0;
    int pckt_recv = 0;
    long long rtt_max = 0;
    long long rtt_min = LLONG_MAX;
    long long rtt_sum = 0;
    double rtt_avg = 0.0;
    
    //time from first pckt
    struct timespec ini_time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ini_time);    

    // send packets and receive responses
    for (int i = 0; i < 100; i++) {
        // 100 milissegundos waiting
    	struct timespec ts = {0, 100000000}; 
	nanosleep(&ts, NULL);

	//time before send
	struct timespec send_time = {0, 0};
	clock_gettime(CLOCK_MONOTONIC_RAW, &send_time);
	
        // send packet
        int n = snprintf(buffer, BUFFER_SIZE, "%d", i);
        if (sendto(sock_fd, buffer, n, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("sendto");
            continue;
        }
        pckt_send++;

        // receive response
        struct sockaddr_in server_resp_addr = {0};
        socklen_t server_resp_addr_len = sizeof(server_resp_addr);
        n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_resp_addr, &server_resp_addr_len);
        if (n == -1) {
            perror("recvfrom err");
            continue;
        }
        pckt_recv++;
        struct timespec recv_time = {0, 0};
	clock_gettime(CLOCK_MONOTONIC_RAW, &recv_time);
	
	// calc RTT in microssec
	long long rtt = (recv_time.tv_sec - send_time.tv_sec) * 1000000LL + (recv_time.tv_nsec - send_time.tv_nsec) / 1000LL;

	if (rtt > rtt_max) {
        	rtt_max = rtt;
    	}

    	if (rtt < rtt_min) {
        	rtt_min = rtt;
    	}

    	rtt_sum += rtt;

    }
    
    
    if (pckt_recv > 0) {
        rtt_avg = (double)rtt_sum / pckt_recv;
    }
    
    struct timespec end_time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    
    // calc Delta in microssec
	long long delta = (end_time.tv_sec - ini_time.tv_sec) * 1000000LL + (end_time.tv_nsec - ini_time.tv_nsec) / 1000LL;
	
    printf("Número de pacotes enviados: %d\nRecebidos: %d\nRound trip time mínimo: %lld ms\nRound trip time medio: %.2lf ms\nRound trip time máximo: %lld ms\nDelta entre o primeiro pacote enviado e o ultimo enviado: %lld ms\n", pckt_send, pckt_recv, rtt_min, rtt_avg, rtt_max, delta);

    close(sock_fd);

    return 0;
}

