#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#define DCAT_MAX_INPUT_LENGTH 1000
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

static int conn_fd;

void * recv_thread_func(void * sockfd);

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Incorrect argument count\n");
		printf("Usage: dcat <port>\n");
		return 1;
	}

	const char *port = argv[1];

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &hints, &result) != 0) {
		fprintf(stderr, "Error: getaddrinfo failed\n");
		return 1;
	}

	int sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sockfd == -1) {
		fprintf(stderr, "Error: Failed to create socket\n");
		return 1;
	}

	if (bind(sockfd, result->ai_addr, result->ai_addrlen) != 0) {
		fprintf(stderr, "Error: Failed to bind to port %s\n", port);
		return 1;
	}

	if (listen(sockfd, 1) != 0) {
		fprintf(stderr, "Error: Failed to start listening on port %s\n", port);
		return 1;
	}

	struct sockaddr_in *sock_in = (struct sockaddr_in *)result;
	printf(GRN "[dcat]" RESET " Listening at %s on port %s\n", inet_ntoa(sock_in->sin_addr), port);

	char *input = malloc(DCAT_MAX_INPUT_LENGTH);

	pthread_t recv_thread;
	int error = pthread_create(&recv_thread, NULL, recv_thread_func, &sockfd);

	if (error != 0) {
		fprintf(stderr, "Error: %s\n", strerror(error));
		return 1;
	}

	pthread_detach(recv_thread);

	int main_thread_continue = 1;
	while (main_thread_continue) {
		memset(input, 0, DCAT_MAX_INPUT_LENGTH);
		fgets(input, DCAT_MAX_INPUT_LENGTH, stdin);

		if (strcmp(input, ".exit\n") == 0) {
			main_thread_continue = 0;
		} else {
			send(conn_fd, input, DCAT_MAX_INPUT_LENGTH, 0);
		}
	}
	printf(GRN "[dcat]" RESET " Disconnected\n");

	free(input);

	close(conn_fd);
	close(sockfd);

	return 0;
}

void * recv_thread_func(void * sockfd) {
	char data[DCAT_MAX_INPUT_LENGTH];

	struct sockaddr_storage addr;
	socklen_t addr_size = sizeof(addr);
	conn_fd = accept(*(int *)sockfd, (struct sockaddr *)&addr, &addr_size);

	if (conn_fd == -1) {
		fprintf(stderr, "Error: Failed to accept socket connection\n");
		pthread_exit(NULL);
	}

	struct sockaddr_in *addr_in = (struct sockaddr_in *)&addr;
	printf(GRN "[dcat]" RESET " Connection from %s\n", inet_ntoa(addr_in->sin_addr));

	while (1) {
		memset(data, 0, DCAT_MAX_INPUT_LENGTH);
		recv(conn_fd, data, DCAT_MAX_INPUT_LENGTH, 0);
		printf("%s", data);
		fflush(stdout);
	}
}
