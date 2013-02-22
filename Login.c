#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define LINE_MAX 120
#define PHASE1_STATIC_TCP_PORT "21325"
#define SUPERNODE_STATIC_PORT 22325
#define SUPERNODE_INFO "68.181.201.3 3325"
#define NUM_THREADS 1

typedef struct username_password {
	char username[100];
	char passwd[100];
}u_p;

typedef struct username_ipaddr {
	char username[100];
	char ipaddr[100];
}u_ip;

u_p list_of_u_p[20]; // list of username and passwords.
u_ip list_of_u_ip[20];// list of username and IP Address.

void read_unames_pwds_from_file(FILE *);
void Phase1(void);
void Phase2(void);

/*
 * read input from the given file and call Phase1 & Phase2
 */
int
main(int argc, char *argv[]) {

	FILE *fp = NULL;
	
	if (argv[1])
	{
		fp = fopen(argv[1], "r");
			
		read_unames_pwds_from_file(fp);
		
		fclose(fp);
		
		Phase1();
		
		Phase2();
	} else {
		printf("File name should be present\n");
		exit(0);
	}
	
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

void
read_unames_pwds_from_file(FILE *fp) {

	char line[LINE_MAX], temp_buffer[LINE_MAX];
	int i, j, num_bytes = 0;
	
	memset(line, 0, LINE_MAX);
	memset(temp_buffer, 0, LINE_MAX);

	while(fgets(line, LINE_MAX, fp) != NULL) {
		sscanf(line, "%[^ ]", temp_buffer);
		num_bytes = size(temp_buffer);

		for (i = 0; i < 20; i++) {
			if (list_of_u_p[i].username[0] == '\0') {
				break;
			}
		}
		
		for (j = 0; temp_buffer[j] != '\0'; j++) {
			list_of_u_p[i].username[j] = temp_buffer[j];
		}
		
		list_of_u_p[i].username[j] = '\0';
		
		memset(temp_buffer, 0, LINE_MAX);
		
		sscanf(&line[num_bytes], "%[^\n]", temp_buffer);
		
		for (j = 0; temp_buffer[j] != '\0'; j++) {
			list_of_u_p[i].passwd[j] = temp_buffer[j];
		}

		list_of_u_p[i].passwd[j] = '\0';
	}
}

/*
 * open a socket listen for incoming connections from users,
 * read messages sent and store them.
 */
void
Phase1() {

	int sock_num = -1, new_sock = -1, status = 0, num_users = 0, num_bytes = 0;
	int i, j, found = 0;
	struct addrinfo hints, *servinfo;
	struct sockaddr client_addr;
	char msg[100], ip[32], username[100], passwd[100];
	char temp_buffer[LINE_MAX];
	pthread_t threads[NUM_THREADS];

	memset(username, 0, 100);
	memset(passwd, 0, 100);
	memset(temp_buffer, 0, LINE_MAX);
	memset(msg, 0, 100);
	
	sock_num = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo("nunki.usc.edu", PHASE1_STATIC_TCP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	inet_ntop(AF_INET, &(((struct sockaddr_in *)servinfo->ai_addr)->sin_addr), ip, 32);
	printf("Phase 1: LogIn server has TCP port number %s and IP address %s\n", PHASE1_STATIC_TCP_PORT, ip);

	if (bind(sock_num, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		perror("bind\n");
		exit(1);
	}
	
	if (listen(sock_num, 3) == -1) {
		perror("listen\n");
		exit(1);
	}
	
	status = sizeof(client_addr);
	
	while (num_users != 3) {
	
		new_sock = accept(sock_num, &client_addr, (socklen_t *)&status);
		
		if (new_sock == -1) {
			perror("accept");
			exit(1);
		}
				
		if (recv(new_sock, msg, 100, 0) == -1) {
			perror("recv");
		}
		
		sscanf(msg, "%[^#]", temp_buffer);
		num_bytes = size(temp_buffer);
		
		memset(temp_buffer, 0, LINE_MAX);
		
		sscanf(&msg[num_bytes], "%[^ ]", temp_buffer);
		num_bytes += size(temp_buffer);
		strncpy(username, temp_buffer, strlen(temp_buffer)+1);
		
		for (i = num_bytes, j = 0; msg[i] != '\0'; i++, j++) {
			passwd[j] = msg[i];
		}
		
		for (i = 0; i < 20; i++) {
			if (list_of_u_p[i].username[0] && list_of_u_p[i].passwd[0] &&
				!strncmp(list_of_u_p[i].username, username, strlen(username)) &&
				!strncmp(list_of_u_p[i].passwd, passwd, strlen(passwd))) {
				found = 1;
				
				printf("Phase 1: Authentication request. User: %s Password: %s User IP Addr: %s. Authorized: %s\n", username, passwd, inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr), "Accepted");
				
				for (j = 0; j < 20; j++) {
					if (list_of_u_ip[j].username[0] == '\0') {
						break;
					}
				}
				
				strncpy(list_of_u_ip[j].username, username, strlen(username));
				strncpy(list_of_u_ip[j].ipaddr, (char *)inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr), strlen((char *)inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr)));
				
				if (send(new_sock, "Accepted#", strlen("Accepted#"), 0) == -1) {
					perror("send");
				}

				if (send(new_sock, SUPERNODE_INFO, strlen(SUPERNODE_INFO), 0) == -1) {
					perror("send");
				}
				printf("Phase 1: SuperNode IP Address: %s Port Number: %s sent to the <User%d>\n", "68.181.201.3", "3325", num_users+1);
				break;
			}
		}
		
		if (!found) {
			printf("Phase 1: Authentication request. User: %s Password: %s User IP Addr: %s. Authorized: %s\n", username, passwd, inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr), "Rejected");
			if (send(new_sock, "Rejected#", strlen("Rejected#"), 0) == -1) {
				perror("send");
			}				
			found = 0;
		}
		
		close(new_sock);
		
		memset(username, 0, 100);
		memset(passwd, 0, 100);
		memset(temp_buffer, 0, LINE_MAX);
		memset(msg, 0, 100);
		
		num_users++;
	}
	
	close(sock_num);
	printf("End of Phase 1 for Login Server\n");
}

/*
 * send stored information from Phase 1 to Supernode
 */
void
Phase2() {

	int sock_num = -1, i, count = 0;
	struct sockaddr_in dest;
	struct sockaddr name;
	socklen_t size_name = 0;
	char msg[100];
	
	memset(msg, 0, 100);
	
	sock_num = socket(AF_INET, SOCK_STREAM, 0);

	memset(&dest, 0, sizeof(struct sockaddr_in));
	inet_pton(AF_INET, "68.181.201.3", &(dest.sin_addr));
	dest.sin_port = htons(SUPERNODE_STATIC_PORT);
	dest.sin_family = AF_INET;
	
	if (connect(sock_num, (struct sockaddr *)&dest, sizeof(dest)) == -1) {
		printf("connect failure\n");
		exit(1);
	}
	
	size_name = sizeof(name);
	(void)getsockname(sock_num, &name, &size_name);
	
	printf("Phase 2: LogIn server has TCP port number %u and IP address %s\n", ntohs(((struct sockaddr_in *)&name)->sin_port), "68.181.201.3");
	
	for (i = 0; i < 20; i++) {
		if(list_of_u_ip[i].username[0] == '\0') {
			break;
		} else {
			memset(msg, 0, 100);
			
			strncpy(msg, list_of_u_ip[i].username, strlen(list_of_u_ip[i].username));
			strncpy(&msg[strlen(list_of_u_ip[i].username)], (char *)"/", 1);
			strncpy(&msg[strlen(list_of_u_ip[i].username)+1], list_of_u_ip[i].ipaddr, strlen(list_of_u_ip[i].ipaddr));
			
			if (send(sock_num, msg, 100, 0) == -1) {
				perror("send\n");
				exit(1);
			}
			count++;
			memset(msg, 0, 100);
		}
	}
	
	printf("Phase 2: LogIn server sent %d username/IP address pairs to SuperNode.\n", count);
	close(sock_num);
	printf("End of Phase 2 for Login Server\n");
}
