#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

int IGNORE_ERROR = 0;
int PRINT_ERROR = 1;
int PANIC_ERROR = 2;

typedef struct p {
	unsigned int *size;
	void *data; 
	int buffer;
} packet;

int build_packet(int, void*, packet*);
packet *initialize_packet();
void print_packet(packet *);
void free_packet(packet *);
int send_packet(int, packet *, int);
int main(int argc, char *argv[]){
	if (argc != 4){
		fprintf(stderr, "Invalid command: %d arguments given.\nPlease use the following format:\n\tcproxy <listening_port> <sproxy_ip> <sproxy_port>\n", argc);
		exit(1);
	}
	
	/* telnet connection */
	char *p = argv[1];
	int port = atoi(p);
	char buf[1024];
	
	struct sockaddr_in tel;

	// set up address
	bzero((char *)&tel, sizeof(tel));
	tel.sin_family = AF_INET;
	tel.sin_addr.s_addr = INADDR_ANY;
	tel.sin_port = htons(port);

	// passive open
	int handle = socket(PF_INET, SOCK_STREAM, 0);
	if (handle <= 0) {
		perror("Phase 1 - server: socket");
		exit(1);
	}

	int b = bind(handle, (struct sockaddr *)&tel, sizeof(tel));
	
	if (b < 0){
		perror("Phase 1 - server: bind");
		exit(1);
	}

	listen(handle, 5);

	/* sproxy connection */	
	char *host = argv[2];
	p = argv[3];
	port = atoi(p);
	struct sockaddr_in srvr;

	// clear server address
	bzero(&srvr, sizeof(srvr));

	// assign values to address structure
	srvr.sin_family = AF_INET;
	srvr.sin_addr.s_addr = inet_addr(host);
	srvr.sin_port = htons(port);

	int server_handle = socket(PF_INET, SOCK_STREAM, AF_UNSPEC);
	
	// checks if socket could open
	if (server_handle < 0) {
		perror("Phase 1: socket"); // Does this mean "socket couldn't open">
		exit(1);
	}

	int ok = connect(server_handle, (struct sockaddr *)&srvr, sizeof(srvr));

	// checks if connection worked
	if (ok < 0) {
		perror("Phase 1: connect");
		exit(1);
	}
			
	
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
		int bytes_recieved = 0; 
		
		while (buf_len > 0) {
			packet *p = initialize_packet();
			buf_len = read(client, buf, sizeof(buf));
			bytes_recieved += buf_len;
			printf("bytes recieved thusly\t%d\n", bytes_recieved);
			/* Experiment - remove the second while loop */
			// while (buf_len > 0){
				// build packet returns p->buffer == p->size
				// p->buffer == p->size iff the packet is fully built.
				int x = build_packet(buf_len, buf, p);
				/* this is only necessary with the second while loop.
				// if the packet is fully built, 
				if (x) {
					// send the packet, and wait for the next packet
					break;
				}
				
				// otherwise, keep building the packet.
				buf_len = read(client, buf, sizeof(buf));
				bytes_recieved += buf_len;
				printf("bytes recieved thusly\t%d\n", bytes_recieved);
				*/
			//}			
			// use packet, free packet
			// set to IGNORE_ERROR, because we are sending partial packets on purpose
			int ok = send_packet(server_handle, p, IGNORE_ERROR);
			
			if (ok == -1) {
				perror("packet head send fail");
				exit(1); // I'm not sure if we want to just give up here...
			}
			
			if (ok == -2) {
				perror("packet body send fail");
				exit(1); // I'm not sure if we want to just give up here...
			}
		}
		
		printf("--- Connection ended ---\n");
	}


}

// builds on an initialized backet
int build_packet(int buffer, void *data, packet *p){
	if (p->size == NULL && buffer < sizeof(int)) {
		// we have to do something about this awfully small buffer.
		p->size = malloc(sizeof(unsigned int));
		if (buffer > 0) {
			p->size = memcpy(p->size, data, buffer);	
		} else {
			*(p->size) = 0;
		}
		p->data = malloc(ntohl(*(p->size)));
		buffer -= buffer;
		data += sizeof(unsigned int);
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

// on_err can be set to three values: 0, 1, and 2 (ignore errors, print errors, and panic).
// printerrors ignores the error, but prints a warning
// any other on_err value will be treated as 0
// returns -1 on first write fail
// returns -2 on second write fail
// POSTCONDITION: packet is freed.
int send_packet(int handle, packet *p, int on_err) {
	if (p->buffer != ntohl(*(p->size))) {
		if (on_err == PANIC_ERROR) {
			fprintf(stderr, "Tried to send an incomplete packet");
			exit(1);
		} else if (on_err == PRINT_ERROR){
			fprintf(stderr, "Warning: sent an incomplete packet");
		}
	}

	// We have to use buffer instead of size, because an error could have occured
	void *data = malloc(p->buffer); //  
	data = memcpy(data, p->data, p->buffer);
	
	int ok = write(handle, &(p->buffer), sizeof(p->buffer)); 	
	if (ok < 0) {
		return -1;
	}

	ok = write(handle, data, p->buffer);
	if (ok < 0) {
		return -2;
	}	

	free_packet(p);
	return 0; 
}	

// initializes a packet
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
