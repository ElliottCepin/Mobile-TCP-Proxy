#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct p {
	unsigned int *size;
	char *data; // for now, this is char
	int buffer;
} packet;

int build_packet(int, void*, packet*);
packet *initialize_packet();
void print_packet(packet *);

int main(int argc, char *argv[]){
	if (argc != 2){
		fprintf(stderr, "Invalid command: %d arguments given. Please use the following format: server <server_port>", argc);
		exit(1);
	}

	char *p = argv[1];
	int port = atoi(p);
	char buf[1024];

	struct sockaddr_in srvr;

	// set up address
	bzero((char *)&srvr, sizeof(srvr));
	srvr.sin_family = AF_INET;
	srvr.sin_addr.s_addr = INADDR_ANY;
	srvr.sin_port = htons(port);

	// passive open
	int handle = socket(PF_INET, SOCK_STREAM, 0);
	if (handle <= 0) {
		perror("Phase 1 - server: socket");
		exit(1);
	}

	int b = bind(handle, (struct sockaddr *)&srvr, sizeof(srvr));
	
	if (b < 0){
		perror("Phase 1 - server: bind");
		exit(1);
	}

	
	
	listen(handle, 5);
	int client;
	socklen_t addrlen;
	int buf_len;
	while (1) {
		client = accept(handle, NULL, NULL);
		if (client <= 0){
			perror("Phase 1 - server: accept");	
			exit(1);
		}
		printf("--- New connection received ---\n");
		
		buf_len = 1;
		while (buf_len > 0) {
			packet *p = initialize_packet();
			buf_len = read(client, buf, sizeof(buf));
			while (1 && buf_len > 0){
				int x = build_packet(buf_len, buf, p);
				if (x) {
					break;
				} else {
			}
				buf_len = read(client, buf, sizeof(buf));
			}

			if (buf_len != 0) print_packet(p);	
		}

		printf("--- Connection ended ---\n");
	}


}

void print_packet(packet *p) {
	char *str = malloc(ntohl(*(p->size)) + 1);
	str = memcpy(str, p->data, ntohl(*(p->size)));
	str[ntohl(*(p->size))] = '\0';
	printf("%d\t%s", ntohl(*(p->size)), str);
	fflush(stdout);
}

int build_packet(int buffer, void *data, packet *p){
	if (p->size == NULL && buffer < sizeof(int)) {
			exit(1);
	}	
	
	if (p->size == NULL) {
		p->size = malloc(sizeof(unsigned int));
		p->size = memcpy(p->size, data, sizeof(unsigned int));	
		p->data = malloc(ntohl(*(p->size)));
		buffer -= sizeof(unsigned int);
		data += sizeof(unsigned int);
	} 
	if (buffer != 0) {
		memcpy(p->data + p->buffer, data, buffer);
		p->buffer += buffer;
	}
	return p->buffer == ntohl(*(p->size));
	
}	

packet *initialize_packet() {
	packet *p = malloc(sizeof(packet));
	p->size = NULL; 
	p->buffer = 0;
	p->data = NULL;
	return p;
}

void free_packet(packet *p) {
	free(p->size);
	free(p->data);
	free(p); // the rest is freed by somehting else
}
