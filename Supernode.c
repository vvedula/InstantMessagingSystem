#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define PHASE2_STATIC_PORT "22325"
#define PHASE3_STATIC_PORT "3325"
#define User1_PORT_NUM 3425
#define User2_PORT_NUM 3525
#define User3_PORT_NUM 3625

#define LINE_MAX 120

typedef struct username_ipaddr {
	char username[100];
	char ipaddr[100];
}u_ip;

typedef struct messages{
	char from_user[20];
	char to_user[20];
	char message[100];
}msg;

u_ip list_of_u_ip[20];// list of username and IP Address.
msg  list_of_msgs[20];//list of messages received.

void Phase2();
void Phase3();

/*
 * Call Phase2 first then call Phase3.
 */ 
int
main(int argc, char *argv[]) {
	
	int i = 0;
	
	Phase2();
	
	Phase3();
	
	return 0;
}

/*
 * find the length of the buffer and return the value
 */
int
size(char *buf) {
	int length = 0, i = 0;

	while(buf[i] != '\0') {
		length++;
		i++;
	}

	length++;

	return length;
}

/*
 * open a TCP socket and listen for incoming connection.
 * read the messages from Login server and store them
 * for using it in  Phase3.
 */
void
Phase2() {

	int sock_num = -1, new_sock = -1, status = 0, count = 0;
	int i, num_bytes = 0;
	struct addrinfo hints, *servinfo;
	struct sockaddr client_addr;
	char msg[100], ip[32], username[100], ip_addr[100], temp_buffer[100];
	
	memset(username, 0, 100);
	memset(ip_addr, 0, 100);
	memset(temp_buffer, 0, LINE_MAX);
	memset(msg, 0, 100);
	
	sock_num = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo("nunki.usc.edu", PHASE2_STATIC_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	inet_ntop(AF_INET, &(((struct sockaddr_in *)servinfo->ai_addr)->sin_addr), ip, 32);

	printf("Phase 2: SuperNode has TCP port %s and IP address: %s\n", PHASE2_STATIC_PORT, ip);
	
	if (bind(sock_num, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		perror("bind\n");
		exit(1);
	}
	
	if (listen(sock_num, 5) == -1) {
		perror("listen\n");
		exit(1);
	}
	
	status = sizeof(client_addr);
	
	new_sock = accept(sock_num, &client_addr, (socklen_t *)&status);
	
	if (new_sock == -1) {
		perror("accept");
		exit(1);
	}

	while (count != 3) {	
		if (recv(new_sock, msg, 100, 0) == -1) {
			perror("recv");
		}
		
		sscanf(msg, "%[^/]", temp_buffer);
		num_bytes = size(temp_buffer);
		strncpy(username, temp_buffer, strlen(temp_buffer));
		
		memset(temp_buffer, 0, 100);
		
		for (i = 0; msg[num_bytes+i] != '\0'; i++) {
			ip_addr[i] = msg[num_bytes+i];
		}
		
		for (i = 0; i < 20; i++) {
			if (list_of_u_ip[i].username[0] == '\0') {
				break;
			}
		}
		
		strncpy(list_of_u_ip[i].username, username, strlen(username));
		strncpy(list_of_u_ip[i].ipaddr, ip_addr, strlen(ip_addr));
		
		memset(msg, 0, 100);
		memset(temp_buffer, 0, 100);
		memset(username, 0, 100);
		memset(ip_addr, 0, 100);
		
		count++;
	}
	
	printf("Phase 2: SuperNode received %d username/IP address pairs.\n", count);
	printf("End of Phase 2 for SuperNode.\n");
	close(new_sock);
	close(sock_num);
}

/*
 * receive messages from all client and send it to appropriate client.
 */
void
Phase3() {

	int sock_num = -1, new_sock = -1, status = 0, count = 0;
	int i, j, k, num_bytes = 0;
	struct addrinfo hints, *servinfo;
	struct sockaddr client_addr;
	struct sockaddr_in dest;
	socklen_t addr_size;
	char msg[100], ip[32], temp[100];

	sock_num = socket(AF_INET, SOCK_DGRAM, 0);
	
	memset(msg, 0, 100);
	memset(temp, 0, 100);
	memset(list_of_msgs, 0, 20*sizeof(msg));
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((status = getaddrinfo("nunki.usc.edu", PHASE3_STATIC_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	inet_ntop(AF_INET, &(((struct sockaddr_in *)servinfo->ai_addr)->sin_addr), ip, 32);
	printf("Phase 3: SuperNode has static UDP port %s IP address %s\n", PHASE3_STATIC_PORT, ip);
	bind(sock_num, servinfo->ai_addr, servinfo->ai_addrlen);

	addr_size = sizeof(dest);
	
	while(count != 6) {
		if (recvfrom(sock_num, msg, 100, 0, (struct sockaddr *)&dest, &addr_size) == -1) {
			printf("recvfrom failure\n");
			exit(1);
		}
		
		printf("Phase 3: SuperNode received the message '%s'\n", msg);
		for (i = 0; i < 20; i++) {
			if(list_of_msgs[i].from_user[0] == '\0') {
				break;
			}
		}

		for (j = 0; msg[j] != '-'; j++) {
			list_of_msgs[i].to_user[j] = msg[j];
		}
		
		for (j = j+1, k = 0; msg[j] != ':'; j++, k++) {
			list_of_msgs[i].from_user[k] = msg[j];
			list_of_msgs[i].message[k] = msg[j];
		}
		
		while(msg[j] != '\0') {
			list_of_msgs[i].message[k] = msg[j];
			j++;
			k++;
		}
		
		count++;
		memset(msg, 0, 100);
	}
	close(sock_num);

	sleep(4);
	
	count = 0;
	
	for (i = 0; i < 20; i++) {
		if (list_of_msgs[i].to_user[0] != '\0') {
			sock_num = socket(AF_INET, SOCK_DGRAM, 0);
			
			memset(msg, 0, 100);
			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			
			memset(&dest, 0, sizeof(struct sockaddr_in));
			for (j = 0; j < 20; j++) {
				if (list_of_u_ip[j].username[0] != '\0') {
					if(strncmp(list_of_u_ip[j].username, list_of_msgs[i].to_user, strlen(list_of_msgs[i].to_user)) == 0) {
						strncpy(msg, list_of_msgs[i].message, strlen(list_of_msgs[i].message));
						break;
					}
				}
			}
			inet_pton(AF_INET, list_of_u_ip[j].ipaddr, &(dest.sin_addr));
			if (strncmp(list_of_u_ip[j].username, "User#1", strlen("User#1")) == 0) {
				dest.sin_port = htons(User1_PORT_NUM);
			} else if (strncmp(list_of_u_ip[j].username, "User#2", strlen("User#2")) == 0) {
				dest.sin_port = htons(User2_PORT_NUM);
			} else {
				dest.sin_port = htons(User3_PORT_NUM);
			}
			dest.sin_family = AF_INET;

			if (sendto(sock_num, msg, 100, 0, (struct sockaddr *)&dest, sizeof(dest)) == -1) {
				printf("sendto failure\n");
				exit(1);
			}
			printf("Phase 3: SuperNode sent the message '%s-%s' on dynamic UDP port number\n", list_of_msgs[i].to_user, list_of_msgs[i].message);
			count++;
			
			close(sock_num);
		}
	}
	
	printf("End of Phase 3 for SuperNode.\n");
}
