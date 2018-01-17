#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "server.h"
#include "compress.c"

uint32_t total_bytes_recieved;
uint32_t total_bytes_sent;
uint32_t total_bytes_before_compression;
uint32_t total_bytes_after_compression;


int main(int argc, char ** argv){
	char * port = argv[1];

	int server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	int optval = 1;
	setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	getaddrinfo(NULL, port, &hints, &result);

	bind(server_sock_fd, result->ai_addr, result->ai_addrlen);

	listen(server_sock_fd, SOMAXCONN);

	freeaddrinfo(result);

	while(1){
//		printf("Waiting for connection!\n");
		int client_fd = accept(server_sock_fd, NULL, NULL);
		process_client(client_fd);
	}
	return 0;
}


header_info parse_header(int client_fd){

	header_info header;
	ssize_t bytes_read = read_all(client_fd, (char*)&header, 8);
	header.magic_number = ntohl(header.magic_number);
	header.payload_length = ntohs(header.payload_length);
	header.req_code = ntohs(header.req_code);
	return header;
}

void * compose_header(uint16_t payload_length, uint16_t status){
	
	char * ret = malloc(8);
	uint32_t mag = htonl(MAGICNUMBER);
	payload_length = htons(payload_length);
	status = htons(status);
	memcpy(ret, &mag, 4);
	memcpy(ret+4, &payload_length, 2);
	memcpy(ret+6, &status, 2);
	return (void*)ret;
}

void send_error(int client_fd, int error){
	
	void * response_header = compose_header(0, error);
	write(client_fd, response_header, 8);
	total_bytes_sent += 8;
	free(response_header);
	return;
}

int handle_ping(int client_fd, header_info header){

	if (header.payload_length != 0){
//		printf("bad length\n");
		send_error(client_fd, 33);
		return -1;
	}
	void * response_header = compose_header(0, 0);
	write(client_fd, response_header, 8);
	total_bytes_sent += 8;
	free(response_header);
	return 0;
}

int handle_get_stats(int client_fd, header_info header){
	
	if (header.payload_length != 0){
//		printf("bad length\n");
		send_error(client_fd, 33);
		return -1;
	}
	void * response_header = compose_header(9, 0);
	char * response = malloc(9);

	uint32_t tbr = htonl(total_bytes_recieved);
	uint32_t tbs = htonl(total_bytes_sent);
	double compression_ratio = (double)total_bytes_after_compression/total_bytes_before_compression*100;
	uint8_t cr = (int)(compression_ratio+0.5);
	memcpy(response, &tbr, 4);
	memcpy(response+4, &tbs, 4);
	memcpy(response+8, &cr, 1);

	write(client_fd, response_header, 8);
	write(client_fd, response, 9);
	total_bytes_sent += 17;

	free(response_header);
	free(response);
	return 0;
}

int handle_reset_stats(int client_fd, header_info header){
	
	total_bytes_recieved = 0;
	total_bytes_sent = 0;
	total_bytes_before_compression = 0;
	total_bytes_after_compression = 0;
	void * response_header = compose_header(0, 0);
	write(client_fd, response_header, 8);
	free(response_header);
	return 0;
}

int handle_compress(int client_fd, header_info header){

	if (header.payload_length == 0){
//		printf("Can't compress payload of length 0\n");
		send_error(client_fd, 36);
		return -1;
	}
	if (header.payload_length > MAX_MESSAGE_SIZE){
//		printf("Message too large\n");
		send_error(client_fd, 2);
		return -1;
	}
	char * buf = calloc(header.payload_length+1, 1);
	ssize_t bytes_read = read_all(client_fd, buf, header.payload_length);
	total_bytes_recieved += header.payload_length;
	total_bytes_before_compression += header.payload_length;

	int compressed_size = compress(buf);
	if (compressed_size < 0){
//		printf("Invalid compression string");
		send_error(client_fd, 35);
		return -1;
	}
	void * response_header = compose_header(compressed_size, 0);
	write(client_fd, response_header, 8);
	write(client_fd, buf, compressed_size);
	total_bytes_sent += 8 + compressed_size;
	total_bytes_after_compression += compressed_size;
	free(buf);
	free(response_header);
	return 0;
}

int process_client(int client_fd){
	
	header_info header = parse_header(client_fd);
//	printf("magic_number: %d   payload_length: %d   req_code: %d\n", header.magic_number, header.payload_length, header.req_code);
	if (header.magic_number != MAGICNUMBER){
//		printf("bad magic\n");
		send_error(client_fd, 34);
		shutdown(client_fd, SHUT_RDWR);
		close(client_fd);
		return -1;
	}

	if (header.req_code == PING) handle_ping(client_fd, header);
	else if (header.req_code == GETSTATS) handle_get_stats(client_fd, header);
	else if (header.req_code == RESETSTATS) handle_reset_stats(client_fd, header);
	else if (header.req_code == COMPRESS) handle_compress(client_fd, header);
	else{
//		printf("bad request\n");
		send_error(client_fd, 3);
		shutdown(client_fd, SHUT_RDWR);
		close(client_fd);
		return -1;
	}
	if (header.req_code != RESETSTATS) total_bytes_recieved += 8;
	shutdown(client_fd, SHUT_RDWR);
	close(client_fd);
	return 0;
}

ssize_t read_all(int socket, char * buf, size_t count){
	size_t bytes_read = 0;
	while(bytes_read < count){
		int res = read(socket, buf, count-bytes_read);
		if (errno == EBADF) return 0;
		if (res == -1){
			if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
		}
		if (res == -1 && errno == EINTR) continue;
		if (res == -1 && errno != EINTR) return -1;
		if (res == 0) return bytes_read;
		if (res == (int)count) return res;
		bytes_read += res;
		buf += res;
	}
	return count;
}

