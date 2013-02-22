#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define LINE_MAX 120
#define PHASE1_STATIC_TCP_PORT 21325

char username[LINE_MAX], passwd[LINE_MAX];
char ipaddr[50], port_num[10];

void read_uname_pwd_from_file(FILE *, char *);
void Phase1(char *, int);
void Phase3(int);
void User(int);

/*
 * Using Fork() to simulate 3 users.
 */
int
main(int argc, char *argv[]) {

	int ret = -1;
	
	ret = fork();
	
	if (ret == 0) {
		User(1);
		exit(0);
	} else {
		ret = 0;
		
		ret = fork();
		if (ret == 0) {
			User(2);
			exit(0);
		} else {
			User(3);
		}
	}
	
	return 0;
}

/*
 * Every User will read username & password then send it to
 * Login server for Authentication purpose.
 */
void
User(int user_num) {

	FILE *fp = NULL;
	char msg[200], file_name[50];

	memset(msg, 0, 200);
	memset(file_name, 0, 50);
	
	strncpy(file_name, "UserPass", strlen("UserPass"));
	if (user_num == 1) {
		strncpy(&file_name[8], "1", 1);
	} else if (user_num == 2) {
		strncpy(&file_name[8], "2", 1);
	} else {
		strncpy(&file_name[8], "3", 1);
	}
	strncpy(&file_name[9], ".txt", strlen(".txt"));
	
	fp = fopen(file_name, "r");
	
	if (fp) {
		read_uname_pwd_from_file(fp, msg);
		
		fclose(fp);

		Phase1(msg, user_num);
		
		sleep(1);
		
		Phase3(user_num);
	} else {
		printf("File pointer is NULL\n");
		exit(0);
	}
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
read_uname_pwd_from_file(FILE *fp, char *msg) {

	char line[LINE_MAX], temp_buffer[LINE_MAX];
	int i, j, num_bytes = 0;
	char *login = "Login#";

	memset(line, 0, LINE_MAX);
	memset(temp_buffer, 0, LINE_MAX);

	(void)fgets(line, LINE_MAX, fp);
	
	sscanf(line, "%[^ ]", temp_buffer);
	num_bytes = size(temp_buffer);
	
	for (i = 0; temp_buffer[i] != '\0'; i++) {
		username[i] = temp_buffer[i];
	}
	
	username[i] = '\0';
	
	memset(temp_buffer, 0, LINE_MAX);
	
	sscanf(&line[num_bytes], "%[^\n]", temp_buffer);
	
	for (i = 0; temp_buffer[i] != '\0'; i++) {
		passwd[i] = temp_buffer[i];
	}

	passwd[i] = '\0';
	
	for (i = 0; i < 6; i++) {
		msg[i] = login[i];
	}
	
	for(j = 0; username[j] != '\0'; j++, i++) {
		msg[i] = username[j];
	}
	
	msg[i++] = ' ';
	
	for(j = 0; passwd[j] != '\0'; j++, i++) {
		msg[i] = passwd[j];
	}
}

/*
 * open a socket and connect to Login server & exchange information.
 */
void
Phase1(char *msg, int num) {

	int sock_num = -1, num_bytes = 0, i;
	struct sockaddr_in dest;
	char temp[9];
	struct sockaddr name;
	socklen_t size_name = 0;
	
	sock_num = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&dest, 0, sizeof(struct sockaddr_in));
	inet_pton(AF_INET, "68.181.201.3", &(dest.sin_addr));
	dest.sin_port = htons(PHASE1_STATIC_TCP_PORT);
	dest.sin_family = AF_INET;
	
	if (connect(sock_num, (struct sockaddr *)&dest, sizeof(dest)) == -1) {
		printf("connect failure\n");
		exit(1);
	}

	size_name = sizeof(name);
	(void)getsockname(sock_num, &name, &size_name);

	printf("Phase 1:  <User%d> has TCP port %u and IP address: %s\n", num, ntohs(((struct sockaddr_in *)&name)->sin_port),"68.181.201.3");
	
	printf("Phase 1: Login request. User: %s password: %s\n", username, passwd);
	
	if (send(sock_num, msg, strlen(msg), 0) == -1) {
		perror("send\n");
		exit(1);
	}
	
	memset(temp, 0, 9);
	
	if (recv(sock_num, temp, 9, 0) == -1) {
		perror("recv\n");
		exit(1);
	}
	
	printf("Phase 1: Login request reply: %s\n", temp);
	
	memset(msg, 0, 200);
	
	if (strncmp(temp, "Accepted#", strlen("Accepted#")) == 0) {
		if (recv(sock_num, msg, 200, 0) == -1) {
			perror("recv\n");
			exit(1);
		}
		
		memset(ipaddr, 0, 50);
		memset(port_num, 0, 10);
		
		sscanf(msg, "%[^ ]", ipaddr);
		num_bytes = strlen(ipaddr);
		for (i = 0; msg[num_bytes+i+1] != '\0'; i++) {
			port_num[i] = msg[num_bytes+i+1];
		}
		
		printf("Phase 1: Supernode has IP Address %s and Port Number %s\n", ipaddr, port_num);
	}
	
	printf("End of Phase 1 for <User%d>.\n", num);
}

/*
 * read messages from a file, inorder to communicate with another
 * User send the message to Supernode. 
 */
void
Phase3(int num) {

	int sock_num = -1, new_sock = -1, status = 0, count = 0;
	struct addrinfo *servinfo, hints;
	struct sockaddr client_addr;
	struct sockaddr_in dest;
	socklen_t addr_size;
	char msg[100], ip[32], file_name[50], portnum[10];
	FILE *fp = NULL;
	
	memset(file_name, 0, 50);
	memset(msg, 0, 100);
	memset(portnum, 0, 10);
	
	strncpy(file_name, "UserText", strlen("UserText"));
	if (num == 1) {
		strncpy(&file_name[8], "1", 1);
		strncpy(portnum, "3425", strlen("3425"));
	} else if (num == 2) {
		strncpy(&file_name[8], "2", 1);
		strncpy(portnum, "3525", strlen("3525"));
	} else {
		strncpy(&file_name[8], "3", 1);
		strncpy(portnum, "3625", strlen("3625"));
	}
	strncpy(&file_name[9], ".txt", strlen(".txt"));
	
	printf("Phase 3: <User%d> has static UDP port %s IP address %s\n", num, portnum, ipaddr);
	
	sock_num = socket(AF_INET, SOCK_DGRAM, 0);
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((status = getaddrinfo("nunki.usc.edu", port_num, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	memset(&dest, 0, sizeof(struct sockaddr_in));
	inet_pton(AF_INET, ipaddr, &(dest.sin_addr));
	dest.sin_port = htons(atoi(port_num));
	dest.sin_family = AF_INET;

	fp = fopen(file_name, "r");
	
	if (!fp) {
		printf("Unable to open file\n");
	}
	
	while(fgets(msg, 100, fp) != NULL) {
		msg[strlen(msg)-1] = '\0';
		printf("Phase 3: <User%d> is sending the message '%s' on UDP dynamic port number \n", num, msg);
		
		if (sendto(sock_num, msg, 100, 0, (struct sockaddr *)&dest, sizeof(dest)) == -1) {
			printf("sendto failure\n");
			exit(1);
		}
		memset(msg, 100, 0);
	}
	close(sock_num);
	
	sock_num = socket(AF_INET, SOCK_DGRAM, 0);
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((status = getaddrinfo("nunki.usc.edu", portnum, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	bind(sock_num, servinfo->ai_addr, servinfo->ai_addrlen);

	while(count != 2) {
		memset(msg, 100, 0);
		if (recvfrom(sock_num, msg, 100, 0, (struct sockaddr *)&dest, &addr_size) == -1) {
			printf("recvfrom failure\n");
			exit(1);
		}
		printf("Phase 3: <User%d> received the message '%s'\n", num, msg);
		count++;
	}
	printf("End of Phase 3 for <User%d>\n", num);
}
