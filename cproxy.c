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

int build_packet(int, void*, packet*, int);
packet *initialize_packet();
void print_packet(packet *);
void free_packet(packet *);
int send_packet(int, packet *, int);
int new_connection(in_addr_t, int);
int new_connection_s(char *, int);
int proxy_send(int, int, char*, int);
int send_raw(int, packet *, int);

const int LISTEN_SOCKET = 0;
const int BUF_LEN = 1024;
const int IS_CPROXY = 1;
const int IS_SPROXY = !IS_CPROXY;
int main(int argc, char *argv[]){
	if (argc != 4){
		fprintf(stderr, "Invalid command: %d arguments given.\nPlease use the following format:\n\tcproxy <listening_port> <sproxy_ip> <sproxy_port>\n", argc);
		exit(1);
	}
	
	struct pollfd polling[BUF_LEN];
	int bitmap[BUF_LEN];
	int count = 0;

	/* telnet listener*/
	char *p = argv[1];
	int port = atoi(p);
	char buf[BUF_LEN];
	
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

	polling[LISTEN_SOCKET].fd = handle;
	polling[LISTEN_SOCKET].events = POLLIN;
	count++;

	/* sproxy connection */	
	char *host = argv[2];
	p = argv[3];
	port = atoi(p);
	
	
	int client;
	socklen_t addrlen;
	
	while (1) {
		poll(polling, count, -1);
		int count_updater = 0; // this allows me to delay the update of count until out of loop; no polling has happened, no reason to check the last item.
		for (int i=0; i<count; i++) {
			if (i == LISTEN_SOCKET) {
				if (polling[LISTEN_SOCKET].revents != 0) {
					printf("New socket");fflush(stdout);
					count_updater = 2;					
					client = accept(polling[LISTEN_SOCKET].fd, NULL, NULL);							
					
					if (client <= 0){
						perror("Phase 1 - server: accept");	
						exit(1);
					}
					printf("--- New connection received ---\n");
					

					polling[count].fd = new_connection_s(host, port);
					polling[count].events = POLLIN;
					bitmap[count] = 0;
					polling[count+1].fd = client;
					polling[count+1].events = POLLIN;
					bitmap[count+1] == 0;
					
				}
			} else {
				if (polling[i+1].revents != 0) {
					printf("cproxy --> sproxy || sproxy --> telnet daemon");fflush(stdout);
					int shut = proxy_send(polling[i+1].fd, polling[i].fd, buf, IS_CPROXY);
					if (shut == 1) {
						shutdown(polling[i+1].fd, SHUT_WR);
						bitmap[i+1] = 1;
						if (bitmap[i]) {
							close(polling[i+1].fd);
							close(polling[i].fd);
							polling[i+1].fd = -1;
							polling[i].fd = -1;
						}
					}
				}	
				if (polling[i].revents != 0) {
					printf("cproxy --> telnet || sproxy --> cproxy"); fflush(stdout);
					int shut = proxy_send(polling[i].fd, polling[i+1].fd, buf, IS_SPROXY);	fflush(stdout);
					if (shut == 1) {
						shutdown(polling[i].fd, SHUT_WR); 
						bitmap[i] = 1;
						if (bitmap[i+1]) {
							close(polling[i+1].fd);
							close(polling[i].fd);
							polling[i].fd = -1;
							polling[i+1].fd = -1;
						}
					}
				}	
				i++;
			}
		}	
		count += count_updater;
	}


}

// it is ok to name this send, because of method signatures, right? <-- no, it is not
// also, I don't use the send syscall (or is it a wrapper for write? I thought it was a unique syscall)
int proxy_send(int client, int server_handle, char *buf, int raw) {
	int buf_len = 1;
	int bytes_recieved = 0; 
		packet *p = initialize_packet();
		buf_len = read(client, buf, BUF_LEN);
		bytes_recieved += buf_len;
		printf("bytes recieved thusly\t%d\n", bytes_recieved);
		if (buf_len == 0) {
			return 1;
		}
		/* Experiment - remove the second while loop */
		// while (buf_len > 0){
			// build packet returns p->buffer == p->size
			// p->buffer == p->size iff the packet is fully built.
			int x = build_packet(buf_len, buf, p, raw);
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
		int ok;
		if (!raw) ok = send_raw(server_handle, p, IGNORE_ERROR);
		else ok = send_packet(server_handle, p, IGNORE_ERROR);
		
		if (ok == -1) {
			perror("packet head send fail");
			exit(1); // I'm not sure if we want to just give up here...
		}
		
		if (ok == -2) {
			perror("packet body send fail");
			exit(1); // I'm not sure if we want to just give up here...
		}
	return 0;
}
// this gets run when a new telnet connects to cproxy
// host, p, and port will always be the same. 
int new_connection(in_addr_t host, int port) {
	/* sproxy connection */	
	struct sockaddr_in srvr;
	int server_handle = socket(PF_INET, SOCK_STREAM, AF_UNSPEC);

	// clear server address
	bzero(&srvr, sizeof(srvr));

	// assign values to address structure
	srvr.sin_family = AF_INET;
	srvr.sin_addr.s_addr = host;
	srvr.sin_port = htons(port);

	
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

	return server_handle;

}

int new_connection_s(char *host, int port) {
	return new_connection(inet_addr(host), port);
}


// builds on an initialized backet
int build_packet(int buffer, void *data, packet *p, int raw){
	if (p->size == NULL && buffer < sizeof(int) && !raw) {
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
	if (p->size == NULL && !raw) {
		p->size = malloc(sizeof(unsigned int));
		p->size = memcpy(p->size, data, sizeof(unsigned int));	
		p->data = malloc(ntohl(*(p->size)));
		buffer -= sizeof(unsigned int);
		data += sizeof(unsigned int);
	} 
	if (p->data == NULL && raw) {
		p->data = malloc(buffer);
	} 
	if (buffer != 0) {
		memcpy(p->data + p->buffer, data, buffer);
		p->buffer += buffer;
	}
	return raw;
	
}	

// on_err can be set to three values: 0, 1, and 2 (ignore errors, print errors, and panic).
// printerrors ignores the error, but prints a warning
// any other on_err value will be treated as 0
// returns -1 on first write fail
// returns -2 on second write fail
// POSTCONDITION: packet is freed.
int send_packet(int handle, packet *p, int on_err) {
	if (p->buffer != 1) {
		if (on_err == PANIC_ERROR) {
			fprintf(stderr, "Tried to send an incomplete packet");
			exit(1);
		} else if (on_err == PRINT_ERROR){
			fprintf(stderr, "Warning: sent an incomplete packet");
		}
	}

	// We have to use buffer instead of size, because an error could have occured
	void *data = malloc(p->buffer + sizeof(p->buffer)); //  
	void *b = data;
	data += sizeof(p->buffer);
	b = memcpy(b, &(p->buffer), sizeof(p->buffer));
	data = memcpy(data, p->data, p->buffer);
		

	int ok = write(handle, b, p->buffer + sizeof(p->buffer));
	if (ok < 0) {
		return -2;
	}	

	free_packet(p);
	return 0; 
}	


int send_raw(int handle, packet *p, int on_err) {
	if (p->buffer != 1) {
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
	
	// int ok = write(handle, &(p->buffer), sizeof(p->buffer)); 	
	// if (ok < 0) {
	// 	return -1;
	// }

	int ok = write(handle, data, p->buffer);
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
