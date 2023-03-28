#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>

#define BUFFER_SIZE 1000

double jitter_min = -1;
double jitter_max = -1;
double jitter_sum = 0;
int num_jitters = 0;
int num_packets = 0;

struct timeval final_time = {0, 0};

void interrupt_handler(int sig) {
    //when CTRL+C
    printf("CTRL+C\n");
    if (num_jitters > 0) {
    	double jitter_avg = jitter_sum / num_jitters;
    	printf("Número de pacotes recebidos: %d\n", num_packets);  
    	printf("Jitter min: %.2f ms\n", jitter_min);
    	printf("Jitter max: %.2f ms\n", jitter_max);
    	printf("Jitter avg: %.2f ms\n", jitter_avg);
    	printf("Delta entre o primeiro pacote e o ultimo.: %ld ms\n", final_time.tv_usec);     	
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    //only port as argument
    if (argc != 2) {
        fprintf(stderr, "Arguments: %s <port>\n", argv[0]);
        exit(1);
    }
    
    //call interrupt_handler
    signal(SIGINT, interrupt_handler);
    
    //port from str to int
    int port = atoi(argv[1]);

    // create socket
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket err. Check Out!");
        exit(1);
    }

    // bind to port
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind err. Check Out!");
        exit(1);
    }

    //if no problem until now, server is up!
    printf("Server started listening on port %d...\n", port);

    char buffer[BUFFER_SIZE];

    struct timeval prev_time = {0, 0};

    while (1) {
        // receive packet
        struct sockaddr_in client_addr = {0};
        socklen_t client_addr_len = sizeof(client_addr);
        int n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (n == -1) {
            perror("recvfrom err");
            continue;
        }
        num_packets++;

        // calculate jitter
        struct timeval curr_time;
        gettimeofday(&curr_time, NULL);
        long long curr_time_ms = curr_time.tv_sec * 1000LL + curr_time.tv_usec / 1000;
        long long prev_time_ms = prev_time.tv_sec * 1000LL + prev_time.tv_usec / 1000;
        long long diff_time_ms = curr_time_ms - prev_time_ms;
        prev_time = curr_time;
        final_time = prev_time;

        if (jitter_min == -1 || diff_time_ms < jitter_min) {
        	jitter_min = diff_time_ms;
    	}

    	if (jitter_max == -1 || diff_time_ms > jitter_max) {
        	jitter_max = diff_time_ms;
    	}

    	jitter_sum += diff_time_ms;
    	num_jitters++;

        // send response packet
        if (sendto(sock_fd, buffer, n, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
            perror("sendto");
            continue;
        }
    }
    
    

    close(sock_fd);

    return 0;
}

